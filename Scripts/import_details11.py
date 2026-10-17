from pathlib import Path
import unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
tasks=[]
for name in ['SM11_Puddle','SM11_Brow']:
    t=unreal.AssetImportTask();t.filename=str(root/'SourceMeshes'/(name+'.obj'));t.destination_path='/Game/Prototype/Meshes'
    t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
    o=unreal.FbxImportUI();o.import_mesh=True;o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False
    o.automated_import_should_detect_type=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    o.static_mesh_import_data.auto_generate_collision=False;t.options=o;tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
assert all(t.imported_object_paths for t in tasks)
unreal.log('CE11_DETAILS_IMPORTED')
