import os
from pathlib import Path
SOURCE_ROOT = Path(os.environ["PARADISE_HEAT_ROOT"])
import unreal as u
for path in ['/Game/Vehicles/Player/M_ExteriorClearGlass','/Game/Vehicles/Player/M_CockpitClearGlass']:
 m=u.load_asset(path)
 assert m,path
 m.set_editor_property('used_with_skeletal_mesh',True)
 u.MaterialEditingLibrary.recompile_material(m)
 assert u.EditorAssetLibrary.save_loaded_asset(m)
 u.log('CABIN_GLASS_FIXED '+path)
