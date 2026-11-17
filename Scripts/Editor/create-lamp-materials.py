import os
from pathlib import Path
SOURCE_ROOT = Path(os.environ["PARADISE_HEAT_ROOT"])
"""Replace only the 740 exact placeholder fixtures; preserve the road and building batches."""
import unreal as u
from pathlib import Path
import json, math
ROOT=SOURCE_ROOT;DEST='/Game/Environment/StreetLights'
mel=u.MaterialEditingLibrary;tools=u.AssetToolsHelpers.get_asset_tools()
def texture(name,normal=False,mask=False):
 t=u.load_asset(DEST+'/'+name);assert t,name
 if normal:t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP)
 if normal or mask:t.set_editor_property('srgb',False)
 u.EditorAssetLibrary.save_loaded_asset(t);return t
def sample(m,t):
 n=mel.create_material_expression(m,u.MaterialExpressionTextureSample);n.texture=t
 if t.compression_settings==u.TextureCompressionSettings.TC_NORMALMAP:n.sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL
 elif not t.srgb:n.sampler_type=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
 return n
def material(name,prefix=None):
 m=u.load_asset(DEST+'/'+name)
 if not m:m=tools.create_asset(name,DEST,u.Material,u.MaterialFactoryNew())
 mel.delete_all_material_expressions(m);m.set_editor_property('used_with_instanced_static_meshes',True)
 if prefix:
  base=sample(m,texture(prefix+'_BaseMap'));mel.connect_material_property(base,'RGB',u.MaterialProperty.MP_BASE_COLOR)
  normal=sample(m,texture(prefix+'_Normal',normal=True));mel.connect_material_property(normal,'RGB',u.MaterialProperty.MP_NORMAL)
  mask=sample(m,texture(prefix+'_MaskMap',mask=True))
  mel.connect_material_property(mask,'R',u.MaterialProperty.MP_METALLIC);mel.connect_material_property(mask,'G',u.MaterialProperty.MP_AMBIENT_OCCLUSION)
  inv=mel.create_material_expression(m,u.MaterialExpressionOneMinus);mel.connect_material_expressions(mask,'A',inv,'');mel.connect_material_property(inv,'',u.MaterialProperty.MP_ROUGHNESS)
 else:
  t=sample(m,texture('Light_Emissive'));mul=mel.create_material_expression(m,u.MaterialExpressionMultiply);mul.set_editor_property('const_b',2.0);mel.connect_material_expressions(t,'RGB',mul,'A');mel.connect_material_property(mul,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
  mel.connect_material_property(t,'RGB',u.MaterialProperty.MP_BASE_COLOR)
 mel.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m);return m
mats={'Street_Light_A':material('M_StreetLamp_Metal','Street_Light_A'),'Street_Light_Glass':material('M_StreetLamp_Lens','Street_Light_Glass'),'Light':material('M_StreetLamp_Emitter')}
info=json.loads((ROOT/'artifacts/setup/lamp-import.json').read_text())
for entry in info:
 mesh=u.load_asset(DEST+'/'+entry['name'])
 for i,slot in enumerate(mesh.static_materials):
  name=str(slot.material_slot_name);key=next((k for k in sorted(mats,key=len,reverse=True) if name.startswith(k)),None)
  assert key,(entry['name'],name);mesh.set_material(i,mats[key])
 u.EditorAssetLibrary.save_loaded_asset(mesh)
