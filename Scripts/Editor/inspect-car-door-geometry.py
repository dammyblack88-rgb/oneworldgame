import os
from pathlib import Path
SOURCE_ROOT = Path(os.environ["PARADISE_HEAT_ROOT"])
import unreal as u,json
from pathlib import Path
out=(SOURCE_ROOT/'artifacts/car-geometry');out.mkdir(exist_ok=True)
asset=u.GeometryScript_AssetUtils if hasattr(u,'GeometryScript_AssetUtils') else u.GeometryScriptLibrary_StaticMeshFunctions
query=u.GeometryScript_MeshQueries
lists=u.GeometryScript_List
for kind in ('Sedan','SUV','BoxTruck'):
    source=u.load_asset(f'/Game/VehicleVarietyVol2/Skeletons/SK_{kind}_01a')
    dynamic=u.DynamicMesh()
    result=asset.copy_mesh_from_skeletal_mesh(source,dynamic,u.GeometryScriptCopyMeshFromAssetOptions(),u.GeometryScriptMeshReadLOD())
    print('COPY_RESULT',kind,str(result))
    positions=query.get_all_vertex_positions(dynamic,False)
    triangles=query.get_all_triangle_indices(dynamic,False)
    print('QUERY_TYPES',str(type(positions)),str(type(triangles)))
    verts=lists.convert_vector_list_to_array(positions[1] if isinstance(positions[0],u.DynamicMesh) else positions[0])
    faces=lists.convert_triangle_list_to_array(triangles[1] if isinstance(triangles[0],u.DynamicMesh) else triangles[0])
    row={'vertices':[[v.x,v.y,v.z] for v in verts],'triangles':[[t.x,t.y,t.z] for t in faces]}
    (out/(kind+'.json')).write_text(json.dumps(row))
    print('GEOMETRY_DUMPED',kind,len(verts),len(faces))
