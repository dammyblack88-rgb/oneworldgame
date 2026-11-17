import os
from pathlib import Path
SOURCE_ROOT = Path(os.environ["PARADISE_HEAT_ROOT"])
"""Import one CC0 tree with bounded textures; run inside the map-authoring process."""
import unreal as u
import json
from pathlib import Path
SRC=(SOURCE_ROOT/'SourceAssets/PolyHaven/tree_small_02')
DEST='/Game/Environment/StreetTrees'
INFO=(SOURCE_ROOT/'artifacts/setup/street-tree-import.json')
TREE=None
if INFO.exists():
 TREE=u.load_asset(json.loads(INFO.read_text())['mesh'])
if not TREE:
 data=json.loads((SRC/'tree_small_02_1k.gltf').read_text())
 for m in data['materials']:
  if m.get('alphaMode')=='BLEND':m['alphaMode']='MASK';m['alphaCutoff']=.4
 (SRC/'StreetTree.gltf').write_text(json.dumps(data))
 task=u.AssetImportTask();task.set_editor_property('filename',str(SRC/'StreetTree.gltf'));task.set_editor_property('destination_path',DEST);task.set_editor_property('automated',True);task.set_editor_property('save',True);task.set_editor_property('replace_existing',True)
 u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
 objects=task.get_objects();meshes=[a for a in objects if isinstance(a,u.StaticMesh)]
 if not meshes:
  meshes=[u.load_asset(p) for p in task.get_editor_property('imported_object_paths') if isinstance(u.load_asset(p),u.StaticMesh)]
 assert meshes,'Tree importer produced no StaticMesh'
 TREE=meshes[0]
 settings=TREE.get_editor_property('nanite_settings');settings.set_editor_property('enabled',True);TREE.set_editor_property('nanite_settings',settings)
 # Explicit alpha mask: the source glTF's JPEG base color has no alpha channel.
 alpha=u.AssetImportTask();alpha.set_editor_property('filename',str(SRC/'textures/leaves_alpha_1k.png'));alpha.set_editor_property('destination_path',DEST);alpha.set_editor_property('automated',True);alpha.set_editor_property('save',True)
 u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([alpha]);at=alpha.get_objects()[0];at.set_editor_property('srgb',False)
 tex=None
 for a in objects:
  if isinstance(a,u.Texture2D) and 'leaves_diff' in a.get_name():tex=a
 if not tex:
  t=u.AssetImportTask();t.set_editor_property('filename',str(SRC/'textures/tree_small_02_leaves_diff_1k.jpg'));t.set_editor_property('destination_path',DEST);t.set_editor_property('automated',True);t.set_editor_property('save',True);u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t]);tex=t.get_objects()[0]
 m=u.AssetToolsHelpers.get_asset_tools().create_asset('M_StreetLeaves',DEST,u.Material,u.MaterialFactoryNew());m.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED);m.set_editor_property('two_sided',True);m.set_editor_property('used_with_instanced_static_meshes',True)
 mel=u.MaterialEditingLibrary
 for texture,prop,channel in [(tex,u.MaterialProperty.MP_BASE_COLOR,'RGB'),(at,u.MaterialProperty.MP_OPACITY_MASK,'R')]:
  n=mel.create_material_expression(m,u.MaterialExpressionTextureSample);n.set_editor_property('texture',texture);mel.connect_material_property(n,channel,prop)
 n=mel.create_material_expression(m,u.MaterialExpressionConstant);n.set_editor_property('r',.85);mel.connect_material_property(n,'',u.MaterialProperty.MP_ROUGHNESS);mel.recompile_material(m)
 slots=[]
 for i,slot in enumerate(TREE.get_editor_property('static_materials')):
  name=str(slot.get_editor_property('material_slot_name'));slots.append(name)
  if 'leav' in name.lower():TREE.set_material(i,m)
 for a in [at,m,TREE]:u.EditorAssetLibrary.save_loaded_asset(a)
 bounds=TREE.get_bounds()
 INFO.write_text(json.dumps({'mesh':TREE.get_path_name(),'source':'https://polyhaven.com/a/tree_small_02','license':'CC0','author':'Rico Cilliers','nanite':True,'slots':slots,'bounds_extent':[bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z]},indent=2)+'\n')
 u.log('STREET_TREE_IMPORT_PASS '+TREE.get_path_name())
# Imported Interchange instances inherit engine parents without Nanite usage.
# Give this asset project-local parent copies; never modify engine content.
for i,slot in enumerate(TREE.get_editor_property('static_materials')):
 material=TREE.get_material(i)
 if isinstance(material,u.MaterialInstanceConstant):
  base=material.get_base_material()
  local=DEST+'/M_TreeParent_'+str(i)
  if not base.get_path_name().startswith(DEST):
   target=u.load_asset(local)
   if not target:target=u.EditorAssetLibrary.duplicate_asset(base.get_path_name(),local)
   assert target,local
   u.MaterialEditingLibrary.set_material_instance_parent(material,target)
   base=target
  base.set_editor_property('used_with_nanite',True);base.set_editor_property('used_with_instanced_static_meshes',True)
  u.MaterialEditingLibrary.recompile_material(base);u.EditorAssetLibrary.save_loaded_asset(base)
  u.MaterialEditingLibrary.update_material_instance(material);u.EditorAssetLibrary.save_loaded_asset(material)
 else:
  material.set_editor_property('used_with_nanite',True);u.MaterialEditingLibrary.recompile_material(material);u.EditorAssetLibrary.save_loaded_asset(material)
u.EditorAssetLibrary.save_loaded_asset(TREE)
u.log('STREET_TREE_MATERIAL_USAGE_SAVED')
