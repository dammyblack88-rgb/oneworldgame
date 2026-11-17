import os
from pathlib import Path
SOURCE_ROOT = Path(os.environ["PARADISE_HEAT_ROOT"])
import unreal as u
root='/Game/Characters/ScannedPlayer'
inputs=u.IKRetargetBatchOperationInputs()
inputs.set_editor_properties({'assets_to_retarget':[u.EditorAssetLibrary.find_asset_data('/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01')],'source_mesh':u.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny'),'target_mesh':u.load_asset('/Game/Scanned3DPeoplePack/RP_Character/rp_nathan_rigged_003_ue4/rp_nathan_rigged_003_ue4'),'ik_retarget_asset':u.load_asset(root+'/RTG_PlayerToNathan'),'target_path':root+'/Animations','prefix':'Nathan_','include_referenced_assets':True,'overwrite_existing_files':False})
results=u.IKRetargetBatchOperation.run_batch_retarget(inputs)
assert results
for a in results:u.EditorAssetLibrary.save_loaded_asset(a.get_asset())
u.log('PUNCH_READY '+str([str(a.package_name) for a in results]))
