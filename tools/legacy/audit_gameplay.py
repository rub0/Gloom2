"""Extract original gameplay data without executing Lua or modifying Legacy."""
import argparse
import hashlib
import json
from pathlib import Path
from lua_data import DataParser

def audit(legacy):
    sources = {}
    def read(relative):
        p = legacy / relative
        sources[relative] = hashlib.sha256(p.read_bytes()).hexdigest()
        return p.read_text(encoding='latin1')
    def table(name):
        return DataParser(read('Exes/media/maps/' + name)).document()
    archetypes = table('archetypes.txt')
    movement_keys = ('acceleration','maxVelocity','frictionCoef','airFrictionCoef',
                     'gravity','airSpeedCoef','jumpForce','dodgeForce','physic_radius','physic_height')
    characters = {name:{key:archetypes[name][key] for key in movement_keys}
                  for name in ('Hound','Archangel','Shadow','Screamer')}
    weapons = {name:archetypes[name] for name in ('SoulReaper','Sniper','ShotGun','MiniGun','IronHellGoat')}
    pickups = {name:data for name,data in archetypes.items()
               if isinstance(data,dict) and data.get('id') in ('weapon','ammo','orb','armor','damageAmplifier','cooldownReducer')}
    maps = {}
    for name in ('Factory.map','Factory_server.txt','Factory_client.txt'):
        entities=table(name)
        maps[name]=[{ 'name':key, 'type':value['type'], 'position':value.get('position'),
                     'resolved':dict(pickups[value['type']],**value)}
                    for key,value in entities.items() if value.get('type') in pickups]
    components=('AvatarController','PhysicController','Weapon','WeaponAmmo','WeaponsManager',
                'SoulReaper','SoulReaperAmmo','Sniper','SniperAmmo','ShotGun','ShotGunAmmo',
                'MiniGun','MiniGunAmmo','IronHellGoat','IronHellGoatAmmo','MagneticBullet',
                'FireBallController','SpawnItemManager','Life')
    for name in components:
        read('Src/Logic/Entity/Components/'+name+'.cpp')
    for path in ('Src/Logic/Maps/Map.cpp','Src/Application/GameState.cpp',
                 'Src/Physics/CharacterController.cpp','Src/Input/PlayerController.cpp','Src/Input/PlayerController.h'):
        read(path)
    return {'version':1,'unit_scale':.15,'legacy_fixed_step_ms':16,'double_tap_ms':300,
            'characters':characters,'weapons':weapons,'pickups':pickups,'maps':maps,'sources':sources}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--legacy-root',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args()
    legacy=a.legacy_root.resolve();output=a.output.resolve()
    if output==legacy or legacy in output.parents:raise ValueError('Output must be outside Legacy')
    text=json.dumps(audit(legacy),indent=2,ensure_ascii=False)+'\n'
    if a.verify:
        if output.read_text(encoding='utf-8')!=text:raise ValueError('Gameplay source audit changed')
    else:
        output.parent.mkdir(parents=True,exist_ok=True);output.write_text(text,encoding='utf-8')
    result=json.loads(text)
    print(json.dumps({'weapons':len(result['weapons']),'pickup_types':len(result['pickups']),
                      'map_pickups':{name:len(items) for name,items in result['maps'].items()},
                      'verified_sources':len(result['sources'])}))
