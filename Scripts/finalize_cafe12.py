"""Persist shader usage required by the baked set, and audit the saved map."""
from pathlib import Path
import json,unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
rows=[]
for name in ['M_Surface','M_Wood','M_Metal','M_Stone','M_Ceramic','M_Fabric','M_Unlit']:
    mat=unreal.load_asset('/Game/Prototype/'+name)
    previous=mat.get_editor_property('used_with_static_lighting')
    if not previous:
        mat.set_editor_property('used_with_static_lighting',True)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mat)
    rows.append({'material':name,'previous_static_lighting_usage':previous,'saved_static_lighting_usage':mat.get_editor_property('used_with_static_lighting')})
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/Maps/Cafeteria')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
interactive=[a.get_name() for a in actors if isinstance(a,(unreal.CEProp,unreal.CEStation,unreal.CECustomer,unreal.CECafeDoor))]
assert not interactive,interactive
fixed=[a for a in actors if 'CEArchitecture' in [str(t) for t in a.tags]]
assert len(fixed)>600,len(fixed)
assert (root/'Content/Maps/Cafeteria_BuiltData.uasset').stat().st_size>1_000_000
(root/'Evidence/entrega12_saved_scene_audit.json').write_text(json.dumps({'passed':True,'fixed_actors':len(fixed),'interactive_actors_baked':interactive,'materials':rows},indent=2))
unreal.log('CE12_SAVED_SCENE_AUDIT_PASS')
