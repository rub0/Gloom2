"""Encode fixed-step captures, compare rates and publish reproducible visual evidence.

Requires Pillow/numpy plus an ffmpeg executable. Does not change capture/reference bytes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image, ImageDraw, ImageOps


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def checked_log(path):
    log = path.read_text(errors='replace')
    if 'Gloom Vulkan/Jolt scene completed successfully.' not in log:
        raise ValueError(f'{path.name} did not finish')
    if any(marker in log for marker in ('Validation Error', 'VUID-', 'Diligent Engine: ERROR', 'Diligent Engine: Error')):
        raise ValueError(f'{path.name} contains a render validation error')
    return log


def contact(root, entries, output, columns=2):
    sheet = Image.new('RGB', (columns*640, ((len(entries)+columns-1)//columns)*388), '#121b24')
    draw = ImageDraw.Draw(sheet)
    for i, (frame, title) in enumerate(entries):
        x, y = (i % columns)*640, (i//columns)*388
        sheet.paste(Image.open(root/f'frame-{frame:04d}.ppm'), (x, y+28))
        draw.text((x+10, y+8), title, fill='white')
    sheet.save(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--ffmpeg', type=Path, required=True)
    parser.add_argument('--source-root', type=Path, default=Path('.'))
    parser.add_argument('--reference-video', type=Path)
    args = parser.parse_args()
    root = args.report.resolve()
    source = args.source_root.resolve()
    videos = root/'videos'
    videos.mkdir(exist_ok=True)
    stills = root/'stills'
    stills.mkdir(exist_ok=True)
    manifest = {'version': 1, 'capture_hz': 30, 'simulated_seconds': 40, 'rates': {},
                'comparison': {}, 'sources': {}}
    frames = {}
    for fps in (30, 60, 144):
        directory = root/'sequences'/f'{fps}fps'
        files = sorted(directory.glob('frame-*.ppm'))
        if len(files) != 1200 or [p.name for p in files] != [f'frame-{i:04d}.ppm' for i in range(1200)]:
            raise ValueError(f'{fps} Hz requires exactly 1200 consecutive captures')
        frames[fps] = files
        log = checked_log(root/f'visual-final-{fps}fps.log')
        manifest['rates'][fps] = {
            'render_frames': fps*40, 'capture_count': len(files),
            'maximum_capture_quantization_seconds': 0 if fps in (30, 60) else 1/fps,
            'summary': next(line for line in log.splitlines() if line.startswith('Animation review:')),
            'frames': {p.name: digest(p) for p in files}}
        output = videos/f'animation-{fps}hz.mp4'
        subprocess.run([str(args.ffmpeg), '-hide_banner', '-loglevel', 'error', '-y',
                        '-framerate', '30', '-i', str(directory/'frame-%04d.ppm'),
                        '-c:v', 'libx264', '-crf', '17', '-pix_fmt', 'yuv420p',
                        '-movflags', '+faststart', str(output)], check=True)
        manifest['rates'][fps]['video_sha256'] = digest(output)

    # Compare the same nominal instants. A 144-Hz capture can be one render tick later.
    regions = {'frame': (0, 0, 640, 360), 'body': (250, 55, 390, 245), 'fps': (270, 230, 580, 360)}
    for fps in (60, 144):
        errors = {name: [] for name in regions}
        for reference, actual in zip(frames[30], frames[fps]):
            a, b = np.asarray(Image.open(reference), dtype=np.float32), np.asarray(Image.open(actual), dtype=np.float32)
            difference = np.abs(a-b)
            for name, (x0, y0, x1, y1) in regions.items():
                errors[name].append(float(difference[y0:y1, x0:x1].mean()))
        manifest['comparison'][f'30_vs_{fps}'] = {
            name: {'mean_absolute_error_255': float(np.mean(values)),
                   'p95_frame_mae_255': float(np.percentile(values, 95)),
                   'maximum_frame_mae_255': max(values),
                   'worst_frame': int(np.argmax(values))}
            for name, values in errors.items()}

    worst = Image.new('RGB', (1920, 1164), '#121b24')
    draw = ImageDraw.Draw(worst)
    case_frames = (manifest['comparison']['30_vs_144']['frame']['worst_frame'],
                   manifest['comparison']['30_vs_60']['fps']['worst_frame'],
                   manifest['comparison']['30_vs_60']['body']['worst_frame'])
    for row, frame in enumerate(case_frames):
        for column, fps in enumerate((30, 60, 144)):
            x, y = column*640, row*388
            worst.paste(Image.open(frames[fps][frame]), (x, y+28))
            draw.text((x+8, y+8), f'{fps} Hz / frame {frame} / {frame/30:.3f} s', fill='white')
    worst.save(root/'rate-worst-cases.png')
    manifest['inspected_rate_case_frames'] = case_frames

    directory = root/'sequences/60fps'
    contact(directory, [(15, 'Archangel / idle / inspection'), (45, 'Archangel / forward'),
                        (114, 'Archangel / jump'), (136, 'Archangel / confirmed shot'),
                        (166, 'Archangel / death'), (276, 'Archangel / respawn'),
                        (315, 'Shadow / idle / inspection'), (345, 'Shadow / forward'),
                        (414, 'Shadow / jump'), (436, 'Shadow / confirmed shot'),
                        (466, 'Shadow / death'), (576, 'Shadow / respawn'),
                        (200, 'Archangel / final corpse pose'),
                        (500, 'Shadow / final corpse pose')], root/'motion-contact.png', 3)
    contact(directory, [(248, 'Archangel / FPS up'), (263, 'Archangel / FPS down'),
                        (548, 'Shadow / FPS up'), (563, 'Shadow / FPS down'),
                        (645, 'Archangel / normal Factory lighting'),
                        (945, 'Shadow / normal Factory lighting')], root/'fps-lighting-contact.png')
    contact(directory, [(136, 'Muzzle / impact'), (142, 'Damage / shield'),
                        (186, 'Blood: review sample'), (214, 'Explosion / heat: review sample'),
                        (350, 'Shadow / smoke and red trails'), (276, 'Spawn')], root/'effects-contact.png')
    for i in (15, 45, 136, 142, 166, 186, 200, 214, 248, 263, 276, 315, 345, 350, 436, 500, 548, 563, 645, 945):
        Image.open(directory/f'frame-{i:04d}.ppm').save(stills/f'frame-{i:04d}.png')

    concepts = Image.new('RGB', (1200, 800), '#121b24')
    draw = ImageDraw.Draw(concepts)
    for row, (name, image, inspection, game) in enumerate([
            ('Archangel', 'archangel.jpg', 45, 645), ('Shadow', 'shadow-painting.jpg', 345, 945)]):
        reference = ImageOps.contain(Image.open(source/'docs/art/characters'/image), (350, 360))
        concepts.paste(reference, ((400-reference.width)//2, row*400+35))
        draw.text((10, row*400+10), name+' / supplied concept', fill='white')
        for col, frame, title in ((1, inspection, 'animated / inspection'), (2, game, 'animated / Factory lights')):
            body = Image.open(directory/f'frame-{frame:04d}.ppm').crop((220, 42, 420, 242)).resize((350, 350))
            concepts.paste(body, (col*400+25, row*400+35))
            draw.text((col*400+10, row*400+10), title, fill='white')
    concepts.save(root/'concept-comparison.png')

    lava = root/'sequences/lava'
    checked_log(root/'visual-lava.log')
    if len(list(lava.glob('*.ppm'))) != 90:
        raise ValueError('Missing three-second lava acceptance sequence')
    subprocess.run([str(args.ffmpeg), '-hide_banner', '-loglevel', 'error', '-y', '-framerate', '30',
                    '-i', str(lava/'frame-%04d.ppm'), '-c:v', 'libx264', '-crf', '17',
                    '-pix_fmt', 'yuv420p', '-movflags', '+faststart', str(videos/'lava.mp4')], check=True)
    contact(lava, [(0, 'Factory lava / start'), (30, 'Factory lava / 1 s'),
                   (60, 'Factory lava / 2 s'), (89, 'Factory lava / 3 s')], root/'lava-contact.png')
    manifest['lava'] = {p.name: digest(p) for p in sorted(lava.glob('*.ppm'))}
    samples = root/'sequences/samples'
    checked_log(root/'visual-samples.log')
    if len(list(samples.glob('*.ppm'))) != 90:
        raise ValueError('Missing three-second blood/explosion/heat acceptance sequence')
    subprocess.run([str(args.ffmpeg), '-hide_banner', '-loglevel', 'error', '-y', '-framerate', '30',
                    '-i', str(samples/'frame-%04d.ppm'), '-c:v', 'libx264', '-crf', '17',
                    '-pix_fmt', 'yuv420p', '-movflags', '+faststart', str(videos/'effects-samples.mp4')], check=True)
    contact(samples, [(15, 'Heat / before burst'), (34, 'Blood / review only'),
                      (63, 'Explosion / review only'), (72, 'Explosion / dissipation')], root/'effects-samples-contact.png')
    manifest['samples'] = {p.name: digest(p) for p in sorted(samples.glob('*.ppm'))}
    for name, indices, crop in [('muzzle-closeup', range(134, 140), (350, 240, 570, 350)),
                                ('shadow-closeup', (315, 330, 345, 360, 375, 390), (250, 65, 390, 245))]:
        sheet = Image.new('RGB', (900, 440), '#121b24')
        draw = ImageDraw.Draw(sheet)
        for i, frame in enumerate(indices):
            x, y = (i % 3)*300, (i//3)*220
            detail = ImageOps.contain(Image.open(directory/f'frame-{frame:04d}.ppm').crop(crop), (300, 190))
            sheet.paste(detail, (x+(300-detail.width)//2, y+28))
            draw.text((x+8, y+8), f'{frame/30:.3f} s', fill='white')
        sheet.save(root/(name+'.png'))
    manifest['auxiliary_video_sha256'] = {p.name: digest(p) for p in (videos/'lava.mp4', videos/'effects-samples.mp4')}
    for relative in ('assets/characters/original/characters_manifest.json', 'assets/effects/manifest.json',
                     'assets/effects/recipes.json', 'apps/gloom/animation_review.hpp', 'apps/gloom/main.cpp',
                     'src/assets/animation.cpp', 'src/assets/gltf_importer.cpp', 'src/assets/scene_gpu_bridge.cpp',
                     'src/gameplay/character_animation.cpp', 'src/gameplay/combat_effects.cpp',
                     'src/render/particles.cpp', 'src/backends/diligent_renderer.cpp',
                     'tools/legacy/import_characters.py', 'tools/legacy/import_effects.py',
                     'tools/review/animation_report.py', 'CMakeLists.txt',
                     'tests/animation_vfx_tests.cpp', 'tests/animation_network_tests.cpp',
                     'build/windows-vs/Debug/gloom.exe', 'build/windows-vs/Debug/gloom_slice_server.exe'):
        manifest['sources'][relative] = digest(source/relative)

    if args.reference_video:
        # Only the user-specified first two minutes; no material from later sections.
        reference_contact = Image.new('RGB', (1280, 776), '#121b24')
        draw = ImageDraw.Draw(reference_contact)
        for i, second in enumerate((23, 36, 45, 119)):
            path = stills/f'reference-{second:03d}s.png'
            subprocess.run([str(args.ffmpeg), '-hide_banner', '-loglevel', 'error', '-y',
                            '-ss', str(second), '-i', str(args.reference_video), '-frames:v', '1',
                            '-vf', 'scale=640:360', str(path)], check=True)
            x, y = (i % 2)*640, (i//2)*388
            reference_contact.paste(Image.open(path), (x, y+28))
            draw.text((x+8, y+8), f'Original video / {second//60}:{second%60:02d}', fill='white')
        reference_contact.save(root/'video-reference-contact.png')
        manifest['reference_video'] = {'url': 'https://www.youtube.com/watch?v=yJPoulcfIAg',
                                       'range_seconds': [0, 120], 'sha256': digest(args.reference_video)}
    (root/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps({'rates': {k: {x: y for x, y in v.items() if x != 'frames'} for k, v in manifest['rates'].items()},
                      'comparison': manifest['comparison']}, indent=2))


if __name__ == '__main__':
    main()
