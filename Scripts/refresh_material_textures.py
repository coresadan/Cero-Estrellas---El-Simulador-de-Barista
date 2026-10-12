"""UE commandlet: refresh only original texture maps without rebuilding mesh assets."""
import os
import unreal
root=unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
tools=unreal.AssetToolsHelpers.get_asset_tools()
tasks=[]
for name in sorted(os.listdir(os.path.join(root,'SourceTextures'))):
    if not name.endswith('.tga'): continue
    task=unreal.AssetImportTask()
    task.set_editor_property('filename',os.path.join(root,'SourceTextures',name))
    task.set_editor_property('destination_path','/Game/Prototype/Textures')
    task.set_editor_property('automated',True)
    task.set_editor_property('replace_existing',True)
    task.set_editor_property('save',True)
    tasks.append(task)
tools.import_asset_tasks(tasks)
for task in tasks:
    assert task.imported_object_paths, 'Texture import failed: '+task.filename
    texture=unreal.EditorAssetLibrary.load_asset(task.imported_object_paths[0])
    name=texture.get_name()
    texture.set_editor_property('srgb',not name.endswith(('_N','_ORM')))
    if name.endswith('_N'): texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
    elif name.endswith('_ORM'): texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log('CE_TEXTURE_REFRESH_READY: nineteen original textures')
