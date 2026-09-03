"""Compose unaltered reference art and cropped runtime captures for art review."""
import argparse
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps


def build(output, references):
    captures=output/'captures'
    files=sorted(captures.glob('*.ppm'))
    sheet=Image.new('RGB',(1280,((len(files)+1)//2)*386),'#141b26')
    draw=ImageDraw.Draw(sheet)
    for i,file in enumerate(files):
        x=i%2*640;y=i//2*386
        draw.text((x+10,y+7),file.stem,fill='white')
        sheet.paste(Image.open(file).convert('RGB').resize((640,360)),(x,y+26))
    sheet.save(output/'capture-contact.png')
    comparison=Image.new('RGB',(1200,454),'#141b26');draw=ImageDraw.Draw(comparison)
    panels=[('Archangel: boceto',Image.open(references/'archangel.jpg')),
        ('Archangel: captura',Image.open(captures/'archangel-front.ppm').crop((465,145,815,570))),
        ('Shadow: boceto',Image.open(references/'shadow-painting.jpg')),
        ('Shadow: captura',Image.open(captures/'shadow-front.ppm').crop((465,145,815,570)))]
    for i,(label,picture) in enumerate(panels):
        fitted=ImageOps.contain(picture.convert('RGB'),(296,400))
        comparison.paste(fitted,(i*300+(300-fitted.width)//2,30+(400-fitted.height)//2))
        draw.text((i*300+10,9),label,fill='white')
    draw.text((10,435),'Pose de reposo y luces de inspeccion. Particulas, humo, estelas y animacion: hito 63.',fill='white')
    comparison.save(output/'concept-comparison.png')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--references',type=Path,required=True)
    args=parser.parse_args();build(args.output,args.references)
