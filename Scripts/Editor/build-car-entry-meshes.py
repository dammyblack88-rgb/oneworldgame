import os
from pathlib import Path
SOURCE_ROOT = Path(os.environ["PARADISE_HEAT_ROOT"])
"""Create reversible door/body derivatives. Never edit the purchased source meshes."""
import unreal as u
import json
from pathlib import Path

root=(SOURCE_ROOT/'artifacts/car-geometry')
specs={
 'Sedan': ([47984,24624,25999,28970,15465,12408,12607,59805,49044,48775,26070,48886,7684,49053,49083,49113,49143,47710], [88,-86,85]),
 'SUV': ([6736,24564,15867,15662,15970,15712,15516,15466,16001,25775], [82,-87,105]),
}
for kind,(selected,hinge) in specs.items():
 data=json.loads((root/(kind+'.json')).read_text())
 parent=list(range(len(data['vertices'])))
 def find(i):
  while parent[i]!=i: parent[i]=parent[parent[i]];i=parent[i]
  return i
 for a,b,c in data['triangles']:
  for j in (b,c):
   x,y=find(a),find(j)
   if x!=y:parent[y]=x
 door_ids=[i for i,t in enumerate(data['triangles']) if find(t[0]) in selected]
 assert len(door_ids)>3000,(kind,len(door_ids))
 source=u.load_asset(f'/Game/VehicleVarietyVol2/Skeletons/SK_{kind}_01a')
 dest=f'/Game/Vehicles/Player/Entry/SK_{kind}_Entry'
 target=u.load_asset(dest) or u.EditorAssetLibrary.duplicate_asset(source.get_path_name(),dest)
 body=u.DynamicMesh();door=u.DynamicMesh()
 for dynamic in (body,door):
  result=u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(source,dynamic,u.GeometryScriptCopyMeshFromAssetOptions(),u.GeometryScriptMeshReadLOD())
  assert result[1]==u.GeometryScriptOutcomePins.SUCCESS,result
 chosen=set(door_ids)
 for dynamic,ids in ((body,door_ids),(door,[i for i in range(len(data['triangles'])) if i not in chosen])):
  indices=u.GeometryScript_List.convert_array_to_index_list(ids)
  u.GeometryScript_MeshEdits.delete_triangles_from_mesh(dynamic,indices)
 result=u.GeometryScript_AssetUtils.copy_mesh_to_skeletal_mesh(body,target,u.GeometryScriptCopyMeshToAssetOptions(),u.GeometryScriptMeshWriteLOD())
 assert result[1]==u.GeometryScriptOutcomePins.SUCCESS,result
 u.EditorAssetLibrary.save_loaded_asset(target)
 u.GeometryScript_MeshTransforms.translate_mesh(door,u.Vector(*[-x for x in hinge]))
 path=f'/Game/Vehicles/Player/Entry/SM_{kind}_DriverDoor'
 mesh=u.load_asset(path)
 if not mesh:
  mesh,outcome=u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(door,path,u.GeometryScriptCreateNewStaticMeshAssetOptions(enable_collision=False))
  assert outcome==u.GeometryScriptOutcomePins.SUCCESS,outcome
 else:
  u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(door,mesh,u.GeometryScriptCopyMeshToAssetOptions(),u.GeometryScriptMeshWriteLOD())
 mesh.set_editor_property('static_materials',[u.StaticMaterial(material_interface=m.material_interface,material_slot_name=m.material_slot_name) for m in source.get_editor_property('materials')])
 u.EditorAssetLibrary.save_loaded_asset(mesh)
 print('ENTRY_MESH_READY',kind,len(door_ids),'hinge',hinge)
