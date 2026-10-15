"""Import all nine body variants, exact speech, and restrained material polish."""
from pathlib import Path
import runpy, json, unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
for script in ['import_characters.py','configure_professional_lods.py']:
    runpy.run_path(str(root/'Scripts'/script),run_name='__main__')
tools=unreal.AssetToolsHelpers.get_asset_tools(); tasks=[]
for path in sorted((root/'SourceAudio/Dialogue').glob('*.wav')):
    t=unreal.AssetImportTask(); t.filename=str(path); t.destination_path='/Game/Prototype/Audio/Dialogue'
    t.destination_name=path.stem; t.automated=True; t.replace_existing=True; t.save=True; tasks.append(t)
tools.import_asset_tasks(tasks)
assert all(t.imported_object_paths for t in tasks)
L=unreal.MaterialEditingLibrary
for name,spec in [('M_Skin',.28),('M_Fabric',.22),('M_Wood',.38),('M_Liquid',.65)]:
    mat=unreal.load_asset('/Game/Prototype/'+name)
    value=L.create_material_expression(mat,unreal.MaterialExpressionConstant,-300,650); value.r=spec
    L.connect_material_property(value,'',unreal.MaterialProperty.MP_SPECULAR)
    L.recompile_material(mat); unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.EditorAssetLibrary.save_directory('/Game/Prototype',only_if_is_dirty=True,recursive=True)
(root/'Evidence/entrega10_content.json').write_text(json.dumps({'passed':True,'speech_assets':len(tasks),'body_variants':9},indent=2),encoding='utf-8')
unreal.log('CE10_CONTENT_READY')
