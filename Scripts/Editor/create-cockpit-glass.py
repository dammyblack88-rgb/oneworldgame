import os
from pathlib import Path
SOURCE_ROOT = Path(os.environ["PARADISE_HEAT_ROOT"])
import unreal as u
path='/Game/Vehicles/Player/M_CockpitClearGlass'
material=u.load_asset(path) or u.AssetToolsHelpers.get_asset_tools().create_asset('M_CockpitClearGlass','/Game/Vehicles/Player',u.Material,u.MaterialFactoryNew())
material.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('two_sided',True)
u.MaterialEditingLibrary.delete_all_material_expressions(material)
opacity=u.MaterialEditingLibrary.create_material_expression(material,u.MaterialExpressionConstant)
opacity.set_editor_property('r',.025)
colour=u.MaterialEditingLibrary.create_material_expression(material,u.MaterialExpressionConstant3Vector)
colour.set_editor_property('constant',u.LinearColor(.10,.13,.15,1))
u.MaterialEditingLibrary.connect_material_property(opacity,'',u.MaterialProperty.MP_OPACITY)
u.MaterialEditingLibrary.connect_material_property(colour,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
u.MaterialEditingLibrary.recompile_material(material)
u.EditorAssetLibrary.save_loaded_asset(material)
print('COCKPIT_GLASS_SAVED')
