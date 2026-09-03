"""Regenerate character conversion elsewhere and verify all output/source bytes."""
import argparse
import hashlib
import json
from pathlib import Path
from import_characters import run

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('legacy-root','reference-root','work-root'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    if args.work_root.resolve()==args.reference_root.resolve() or args.reference_root.resolve() in args.work_root.resolve().parents:
        parser.error('Verification must use a separate directory')
    run(argparse.Namespace(legacy_root=args.legacy_root,output_root=args.work_root/'output',work_root=args.work_root/'ogre'))
    checked=0
    for file in sorted((args.work_root/'output').rglob('*')):
        if not file.is_file():continue
        relative=file.relative_to(args.work_root/'output')
        if file.read_bytes()!=(args.reference_root/relative).read_bytes():raise ValueError('Output changed: '+str(relative))
        checked+=1
    manifest=json.loads((args.reference_root/'characters_manifest.json').read_text())
    for path,digest in manifest['sources'].items():
        if hashlib.sha256((args.legacy_root/path).read_bytes()).hexdigest()!=digest:raise ValueError('Legacy source changed: '+path)
    result={'identical_generated_files':checked,'verified_sources':len(manifest['sources'])}
    (args.work_root/'verification.json').write_text(json.dumps(result,indent=2))
    print(json.dumps(result))
