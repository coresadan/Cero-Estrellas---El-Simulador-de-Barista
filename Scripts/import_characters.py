"""UE Python: import the three original skinned FBX bodies and grip mesh."""
import unreal, os, json
root=unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
tools=unreal.AssetToolsHelpers.get_asset_tools()
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
for name in ['M_Fabric','M_Skin','M_Surface','M_Metal','M_Unlit']:
    mat=unreal.load_asset('/Game/Prototype/'+name)
    mat.set_editor_property('used_with_skeletal_mesh',True)
    unreal.MaterialEditingLibrary.recompile_material(mat); unreal.EditorAssetLibrary.save_loaded_asset(mat)
manifest=[]
for name in ['SK_Crypto','SK_Hipster','SK_Influencer','SM_GripHand']:
    skeletal=name.startswith('SK_')
    task=unreal.AssetImportTask(); task.filename=os.path.join(root,'SourceMeshes',name+('.fbx' if skeletal else '.obj'))
    task.destination_path='/Game/Prototype/Characters' if skeletal else '/Game/Prototype/Meshes'
    task.destination_name=name; task.automated=True; task.replace_existing=True; task.save=True
    opt=unreal.FbxImportUI(); opt.import_mesh=True; opt.import_materials=False; opt.import_textures=False
    opt.import_as_skeletal=skeletal; opt.automated_import_should_detect_type=False
    opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH if skeletal else unreal.FBXImportType.FBXIT_STATIC_MESH
    if skeletal:
        opt.import_animations=False; opt.create_physics_asset=False
        opt.skeletal_mesh_import_data.import_uniform_scale=1.0
    task.options=opt; tools.import_asset_tasks([task])
    assert task.imported_object_paths, name+' import failed'
    mesh=unreal.load_asset(task.destination_path+'/'+name)
    assert mesh, name+' missing'
    if skeletal:
        assert unreal.CECharacterAssets.prepare_skeleton(mesh), name+' skeleton not initialized'
        assert unreal.CECharacterAssets.prepare_physics(mesh), name+' articulated collision not initialized'
        slots=mesh.get_editor_property('materials')
        for slot in slots:
            material=str(slot.material_slot_name)
            base='M_Skin' if material=='Skin' else 'M_Surface' if material in ['Hair','Leather'] else 'M_Fabric'
            slot.material_interface=unreal.load_asset('/Game/Prototype/'+base)
        mesh.set_editor_property('materials',slots)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    manifest.append({'asset':mesh.get_path_name(),'skeletal':skeletal})
unreal.EditorAssetLibrary.save_directory('/Game/Prototype/Characters',only_if_is_dirty=False,recursive=True)
with open(os.path.join(root,'Evidence','entrega08_character_import.json'),'w',encoding='utf-8') as out: json.dump(manifest,out,indent=2)
unreal.log('CE_RIG_IMPORT_READY '+str(manifest))
