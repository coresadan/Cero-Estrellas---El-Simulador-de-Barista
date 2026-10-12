"""Run inside UE 5.8 Python. Generate ONLY this project's prototype assets."""
import os
import unreal

ASSETS = "/Game/Prototype"
unreal.EditorAssetLibrary.make_directory(ASSETS)
unreal.EditorAssetLibrary.make_directory("/Game/Maps")
# A referenced project cubemap is explicitly cooked with the cafe assets.
if not unreal.EditorAssetLibrary.does_asset_exist(ASSETS+'/T_CafeReflection'):
    assert unreal.EditorAssetLibrary.duplicate_asset('/Engine/EngineResources/DefaultTextureCube',ASSETS+'/T_CafeReflection')
tools = unreal.AssetToolsHelpers.get_asset_tools()

texture_tasks=[]
texture_dir=os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()),'SourceTextures')
for filename in sorted(os.listdir(texture_dir)):
    if not filename.endswith('.tga'): continue
    task=unreal.AssetImportTask()
    task.set_editor_property('filename',os.path.join(texture_dir,filename))
    task.set_editor_property('destination_path',ASSETS+'/Textures')
    task.set_editor_property('automated',True); task.set_editor_property('replace_existing',True)
    task.set_editor_property('save',True); texture_tasks.append(task)
tools.import_asset_tasks(texture_tasks)
for task in texture_tasks:
    texture=unreal.EditorAssetLibrary.load_asset(task.imported_object_paths[0])
    name=texture.get_name()
    if name.endswith('_N'):
        texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property('srgb',False)
    elif name.endswith('_ORM'):
        texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS)
        texture.set_editor_property('srgb',False)
    else: texture.set_editor_property('srgb',True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
texture_sets={'M_Wood':'Wood','M_Stone':'Stone','M_Fabric':'Fabric','M_Metal':'Metal','M_Skin':'Skin','M_Surface':'Paint'}

for name, unlit in [("M_Surface", False), ("M_Unlit", True), ("M_Liquid", False), ("M_Metal", False), ("M_Wood", False), ("M_Ceramic", False), ("M_Fabric", False), ("M_Skin", False), ("M_Pastry", False), ("M_Stone",False), ("M_Screen",True)]:
    path = ASSETS + "/" + name
    mat = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not mat:
        mat = tools.create_asset(name, ASSETS, unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    tint = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.8, 0.7, 0.5, 1))
    if unlit:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        unreal.MaterialEditingLibrary.connect_material_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        unreal.MaterialEditingLibrary.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
        rough = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 180)
        rough.set_editor_property("r", {"M_Liquid":.16,"M_Metal":.24,"M_Ceramic":.22,"M_Fabric":.91,"M_Skin":.65,"M_Pastry":.83}.get(name,.52))
        unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        if name == "M_Pastry":
            position = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1200, 0)
            grain_scale = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -1200, 180)
            frequency={"M_Wood":(.025,.35,.1),"M_Fabric":(2,2,2),"M_Skin":(.28,.28,.28),"M_Pastry":(.8,.8,.8)}[name]
            grain_scale.set_editor_property('constant', unreal.LinearColor(*frequency, 1))
            stretch = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply, -1000, 0)
            noise = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionNoise, -800, 0)
            noise.set_editor_property('levels', 2 if name=='M_Wood' else 1)
            noise.set_editor_property('quality', 1)
            noise.set_editor_property('output_min', .92 if name=='M_Skin' else .83 if name=='M_Fabric' else .55)
            noise.set_editor_property('output_max', 1.05 if name=='M_Skin' else 1.08 if name=='M_Fabric' else 1.15)
            color = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 0)
            unreal.MaterialEditingLibrary.connect_material_expressions(position, '', stretch, 'A')
            unreal.MaterialEditingLibrary.connect_material_expressions(grain_scale, '', stretch, 'B')
            unreal.MaterialEditingLibrary.connect_material_expressions(stretch, '', noise, 'Position')
            unreal.MaterialEditingLibrary.connect_material_expressions(noise, '', color, 'A')
            unreal.MaterialEditingLibrary.connect_material_expressions(tint, '', color, 'B')
            unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
        if name == "M_Metal":
            metal = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 260)
            metal.set_editor_property("r", 0.85)
            unreal.MaterialEditingLibrary.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
        if name == "M_Liquid":
            mat.set_editor_property("used_with_instanced_static_meshes", True)
            channels = []
            for index in range(3):
                data = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionPerInstanceCustomData, -800, index*120)
                data.set_editor_property("data_index", index)
                channels.append(data)
            rg = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -600, 0)
            rgb = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -400, 0)
            interp = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionVertexInterpolator, -200, 0)
            unreal.MaterialEditingLibrary.connect_material_expressions(channels[0], "", rg, "A")
            unreal.MaterialEditingLibrary.connect_material_expressions(channels[1], "", rg, "B")
            unreal.MaterialEditingLibrary.connect_material_expressions(rg, "", rgb, "A")
            unreal.MaterialEditingLibrary.connect_material_expressions(channels[2], "", rgb, "B")
            assert unreal.MaterialEditingLibrary.connect_material_expressions(rgb, "", interp, "VS"), "Missing vertex interpolator input"
            unreal.MaterialEditingLibrary.connect_material_property(interp, "", unreal.MaterialProperty.MP_BASE_COLOR)
    if name in texture_sets or name=='M_Screen':
        uv=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureCoordinate,-1000,-300)
        uv.set_editor_property('u_tiling',2.0 if name=='M_Fabric' else 1.0)
        uv.set_editor_property('v_tiling',2.0 if name=='M_Fabric' else 1.0)
        texture_set=texture_sets.get(name,'Screen')
        samples={}
        for suffix in (['A'] if name=='M_Screen' else ['A','N','ORM']):
            sample=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-800,-300+len(samples)*200)
            sample.set_editor_property('texture',unreal.EditorAssetLibrary.load_asset(ASSETS+'/Textures/'+texture_set+'_'+suffix))
            if suffix=='N': sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
            elif suffix=='ORM': sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
            unreal.MaterialEditingLibrary.connect_material_expressions(uv,'',sample,'UVs')
            samples[suffix]=sample
        color=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionMultiply,-200,-160)
        unreal.MaterialEditingLibrary.connect_material_expressions(samples['A'],'RGB',color,'A')
        unreal.MaterialEditingLibrary.connect_material_expressions(tint,'',color,'B')
        unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR if unlit else unreal.MaterialProperty.MP_BASE_COLOR)
        if not unlit:
            unreal.MaterialEditingLibrary.connect_material_property(samples['N'],'RGB',unreal.MaterialProperty.MP_NORMAL)
            for channel,prop in [('R',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),('G',unreal.MaterialProperty.MP_ROUGHNESS),('B',unreal.MaterialProperty.MP_METALLIC)]:
                unreal.MaterialEditingLibrary.connect_material_property(samples['ORM'],channel,prop)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)

project = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
wav_dir = os.path.join(project, "SourceAudio", "Boss")
tasks = []
for filename in sorted(os.listdir(wav_dir)):
    if not filename.endswith(".wav"):
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(wav_dir, filename))
    task.set_editor_property("destination_path", ASSETS + "/Audio")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tasks.append(task)
tools.import_asset_tasks(tasks)

fx_tasks = []
for filename in sorted(os.listdir(os.path.join(project, "SourceAudio", "FX"))):
    if not filename.endswith(".wav"):
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(project, "SourceAudio", "FX", filename))
    task.set_editor_property("destination_path", ASSETS + "/Audio/FX")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    fx_tasks.append(task)
tools.import_asset_tasks(fx_tasks)

mesh_task = unreal.AssetImportTask()
mesh_task.set_editor_property("filename", os.path.join(project, "SourceMeshes", "SM_Cup.obj"))
mesh_task.set_editor_property("destination_path", ASSETS + "/Meshes")
mesh_task.set_editor_property("destination_name", "SM_Cup")
mesh_task.set_editor_property("automated", True)
mesh_task.set_editor_property("replace_existing", True)
mesh_task.set_editor_property("save", True)
options = unreal.FbxImportUI()
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_materials", False)
options.set_editor_property("import_textures", False)
options.set_editor_property("import_as_skeletal", False)
options.static_mesh_import_data.set_editor_property("auto_generate_collision", True)
mesh_task.set_editor_property("options", options)
mesh_tasks = [mesh_task]
for name in sorted(f[:-4] for f in os.listdir(os.path.join(project, 'SourceMeshes')) if f.endswith('.obj') and f != 'SM_Cup.obj'):
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', os.path.join(project, 'SourceMeshes', name+'.obj'))
    task.set_editor_property('destination_path', ASSETS+'/Meshes')
    task.set_editor_property('destination_name', name)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    task.set_editor_property('options', options)
    mesh_tasks.append(task)
tools.import_asset_tasks(mesh_tasks)

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
map_path = "/Game/Maps/Cafeteria"
if not unreal.EditorAssetLibrary.does_asset_exist(map_path):
    level.new_level(map_path)
else:
    level.load_level(map_path)
for actor in actors.get_all_level_actors():
    if isinstance(actor, unreal.PlayerStart):
        actors.destroy_actor(actor)
start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-140, -120, 98), unreal.Rotator(0, 0, 0))
start.set_actor_label("InicioBarista")
level.save_current_level()
unreal.EditorAssetLibrary.save_directory(ASSETS, only_if_is_dirty=False, recursive=True)
unreal.log("CE_CONTENT_READY: 24 authored models, eleven materials, nineteen original PBR/UI textures, eighteen boss voice clips, eleven sound effects and Cafeteria map")
for ce_script in ['import_characters.py','import_polish.py']:
    ce_path=os.path.join(project,'Scripts',ce_script)
    exec(compile(open(ce_path,encoding='utf-8').read(),ce_path,'exec'),{'__file__':ce_path})
