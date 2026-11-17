import os
from pathlib import Path
SOURCE_ROOT = Path(os.environ["PARADISE_HEAT_ROOT"])
"""Map live mirror views onto polygons fitted to the original reflective glass."""
import json, math
from pathlib import Path
import unreal as u
root=SOURCE_ROOT
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def sub(a,b):return [x-y for x,y in zip(a,b)]
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def norm(a):return [x/math.sqrt(dot(a,a)) for x in a]
def hull(points):
 p=sorted(set(points));lo=[];hi=[]
 def turn(a,b,c):return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
 for a in p:
  while len(lo)>=2 and turn(lo[-2],lo[-1],a)<=0:lo.pop()
  lo.append(a)
 for a in reversed(p):
  while len(hi)>=2 and turn(hi[-2],hi[-1],a)<=0:hi.pop()
  hi.append(a)
 return lo[:-1]+hi[:-1]
report=[]
for kind in ['Sedan','SUV','BoxTruck']:
 data=json.loads((root/f'artifacts/car-geometry/{kind}.json').read_text());v=data['vertices'];ts=data['triangles']
 centers={'Sedan':[(34.2,-4.7,127.6),(59.5,-98,100),(59.5,98,100)],'SUV':[(36,0,153),(54.7,-103,122),(54.7,103,122)],'BoxTruck':[(0,0,0),(162,-126,224),(162,126,224)]}[kind]
 for index in range(0 if kind=='Sedan' else 1,3):
  center=centers[index];n=norm([.95,.3,0] if index==0 else [.96,-.28 if index==1 else .28,0]);right=norm(cross([0,0,1],n));up=[0,0,1]
  if kind!='Sedan':n=[1,0,0];right=[0,1,0]
  if kind=='BoxTruck' and index==1:n=[.6,-.8,0];right=norm(cross([0,0,1],n))
  parent=list(range(len(v)))
  def find(i):
   while parent[i]!=i:parent[i]=parent[parent[i]];i=parent[i]
   return i
  for tri in ts:
   for j in tri[1:]:parent[find(j)]=find(tri[0])
  group={'Sedan':[64174,12408,4003],'SUV':[0,15867,25965],'BoxTruck':[0,8107,1632]}[kind][index]
  ids=[]
  for i,t in enumerate(ts):
   if find(t[0])!=group:continue
   a,b,c=[v[j] for j in t];q=[(a[k]+b[k]+c[k])/3 for k in range(3)];d=sub(q,center)
   if abs(d[0])>12 or abs(d[1])>(11 if index==0 else 13) or abs(d[2])>(5 if index==0 else 18 if kind=='BoxTruck' else 9):continue
   if kind=='Sedan' and index>0 and (abs(q[1])<90 or q[2]<94):continue
   tn=cross(sub(b,a),sub(c,a));length=math.sqrt(dot(tn,tn))
   if length and dot(tn,n)/length>(.995 if kind=="Sedan" else .98):ids.append(i)
  assert len(ids)>=2,(kind,index,len(ids))
  pts=[v[j] for i in ids for j in ts[i]];outline=hull([(dot(p,right),p[2]) for p in pts]);depth=sum(dot(p,n) for p in pts)/len(pts)-.08
  xmin=min(p[0] for p in outline);xmax=max(p[0] for p in outline);ymin=min(p[1] for p in outline);ymax=max(p[1] for p in outline)
  # Mirror glass itself defines the silhouette; no rectangular plane remains.
  # Keep the actual curved glass triangles and offset by 1 mm, rather than
  # flattening a polygon through the existing curved glass.
  selected_vertices=sorted(set(j for i in ids for j in ts[i]));remap={j:k+1 for k,j in enumerate(selected_vertices)}
  lines=[]
  for j in selected_vertices:
   pos=[v[j][k]-n[k]*.12 for k in range(3)];pos[1]=-pos[1]
   lines.append('v '+' '.join(str(z) for z in pos))
  for j in selected_vertices:
   x=dot(v[j],right);y=v[j][2];lines.append(f'vt {(x-xmin)/(xmax-xmin)} {(y-ymin)/(ymax-ymin)}')
  for i in ids:lines.append('f '+' '.join(f'{remap[j]}/{remap[j]}' for j in ts[i]))
  obj=root/f'artifacts/car-geometry/{kind}_Mirror{index}.obj';obj.write_text('\n'.join(lines)+'\n')
  task=u.AssetImportTask();task.filename=str(obj);task.destination_path='/Game/Vehicles/Player/Entry';task.destination_name=f'SM_{kind}_Mirror{index}';task.automated=True;task.replace_existing=True;task.save=True
  options=u.FbxImportUI();options.import_mesh=True;options.import_materials=False;options.import_textures=False;options.import_as_skeletal=False;options.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
  options.static_mesh_import_data.set_editor_property('convert_scene',False);options.static_mesh_import_data.set_editor_property('auto_generate_collision',False);task.options=options
  u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);assert task.imported_object_paths,(kind,index)
  asset=u.load_asset(task.imported_object_paths[0]);print('MIRROR_BOUNDS',kind,index,asset.get_bounding_box())
  report.append({'kind':kind,'mirror':index,'triangles':len(ids),'outline_vertices':len(outline),'width':xmax-xmin,'height':ymax-ymin,'depth':depth})
(root/'artifacts/car-geometry/fitted-mirrors.json').write_text(json.dumps(report,indent=2));print('FITTED_MIRRORS_READY',report)
