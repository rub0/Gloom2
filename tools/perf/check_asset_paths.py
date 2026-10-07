"""Cook through native Unicode arguments and reject a Windows junction leaving the VFS mount."""
from pathlib import Path
import json
import shutil
import subprocess
from urllib.parse import quote, unquote

ROOT = Path(__file__).resolve().parents[2]


def main():
    source = ROOT/'.cache/hito117/path-check/fuente-ñ-猫-🐺'
    cache = ROOT/'.cache/hito117/path-check/cache-ñ-猫'
    outside = ROOT/'.cache/hito117/path-check/outside'
    cache.mkdir(parents=True, exist_ok=True)
    outside.mkdir(parents=True, exist_ok=True)
    shutil.copytree(ROOT/'assets/tests/material_surface', source, dirs_exist_ok=True)
    scene = json.loads((source/'scene.gltf').read_text(encoding='utf-8'))
    image = scene['images'][0]
    original = source/unquote(image['uri'])
    filename = 'textura-ñ-猫-🐺'+original.suffix
    shutil.copyfile(original, source/filename)
    image['uri'] = quote(filename)
    (source/'escena-猫.gltf').write_text(json.dumps(scene), encoding='utf-8')
    exe = ROOT/'build/windows-vs/Release/gloom_asset_cooker.exe'
    rows = []

    def cook(name, virtual_source, result, message='', root=source):
        process = subprocess.run(['rtk','proxy',str(exe),str(root),str(cache),virtual_source,'cache:/cocinado-猫.gasset'],
                                 cwd=ROOT, text=True, capture_output=True)
        assert process.returncode == result and message in process.stderr, (name,process.returncode,process.stdout,process.stderr)
        rows.append(dict(case=name,exit=process.returncode,stdout=process.stdout,stderr=process.stderr))

    cook('Unicode roots, filename and percent-decoded image URI', 'game:/escena-猫.gltf', 0)
    assert (cache/'cocinado-猫.gasset').is_file()
    cook('traversal', 'game:/../escena-猫.gltf', 2, 'valid game:/')
    cook('missing mount root', 'game:/scene.gltf', 1, 'mount name or directory', root=outside/'absent')
    junction = source/'escape'
    if not junction.exists():
        process = subprocess.run(['rtk','proxy','cmd','/c','mklink','/J',str(junction),str(outside)],cwd=ROOT,capture_output=True,text=True)
        assert process.returncode == 0, (process.stdout,process.stderr)
    cook('junction escape', 'game:/escape/scene.gltf', 1, 'escapes its mount')
    # Pass the raw UTF-16 argument directly; a UTF-8 wrapper cannot represent an unpaired surrogate.
    invalid = subprocess.run([str(exe),str(source),str(cache),'game:/'+chr(0xD800)+'.gltf','cache:/bad.gasset'],
                             cwd=ROOT,text=True,capture_output=True)
    assert invalid.returncode == 2 and 'not valid Unicode' in invalid.stderr, (invalid.returncode,invalid.stderr)
    rows.append(dict(case='ill-formed UTF-16',exit=invalid.returncode,stderr=invalid.stderr))
    (cache.parent/'checks.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
    print('5 native asset path checks passed')


if __name__ == '__main__':
    main()
