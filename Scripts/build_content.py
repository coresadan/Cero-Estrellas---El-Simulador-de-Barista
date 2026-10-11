"""Run inside UE 5.8 Python. Generate ONLY this project's prototype assets."""
import os
import unreal

ASSETS = "/Game/Prototype"
unreal.EditorAssetLibrary.make_directory(ASSETS)
unreal.EditorAssetLibrary.make_directory("/Game/Maps")
tools = unreal.AssetToolsHelpers.get_asset_tools()

for name, unlit in [("M_Surface", False), ("M_Unlit", True), ("M_Liquid", False), ("M_Metal", False), ("M_Wood", False)]:
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
        rough.set_editor_property("r", 0.16 if name == "M_Liquid" else 0.24 if name == "M_Metal" else 0.52)
        unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        if name == "M_Wood":
            position = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1200, 0)
            grain_scale = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -1200, 180)
            grain_scale.set_editor_property('constant', unreal.LinearColor(.025, .35, .1, 1))
            stretch = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply, -1000, 0)
            noise = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionNoise, -800, 0)
            noise.set_editor_property('levels', 2)
            noise.set_editor_property('quality', 1)
            noise.set_editor_property('output_min', .65)
            noise.set_editor_property('output_max', 1.15)
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
for name in ['SM_Torso', 'SM_Head']:
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
unreal.log("CE_CONTENT_READY: cup, character silhouettes, five materials, eighteen boss voice clips, eleven sound effects and Cafeteria map")
