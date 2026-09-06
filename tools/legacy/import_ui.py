"""Recover SWF bitmap resources and build the modern UI atlas; no Flash runtime."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import zlib
from PIL import Image, ImageDraw, ImageFont

def swf_tags(path):
    raw = path.read_bytes()
    data = raw[8:] if raw[:3] == b'FWS' else zlib.decompress(raw[8:]) if raw[:3] == b'CWS' else None
    if data is None:
        raise ValueError('Unsupported SWF compression: '+str(path))
    if len(data)+8 != struct.unpack_from('<I', raw, 4)[0]:
        raise ValueError('SWF length mismatch')
    offset = (5+4*(data[0] >> 3)+7)//8+4
    while offset+2 <= len(data):
        head, = struct.unpack_from('<H', data, offset); offset += 2
        kind, size = head >> 6, head & 63
        if size == 63:
            size, = struct.unpack_from('<I', data, offset); offset += 4
        body = data[offset:offset+size]; offset += size
        if len(body) != size:
            raise ValueError('Truncated SWF tag')
        yield kind, body
        if not kind:
            break

def recover(path, output):
    tags, images = {}, []
    for kind, data in swf_tags(path):
        tags[str(kind)] = tags.get(str(kind), 0)+1
        image = None
        if kind in (20, 36) and data[2] == 5:
            width, height = struct.unpack_from('<HH', data, 3)
            pixels = zlib.decompress(data[7:])
            import numpy as np
            raw = np.frombuffer(pixels, dtype='uint8').reshape(height,width,4)
            rgba = raw[:,:,[1,2,3,0]].copy()
            if kind == 20:
                rgba[:,:,3] = 255
            image = Image.fromarray(rgba)
            if kind == 36:
                import numpy as np
                p = np.array(image, dtype='uint16'); a = p[:,:,3:4]
                p[:,:,:3] = np.minimum(255, (p[:,:,:3]*255+ a//2)//np.maximum(a,1))
                image = Image.fromarray(p.astype('uint8'))
        elif kind == 21:
            image = Image.open(io.BytesIO(data[2:])).convert('RGBA')
        elif kind == 35:
            length, = struct.unpack_from('<I', data, 2)
            image = Image.open(io.BytesIO(data[6:6+length])).convert('RGBA')
            image.putalpha(Image.frombytes('L', image.size, zlib.decompress(data[6+length:])))
        if image:
            name = path.stem+'-'+str(struct.unpack_from('<H', data)[0])+'.png'
            image.save(output/name)
            images.append({'name': name, 'size': image.size})
    return {'tags': tags, 'images': images}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--legacy-root', type=Path, required=True)
    parser.add_argument('--output-root', type=Path, required=True)
    args = parser.parse_args()
    output = args.output_root; output.mkdir(parents=True, exist_ok=True)
    recovered = output/'recovered'; recovered.mkdir(exist_ok=True)
    audit = {}; sources = {}
    for path in sorted((args.legacy_root/'Exes/media/gui').glob('*.swf')):
        audit[path.name] = recover(path, recovered)
        sources[path.relative_to(args.legacy_root).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    atlas = Image.new('RGBA', (4096,2048)); entries = []; x=y=row=2
    def pack(name, image, advance=0):
        nonlocal x,y,row
        image = image.convert('RGBA')
        if x+image.width+2 > atlas.width:
            x=2; y+=row+2; row=0
        if y+image.height+2 > atlas.height:
            raise ValueError('UI atlas capacity exceeded')
        atlas.paste(image,(x,y)); entries.append(f'{name} {x} {y} {image.width} {image.height} {advance}')
        x+=image.width+2; row=max(row,image.height)
    pack('white',Image.new('RGBA',(2,2),'white'))
    selected = {'frame':'Hud-80.png','reaper':'Hud-96.png','bite':'Hud-171.png',
                'heal':'Hud-161.png','health':'Hud-83.png','crosshair':'Hud-40.png',
                'logo':'MenuPrincipal-34.png','archangel':'SeleccionPersonaje-94.png',
                'shadow':'SeleccionPersonaje-85.png','art':'LoadingMenu-1.png',
                'minigun':'Hud-135.png','shotgun':'Hud-148.png','sniper':'Hud-119.png','goat':'Hud-109.png',
                'button':'MenuPrincipal-13.png','ring':'Hud-13.png'}
    for name,file in selected.items():
        im=Image.open(recovered/file)
        if name=='logo':im=im.crop(im.getbbox())
        im.thumbnail((768,512)); pack(name,im)
        if name=='health':
            pack('life_icon',im.crop((0,0,28,31)))
            pack('shield_icon',im.crop((0,31,28,62)))
        if name=='frame':
            pack('frame_left',im.crop((0,0,96,im.height)))
            pack('frame_middle',im.crop((96,0,im.width-96,im.height)))
            pack('frame_right',im.crop((im.width-96,0,im.width,im.height)))
    background=args.legacy_root/'Src/Flash GUI/MenuPrincipal/bg.png'
    im=Image.open(background); im.thumbnail((1280,720)); pack('background',im)
    sources[background.relative_to(args.legacy_root).as_posix()]=hashlib.sha256(background.read_bytes()).hexdigest()
    for prefix, relative in [('g','Exes/media/gui/fonts/DejaVuSans.ttf'),('h','Exes/media/fonts/HammerheadThin.ttf')]:
        path=args.legacy_root/relative; sources[relative]=hashlib.sha256(path.read_bytes()).hexdigest()
        font=ImageFont.truetype(str(path),32)
        for code in range(32,256):
            char=chr(code); width=max(1,int(font.getlength(char)+2))
            glyph=Image.new('RGBA',(width,42)); ImageDraw.Draw(glyph).text((0,0),char,font=font,fill='white')
            pack(prefix+str(code),glyph,float(font.getlength(char)))
    atlas.save(output/'atlas.png'); (output/'atlas.rgba').write_bytes(atlas.tobytes())
    (output/'atlas.txt').write_text('4096 2048\n'+'\n'.join(entries)+'\n')
    (output/'manifest.json').write_text(json.dumps({'sources': sources, 'swf': audit,
        'selected_bitmaps':selected,'atlas_sha256':hashlib.sha256(atlas.tobytes()).hexdigest()}, indent=2)+'\n')
    images = sorted(recovered.glob('*.png'))
    sheet = Image.new('RGB', (1000, ((len(images)+4)//5)*130), '#28313b'); draw = ImageDraw.Draw(sheet)
    for i,p in enumerate(images):
        im = Image.open(p); im.thumbnail((190, 100))
        x,y=(i%5)*200,(i//5)*130
        sheet.paste(im,(x,y+25),im); draw.text((x+2,y+5),p.name,fill='white')
    sheet.save(output/'recovered-contact.png')
    print('SWF:',len(audit),'bitmaps:',len(images))

if __name__ == '__main__':
    main()
