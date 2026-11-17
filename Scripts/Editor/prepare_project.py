"""Prepare the selected local source profile inside Unreal Editor's Python commandlet."""
import json
import os
from pathlib import Path
import re
import runpy
import unreal as u

ROOT = Path(os.environ['PARADISE_HEAT_ROOT'])
PROFILE = os.environ['PARADISE_HEAT_PROFILE']
HERE = Path(__file__).resolve().parent

def run(name):
    u.log('PARADISE_SETUP: ' + name)
    runpy.run_path(str(HERE / name), run_name='__main__')

def require(path):
    obj = u.load_asset(path)
    if obj is None:
        raise RuntimeError('Required asset did not load: ' + path)
    return obj

if PROFILE == 'city':
    run('import-street-tree.py')
    run('import-city-lamps.py')
    run('create-lamp-materials.py')
    run('integrate-nathan.py')
    run('prepare-nathan-punch.py')
    run('inspect-car-door-geometry.py')
    run('build-car-entry-meshes.py')
    run('build-cockpit-controls.py')
    run('build-fitted-mirrors.py')
    run('create-cockpit-glass.py')
    run('create-live-mirror-material.py')
    # The exterior glass is an original project-authored material retained in Content.
    run('fix-cabin-glass-usage.py')
    for row in json.loads((HERE / 'window-overrides.json').read_text()):
        target = row['override'].split('.')[0]
        mat = u.load_asset(target) or u.EditorAssetLibrary.duplicate_asset(row['source'], target)
        if mat is None:
            raise RuntimeError('Cannot recreate building material: ' + target)
        for key, value in row['values'].items():
            u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mat, key, value)
        u.MaterialEditingLibrary.update_material_instance(mat)
        u.EditorAssetLibrary.save_loaded_asset(mat)
        day_path = '/Game/Environment/CityDistrict10x/DayMaterials/Day_' + mat.get_name()
        day = u.load_asset(day_path) or u.EditorAssetLibrary.duplicate_asset(target, day_path)
        for key, value in [('Exposure', -6), ('AmountOff', 1), ('BlindsBrightness', .15)]:
            u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(day, key, value)
        u.MaterialEditingLibrary.update_material_instance(day)
        u.EditorAssetLibrary.save_loaded_asset(day)
    for name in ['M_StreetLamp_Lens', 'M_StreetLamp_Emitter']:
        target = '/Game/Environment/CityDistrict10x/DayMaterials/Day_' + name
        day = u.load_asset(target) or u.EditorAssetLibrary.duplicate_asset('/Game/Environment/StreetLights/' + name, target)
        node = u.MaterialEditingLibrary.create_material_expression(day, u.MaterialExpressionConstant)
        node.set_editor_property('r', 0)
        u.MaterialEditingLibrary.connect_material_property(node, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
        u.MaterialEditingLibrary.recompile_material(day)
        u.EditorAssetLibrary.save_loaded_asset(day)
    run('fix-lamp-materials.py')
    for path in u.EditorAssetLibrary.list_assets('/Game/Environment/LeafLitter'):
        mat = u.load_asset(path)
        if isinstance(mat, u.Material):
            mat.set_editor_property('used_with_nanite', True)
            mat.set_editor_property('used_with_instanced_static_meshes', True)
            u.MaterialEditingLibrary.recompile_material(mat)
            u.EditorAssetLibrary.save_loaded_asset(mat)
    require('/Game/Vehicles/Player/Entry/SK_Sedan_Entry')
    require('/Game/Vehicles/Player/Entry/SK_Nathan_CockpitArms')
    player_path = '/Game/Characters/ScannedPlayer/BP_NathanPlayer'
    map_path = '/Game/Maps/CityDistrict10x_Day'
else:
    player_path = '/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter'
    map_path = '/Game/ThirdPerson/Lvl_ThirdPerson'

player = require(player_path)
game_mode = require('/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode')
u.get_default_object(game_mode.generated_class()).set_editor_property('default_pawn_class', player.generated_class())
u.EditorAssetLibrary.save_loaded_asset(game_mode)
config = ROOT / 'ParadiseHeat/Config/DefaultEngine.ini'
s = config.read_text()
for key in ['GameDefaultMap', 'EditorStartupMap']:
    s = re.sub(r'^' + key + r'=.*$', key + '=' + map_path + '.' + map_path.rsplit('/', 1)[1], s, flags=re.M)
config.write_text(s)
(ROOT / 'artifacts/setup' / (PROFILE + '.ready')).write_text('Prepared locally. Generated licensed assets must remain untracked.\n')
u.log('PARADISE_SETUP_COMPLETE: ' + PROFILE)
