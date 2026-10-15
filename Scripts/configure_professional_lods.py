"""Generate a second skeletal LOD while keeping the original close-up geometry."""
from pathlib import Path
import json, unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
editor=unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
rows=[]
for name in [p['mesh'] for p in json.loads((root/'SourceMeshes/character_manifest.json').read_text(encoding='utf-8'))]:
    mesh=unreal.load_asset('/Game/Prototype/Characters/'+name)
    assert editor.regenerate_lod(mesh,2,False,False),name+' LOD generation failed'
    counts=[editor.get_num_verts(mesh,i) for i in range(2)]
    assert counts[0]>0 and 0<counts[1]<counts[0],(name,counts)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    rows.append({'mesh':name,'vertices_per_lod':counts})
(root/'Evidence/entrega10_lods.json').write_text(json.dumps({'passed':True,'meshes':rows},indent=2),encoding='utf-8')
unreal.log('CE09_CHARACTER_LODS_READY '+str(rows))
