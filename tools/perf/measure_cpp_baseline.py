"""Serial Factory/Hound baseline; never changes source assets or visual references."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--profile', action='store_true', help='Expect the optional allocation diagnostic build')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    exe = ROOT/'build/windows-vs/Release/gloom.exe'
    rows = []
    cases = [('factory-'+str(i), ['--vertical-slice-performance-1080p']) for i in range(1, 4)]
    for audio in (False, True):
        for i in (1, 2):
            cases.append((f'hound-{"audio" if audio else "null"}-{i}',
                ['--vertical-slice-performance-hound-eight-1080p', '--hound-production-v17', '--hound-budget-lod=0']
                + (['--audio-device'] if audio else [])))
    for name, options in cases:
        log = args.output/(name+'.log')
        with log.open('w', encoding='utf-8') as output:
            result = subprocess.run(['rtk', 'proxy', str(exe), *options, '--present=immediate'],
                                    cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, timeout=120)
        text = re.sub(r'\x1b\[[0-9;]*m', '', log.read_text(encoding='utf-8', errors='replace'))
        assert result.returncode == 0, (name, text[-2000:])
        assert not re.search(r'Diligent Engine: ERROR|VUID-|mapping failed', text), name
        for required in ('samples=360', 'render: 1920x1080 output=1920x1080', 'measured missing meshes=0 textures=0'):
            assert required in text, (name, required)
        if name.startswith('hound'):
            for required in ('instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53', 'Animated TPS mask: 127', 'weapon_mask=31',
                             'forced TPS LOD=0', 'Texture compression: BC5/BC7',
                             'measured evictions=0 budget_limited_frames=0'):
                assert required in text, (name, required)
            if 'audio' in name:
                assert 'Audio output: SDL device' in text, name
        else:
            assert 'instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0' in text, name
        if args.profile:
            assert 'Allocation profile: measured_frames=360' in text, name
        else:
            assert 'Allocation profile:' not in text, name
        prefixes = ('Factory benchmark:', 'CPU stages ms:', 'CPU animation ms:', 'GPU full render:',
                    'Factory render:', 'Frame budget:', 'GPU asset residency:', 'Process memory:',
                    'Skin uploads:', 'Audio output:', 'Allocation profile:', 'Allocation phase:',
                    'Allocation layout:', 'Allocation capacity:', 'Animated TPS mask:', 'Texture compression:',
                    'H07 v17 probe:', 'Swapchain:', 'Jobs:')
        evidence = [line for line in text.splitlines() if line.startswith(prefixes)]
        rows.append({'case': name, 'options': options, 'evidence': evidence})
        print(name+': '+next(line for line in evidence if line.startswith('Factory benchmark:')), flush=True)
    document = {'commit': subprocess.check_output(['rtk','proxy','git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
                'exe_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(), 'profile':args.profile,
                'finished_utc':datetime.now(timezone.utc).isoformat(),
                'source_diff_sha256':hashlib.sha256(subprocess.check_output(['rtk','proxy','git','diff','HEAD'],cwd=ROOT)).hexdigest(),
                'runs':rows}
    (args.output/'measurements.json').write_text(json.dumps(document, indent=2)+'\n',encoding='utf-8')


if __name__ == '__main__':
    main()
