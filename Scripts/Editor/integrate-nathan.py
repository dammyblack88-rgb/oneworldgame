import os
from pathlib import Path
SOURCE_ROOT = Path(os.environ["PARADISE_HEAT_ROOT"])
"""Retarget template locomotion and connect the accepted assembled Nathan to its player."""
import unreal as u
import json
from pathlib import Path
report={'status':'IN_PROGRESS'}
root='/Game/Characters/ScannedPlayer'
try:
    tools=u.AssetToolsHelpers.get_asset_tools()
    template=u.load_asset('/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter')
    template_cdo=u.get_default_object(template.generated_class())
    source_component=template_cdo.get_editor_property('mesh')
    source=source_component.get_editor_property('skeletal_mesh_asset')
    target=u.load_asset('/Game/Scanned3DPeoplePack/RP_Character/rp_nathan_rigged_003_ue4/rp_nathan_rigged_003_ue4')
    anim_class=source_component.get_editor_property('anim_class')
    source_anim=u.load_asset(anim_class.get_path_name().split('.')[0])
    report.update(source_mesh=source.get_path_name(),target_mesh=target.get_path_name(),source_animation=source_anim.get_path_name())
    def rig(name,mesh):
        asset=u.load_asset(root+'/'+name) or tools.create_asset(name,root,u.IKRigDefinition,u.IKRigDefinitionFactory())
        controller=u.IKRigController.get_controller(asset)
        if not controller.set_skeletal_mesh(mesh): raise RuntimeError('Cannot set mesh for '+name)
        if not controller.apply_auto_generated_retarget_definition(): raise RuntimeError('No retarget template for '+name)
        u.EditorAssetLibrary.save_loaded_asset(asset)
        return asset
    source_rig=rig('IK_PlayerSource',source)
    target_rig=rig('IK_Nathan',target)
    retarget=u.load_asset(root+'/RTG_PlayerToNathan') or tools.create_asset('RTG_PlayerToNathan',root,u.IKRetargeter,u.IKRetargetFactory())
    controller=u.IKRetargeterController.get_controller(retarget)
    controller.set_ik_rig(u.RetargetSourceOrTarget.SOURCE,source_rig)
    controller.set_ik_rig(u.RetargetSourceOrTarget.TARGET,target_rig)
    controller.set_preview_mesh(u.RetargetSourceOrTarget.SOURCE,source)
    controller.set_preview_mesh(u.RetargetSourceOrTarget.TARGET,target)
    controller.add_default_ops()
    controller.auto_map_chains(u.AutoMapChainType.FUZZY,True)
    controller.auto_align_all_bones(u.RetargetSourceOrTarget.TARGET)
    u.EditorAssetLibrary.save_loaded_asset(retarget)
    inputs=u.IKRetargetBatchOperationInputs()
    inputs.set_editor_properties({'assets_to_retarget':[u.EditorAssetLibrary.find_asset_data(source_anim.get_path_name())], 'source_mesh':source,'target_mesh':target,'ik_retarget_asset':retarget,'target_path':root+'/Animations','prefix':'Nathan_','include_referenced_assets':True,'overwrite_existing_files':False})
    results=u.IKRetargetBatchOperation.run_batch_retarget(inputs)
    report['retargeted']=[str(a.package_name) for a in results]
    new_anim=u.load_asset(root+'/Animations/Nathan_'+source_anim.get_name())
    for data in results:
        obj=data.get_asset()
        if isinstance(obj,u.AnimBlueprint) and obj.get_name()=='Nathan_'+source_anim.get_name(): new_anim=obj
    if not new_anim: raise RuntimeError('Retarget did not produce the player animation Blueprint')
    # Retargeting preserves mannequin foot Control Rig; its IK targets do not match Nathan.
    for graph in u.BlueprintEditorLibrary.list_graphs(new_anim):
        for node in u.ObjectIterator(u.AnimGraphNode_ControlRig):
            if node.get_outer()!=graph: continue
            pin=u.BlueprintEditorLibrary.find_input_pin(node,'Alpha')
            u.BlueprintGraphPinLibrary.break_pin_links(pin)
            if not u.BlueprintGraphPinLibrary.set_pin_value(pin,'0.0'): raise RuntimeError('Could not disable inherited mannequin foot IK')
    u.BlueprintEditorLibrary.compile_blueprint(new_anim)
    player=u.load_asset(root+'/BP_NathanPlayer') or u.EditorAssetLibrary.duplicate_asset(template.get_path_name(),root+'/BP_NathanPlayer')
    u.BlueprintEditorLibrary.reparent_blueprint(player,u.load_class(None,'/Script/ParadiseHeat.ParadiseHeatCharacter'))
    u.BlueprintEditorLibrary.compile_blueprint(player)
    cdo=u.get_default_object(player.generated_class())
    mesh=cdo.get_editor_property('mesh')
    mesh.set_skeletal_mesh_asset(target)
    mesh.set_anim_instance_class(new_anim.generated_class())
    half=cdo.get_editor_property('capsule_component').get_unscaled_capsule_half_height()
    mesh.set_relative_location(u.Vector(0,0,-half),False,False)
    mesh.set_relative_rotation(u.Rotator(yaw=-90),False,False)
    mesh.set_editor_property('override_materials',[])
    cdo.get_editor_property('character_movement').set_editor_property('max_walk_speed',300)
    u.EditorAssetLibrary.save_loaded_asset(player)
    gm=u.load_asset('/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode')
    gm_cdo=u.get_default_object(gm.generated_class())
    report['previous_pawn']=gm_cdo.get_editor_property('default_pawn_class').get_path_name()
    gm_cdo.set_editor_property('default_pawn_class',player.generated_class())
    u.EditorAssetLibrary.save_loaded_asset(gm)
    u.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
    report.update(status='ASSETS_CONNECTED_RUNTIME_PENDING',player=player.get_path_name(),animation=new_anim.get_path_name())
except Exception as exc:
    report.update(status='FAILED',error=str(exc))
    u.log_error('PARADISEHEAT_INTEGRATE '+str(exc)); raise
finally:
    (SOURCE_ROOT/'artifacts/setup/nathan-integration.json').write_text(json.dumps(report,indent=2))
    u.log('PARADISEHEAT_INTEGRATE '+json.dumps(report))
