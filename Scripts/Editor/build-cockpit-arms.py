import os
from pathlib import Path
SOURCE_ROOT = Path(os.environ["PARADISE_HEAT_ROOT"])
import unreal as u
source=u.load_asset('/Game/Scanned3DPeoplePack/RP_Character/rp_nathan_rigged_003_ue4/rp_nathan_rigged_003_ue4')
mesh=u.DynamicMesh()
u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(source,mesh,u.GeometryScriptCopyMeshFromAssetOptions(),u.GeometryScriptMeshReadLOD())
bones=u.GeometryScript_BoneWeights.get_all_bones_info(mesh)[1]
arm_bones=set()
for bone in bones:
 if any(str(bone.name).startswith(s) for s in ('lowerarm','hand','thumb','index','middle','ring','pinky')):arm_bones.add(bone.index)
triangles=u.GeometryScript_List.convert_triangle_list_to_array(u.GeometryScript_MeshQueries.get_all_triangle_indices(mesh,False)[1])
vertices={int(i) for t in triangles for i in (t.x,t.y,t.z)}
keep=set()
for i in vertices:
 _,weight,valid=u.GeometryScript_BoneWeights.get_largest_vertex_bone_weight(mesh,i)
 if valid and weight.bone_index in arm_bones:keep.add(i)
remove=[i for i,t in enumerate(triangles) if not all(v in keep for v in (t.x,t.y,t.z))]
assert len(triangles)-len(remove)>1000
u.GeometryScript_MeshEdits.delete_triangles_from_mesh(mesh,u.GeometryScript_List.convert_array_to_index_list(remove))
path='/Game/Vehicles/Player/Entry/SK_Nathan_CockpitArms'
target=u.load_asset(path) or u.EditorAssetLibrary.duplicate_asset(source.get_path_name(),path)
_,outcome=u.GeometryScript_AssetUtils.copy_mesh_to_skeletal_mesh(mesh,target,u.GeometryScriptCopyMeshToAssetOptions(),u.GeometryScriptMeshWriteLOD())
assert outcome==u.GeometryScriptOutcomePins.SUCCESS
u.EditorAssetLibrary.save_loaded_asset(target)
print('COCKPIT_ARMS_READY',len(triangles)-len(remove))
