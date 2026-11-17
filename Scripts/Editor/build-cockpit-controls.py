import os
from pathlib import Path
SOURCE_ROOT = Path(os.environ["PARADISE_HEAT_ROOT"])
import unreal as u
import json
from pathlib import Path
root=SOURCE_ROOT
exec((root/'Scripts/Editor/build-cockpit-arms.py').read_text())
for kind in ('Sedan','SUV','BoxTruck'):
 source=u.load_asset(f'/Game/VehicleVarietyVol2/Skeletons/SK_{kind}_01a')
 mesh=u.DynamicMesh()
 u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(source,mesh,u.GeometryScriptCopyMeshFromAssetOptions(),u.GeometryScriptMeshReadLOD())
 _,valid,bone=u.GeometryScript_BoneWeights.get_bone_info(mesh,'SteeringWheel')
 assert valid
 triangles=u.GeometryScript_List.convert_triangle_list_to_array(u.GeometryScript_MeshQueries.get_all_triangle_indices(mesh,False)[1])
 data=json.loads((root/f'artifacts/car-geometry/{kind}.json').read_text())
 parent=list(range(len(data['vertices'])))
 def find(i):
  while parent[i]!=i:parent[i]=parent[parent[i]];i=parent[i]
  return i
 for a,b,c in data['triangles']:
  for j in (b,c):
   x,y=find(a),find(j)
   if x!=y:parent[y]=x
 wheel_groups={'Sedan':[63916],'SUV':[10276],'BoxTruck':[49905,46642]}[kind]
 wheel_ids=[i for i,t in enumerate(data['triangles']) if find(t[0]) in wheel_groups]
 chosen=set(wheel_ids)
 remove=[i for i in range(len(triangles)) if i not in chosen]
 assert len(triangles)-len(remove)>100
 u.GeometryScript_MeshEdits.delete_triangles_from_mesh(mesh,u.GeometryScript_List.convert_array_to_index_list(remove))
 center=bone.world_transform.translation
 u.GeometryScript_MeshTransforms.translate_mesh(mesh,u.Vector(-center.x,-center.y,-center.z))
 path=f'/Game/Vehicles/Player/Entry/SM_{kind}_SteeringWheel'
 wheel=u.load_asset(path)
 if not wheel:
  wheel,outcome=u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(mesh,path,u.GeometryScriptCreateNewStaticMeshAssetOptions(enable_collision=False))
 else:
  u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(mesh,wheel,u.GeometryScriptCopyMeshToAssetOptions(),u.GeometryScriptMeshWriteLOD())
 wheel.set_editor_property('static_materials',[u.StaticMaterial(material_interface=m.material_interface,material_slot_name=m.material_slot_name) for m in source.get_editor_property('materials')])
 u.EditorAssetLibrary.save_loaded_asset(wheel)
 print('STEERING_WHEEL_READY',kind,center,bone.world_transform.rotation,len(triangles)-len(remove))
 # Preserve source, but remove the baked fixed wheel from the interactive body.
 body=u.DynamicMesh()
 u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(source,body,u.GeometryScriptCopyMeshFromAssetOptions(),u.GeometryScriptMeshReadLOD())
 door_groups={'Sedan':[47984,24624,25999,28970,15465,12408,12607,59805,49044,48775,26070,48886,7684,49053,49083,49113,49143,47710],'SUV':[6736,24564,15867,15662,15970,15712,15516,15516,15466,16001,25775],'BoxTruck':[]}[kind]
 cut=[i for i,t in enumerate(data['triangles']) if i in chosen or find(t[0]) in door_groups]
 u.GeometryScript_MeshEdits.delete_triangles_from_mesh(body,u.GeometryScript_List.convert_array_to_index_list(cut))
 body_path=f'/Game/Vehicles/Player/Entry/SK_{kind}_Entry'
 body_asset=u.load_asset(body_path) or u.EditorAssetLibrary.duplicate_asset(source.get_path_name(),body_path)
 _,outcome=u.GeometryScript_AssetUtils.copy_mesh_to_skeletal_mesh(body,body_asset,u.GeometryScriptCopyMeshToAssetOptions(),u.GeometryScriptMeshWriteLOD())
 assert outcome==u.GeometryScriptOutcomePins.SUCCESS
 u.EditorAssetLibrary.save_loaded_asset(body_asset)
