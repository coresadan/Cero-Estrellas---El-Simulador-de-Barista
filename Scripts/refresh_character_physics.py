"""Refresh collision capsules without reimporting meshes or rebuilding materials."""
import unreal
for name in ['SK_Crypto','SK_Hipster','SK_Influencer']:
    mesh=unreal.load_asset('/Game/Prototype/Characters/'+name)
    assert unreal.CECharacterAssets.prepare_physics(mesh),name
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
unreal.log('CE_CHARACTER_PHYSICS_REFRESH_READY')
