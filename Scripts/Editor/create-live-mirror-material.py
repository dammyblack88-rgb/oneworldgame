import os
from pathlib import Path
SOURCE_ROOT = Path(os.environ["PARADISE_HEAT_ROOT"])
import unreal as u
path='/Game/Vehicles/Player/M_LiveCarMirror'
m=u.load_asset(path) or u.AssetToolsHelpers.get_asset_tools().create_asset('M_LiveCarMirror','/Game/Vehicles/Player',u.Material,u.MaterialFactoryNew())
m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property('two_sided',True)
e=u.MaterialEditingLibrary;e.delete_all_material_expressions(m)
tex=e.create_material_expression(m,u.MaterialExpressionTextureSampleParameter2D);tex.set_editor_property('parameter_name','MirrorView');tex.set_editor_property('texture',u.load_asset('/Engine/EngineResources/WhiteSquareTexture'))
uv=e.create_material_expression(m,u.MaterialExpressionTextureCoordinate)
scale=e.create_material_expression(m,u.MaterialExpressionConstant2Vector);scale.set_editor_property('r',-1);scale.set_editor_property('g',1)
mul=e.create_material_expression(m,u.MaterialExpressionMultiply)
offset=e.create_material_expression(m,u.MaterialExpressionConstant2Vector);offset.set_editor_property('r',1);offset.set_editor_property('g',0)
add=e.create_material_expression(m,u.MaterialExpressionAdd)
e.connect_material_expressions(uv,'',mul,'A');e.connect_material_expressions(scale,'',mul,'B')
e.connect_material_expressions(mul,'',add,'A');e.connect_material_expressions(offset,'',add,'B');e.connect_material_expressions(add,'',tex,'UVs')
inv=e.create_material_expression(m,u.MaterialExpressionEyeAdaptationInverse)
print('INVERSE_INPUTS',e.get_material_expression_input_names(inv))
e.connect_material_expressions(tex,'RGB',inv,e.get_material_expression_input_names(inv)[0])
e.connect_material_property(inv,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
e.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m)
print('LIVE_MIRROR_MATERIAL_READY')
