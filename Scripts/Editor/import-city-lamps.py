import os
from pathlib import Path
SOURCE_ROOT = Path(os.environ["PARADISE_HEAT_ROOT"])
import unreal as u
from pathlib import Path
import json
ROOT=SOURCE_ROOT; DEST='/Game/Environment/StreetLights'
tools=u.AssetToolsHelpers.get_asset_tools(); mel=u.MaterialEditingLibrary
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
assets={}
for file in sorted((ROOT/'SourceAssets/Street_Lights_Pack_01_Textures').glob('*.png'))+sorted((ROOT/'SourceAssets/Street_Lights_Pack_01_fbx').glob('*.fbx')):
 a=u.load_asset(DEST+'/'+file.stem)
 if not a:
  task=u.AssetImportTask();task.filename=str(file);task.destination_path=DEST;task.automated=True;task.save=True
  if file.suffix=='.fbx':
   opt=u.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
   opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=True
   task.options=opt
  tools.import_asset_tasks([task]);a=u.load_asset(DEST+'/'+file.stem)
 assert a,str(file)
 assets[file.stem]=a
report=[]
for name,a in assets.items():
 if isinstance(a,u.StaticMesh):
  b=a.get_bounds();report.append({'name':name,'origin':[b.origin.x,b.origin.y,b.origin.z],'extent':[b.box_extent.x,b.box_extent.y,b.box_extent.z],'slots':[str(s.material_slot_name) for s in a.static_materials]})
(ROOT/'artifacts/setup/lamp-import.json').write_text(json.dumps(report,indent=2))
u.log('LAMP_IMPORT_COMPLETE '+json.dumps(report))
