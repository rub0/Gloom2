"""Serial 1080p H06 envelope measurements after prepare_hound_budget.py and Release build."""
from pathlib import Path
import json
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT/'.cache/hound-budget-106'


def run(name, options):
    path = WORK/(name+'.log')
    with path.open('w', encoding='utf-8') as output:
        result = subprocess.run(['rtk', 'proxy', str(ROOT/'build/windows-vs/Release/gloom.exe'),
            '--vertical-slice-performance-hound-eight-1080p', '--present=immediate', *options],
            cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, timeout=120)
    text = re.sub(r'\x1b\[[0-9;]*m', '', path.read_text(encoding='utf-8', errors='replace'))
    assert result.returncode == 0, text[-3000:]
    assert not re.search(r'Diligent Engine: ERROR|VUID-|mapping failed', text), name
    for required in ('samples=360', 'render: 1920x1080 output=1920x1080', 'skinned=53 visible_skinned=53',
                     'Animated TPS mask: 127', 'measured missing meshes=0 textures=0', 'measured evictions=0 budget_limited_frames=0'):
        assert required in text, (name, required)
    assert 'Texture compression: BC5/BC7' in text, 'This budget targets the measured GTX 1070 BC path'
    if options and options[0].startswith('--hound-budget'):
        assert 'weapon_mask=31' in text, name
    if '--audio-device' in options:
        assert 'Audio output: SDL device' in text, 'Real audio device is required for this series'
    lines = [line for line in text.splitlines() if re.match(
        r'Factory benchmark:|GPU full render:|CPU animation|Frame budget:|Factory render:|GPU asset residency:|'
        r'Process memory:|Texture compression:|H06 budget|Skin uploads:|Animated TPS|Swapchain:|Audio output:', line)]
    for suffix in ('a', 'b'):
        shutil.copyfile(ROOT/f'.cache/hito102-hound-{suffix}.ppm', WORK/f'{name}-{suffix}.ppm')
    print(name+': '+next(line for line in lines if line.startswith('Factory benchmark:')), flush=True)
    return {'name': name, 'options': options, 'evidence': lines}


def main():
    WORK.mkdir(parents=True, exist_ok=True)
    rows = []
    for name, options in [('v16', []), ('budget-lod0', ['--hound-budget-lod=0']),
                          ('budget-lod1', ['--hound-budget-lod=1']), ('budget-lod2', ['--hound-budget-lod=2'])]:
        for audio in (False, True):
            for repeat in ('a', 'b'):
                rows.append(run(f'{name}-{"audio" if audio else "null"}-{repeat}', options+(['--audio-device'] if audio else [])))
    (WORK/'measurements.json').write_text(json.dumps(rows, indent=2)+'\n', encoding='utf-8')
    print('16 serial runs complete; timing distributions retained, no pass inferred from the mean', flush=True)


if __name__ == '__main__':
    main()
