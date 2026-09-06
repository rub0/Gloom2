"""Regenerate the UI extraction in a separate directory and compare every byte."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--legacy-root',type=Path,required=True)
p.add_argument('--reference-root',type=Path,required=True)
p.add_argument('--work-root',type=Path,required=True)
a=p.parse_args();ref=a.reference_root.resolve();work=a.work_root.resolve()
if ref==work or ref in work.parents or work in ref.parents:p.error('Use a separate verification output directory')
subprocess.run([sys.executable,str(Path(__file__).with_name('import_ui.py')),'--legacy-root',str(a.legacy_root),'--output-root',str(work)],check=True)
files=sorted(x.relative_to(ref) for x in ref.rglob('*') if x.is_file())
if files!=sorted(x.relative_to(work) for x in work.rglob('*') if x.is_file()):raise ValueError('UI file inventory mismatch')
for file in files:
    if (ref/file).read_bytes()!=(work/file).read_bytes():raise ValueError('UI bytes differ: '+str(file))
m=json.loads((ref/'manifest.json').read_text())
for source,digest in m['sources'].items():
    if hashlib.sha256((a.legacy_root/source).read_bytes()).hexdigest()!=digest:raise ValueError('Original source modified')
print(json.dumps({'identical_generated_files':len(files),'verified_sources':len(m['sources']),'swf_files':len(m['swf'])}))
