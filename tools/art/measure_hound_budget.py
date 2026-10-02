"""Serial 1080p H06 envelope measurements after prepare_hound_budget.py and Release build."""
from pathlib import Path
import json
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
V17 = '--v17' in sys.argv
WORK = ROOT/('.cache/hound-production-v17' if V17 else '.cache/hound-budget-106')


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
    if options and options[0].startswith(('--hound-budget','--hound-production')):
        assert 'weapon_mask=31' in text, name
    if V17:
        assert 'H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31' in text
        assert re.search(r'GPU asset residency: [0-9.]+ MiB; evictions=0 ', text), name
    if '--audio-device' in options:
        assert 'Audio output: SDL device' in text, 'Real audio device is required for this series'
    lines = [line for line in text.splitlines() if re.match(
        r'Factory benchmark:|GPU full render:|CPU animation|Frame budget:|Factory render:|GPU asset residency:|'
        r'Process memory:|Texture compression:|H06 budget|H07 v17|Skin uploads:|Animated TPS|Swapchain:|Audio output:', line)]
    for suffix in ('a', 'b'):
        shutil.copyfile(ROOT/f'.cache/hito102-hound-{suffix}.ppm', WORK/f'{name}-{suffix}.ppm')
    print(name+': '+next(line for line in lines if line.startswith('Factory benchmark:')), flush=True)
    return {'name': name, 'options': options, 'evidence': lines}


def main():
    WORK.mkdir(parents=True, exist_ok=True)
    rows = []
    profiles = [('v17-lod0', ['--hound-production-v17','--hound-budget-lod=0'])] if V17 else [
        ('v16', []), ('budget-lod0', ['--hound-budget-lod=0']), ('budget-lod1', ['--hound-budget-lod=1']), ('budget-lod2', ['--hound-budget-lod=2'])]
    for name, options in profiles:
        for audio in (False, True):
            for repeat in ('a', 'b'):
                rows.append(run(f'{name}-{"audio" if audio else "null"}-{repeat}', options+(['--audio-device'] if audio else [])))
    (WORK/'measurements.json').write_text(json.dumps(rows, indent=2)+'\n', encoding='utf-8')
    if V17:
        for row in rows:
            text = '\n'.join(row['evidence'])
            timing = re.search(r'Factory benchmark:.*p99=([0-9.]+) max=([0-9.]+)', text)
            assert timing and float(timing[1])<=4 and float(timing[2])<=5, row['name']+' exceeds H06 frame budget'
            assert 'over_5ms=0/360' in text
            assert float(re.search(r'GPU asset residency: ([0-9.]+)', text)[1])<=384
    print(f'{len(rows)} serial runs complete; timing distributions retained, no pass inferred from the mean', flush=True)


if __name__ == '__main__':
    main()
