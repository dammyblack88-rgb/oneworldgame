import unreal as u
mel=u.MaterialEditingLibrary
for path,prefix in [('/Game/Environment/StreetLights/M_StreetLamp_Metal','Street_Light_A'),('/Game/Environment/StreetLights/M_StreetLamp_Lens','Street_Light_Glass'),('/Game/Environment/CityDistrict10x/DayMaterials/Day_M_StreetLamp_Lens','Street_Light_Glass')]:
 m=u.load_asset(path);assert m,path
 t=u.load_asset('/Game/Environment/StreetLights/'+prefix+'_MaskMap');assert t
 node=mel.create_material_expression(m,u.MaterialExpressionTextureSample)
 node.texture=t;node.sampler_type=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
 inv=mel.create_material_expression(m,u.MaterialExpressionOneMinus)
 assert mel.connect_material_expressions(node,'A',inv,''),path
 assert mel.connect_material_property(inv,'',u.MaterialProperty.MP_ROUGHNESS),path
 mel.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m,False)
 print('SHIPPING_LAMP_FIXED',path)
