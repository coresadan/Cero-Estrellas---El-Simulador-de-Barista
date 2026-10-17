"""Persist fixed architecture, durable material instances, lighting and captures.

Interactive machines, chairs, door, customers and liquids are runtime actors.
Re-running deliberately regenerates CEArchitecture actors only.
"""
from pathlib import Path
import json, hashlib, unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
tools=unreal.AssetToolsHelpers.get_asset_tools()
assert levels.load_level('/Game/Maps/Cafeteria')
for actor in actors.get_all_level_actors():
    if 'CEArchitecture' in [str(t) for t in actor.tags]: actors.destroy_actor(actor)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert unreal.CECharacterAssets.author_cafe(world)
materials={};static_meshes={};fixed=[];lights=[]
for actor in actors.get_all_level_actors():
    if 'CEArchitecture' not in [str(t) for t in actor.tags]:continue
    actor.set_actor_label(next((str(t) for t in actor.tags if str(t).startswith('CE12_Block_') or str(t).startswith('CE12_Sign_')),actor.get_name()))
    actor.set_folder_path('Cafeteria12/Decorado' if not isinstance(actor,unreal.Light) else 'Cafeteria12/Luces')
    fixed.append(actor)
    for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
        mesh=comp.static_mesh
        static_meshes[mesh.get_path_name()]=mesh
        # Emissive panels and little decorative fittings do not block window light.
        unlit=False
        for i in range(comp.get_num_materials()):
            material=comp.get_material(i)
            if not isinstance(material,unreal.MaterialInstanceDynamic):continue
            parent=material.get_editor_property('parent')
            color=material.get_vector_parameter_value('Tint')
            key=parent.get_path_name()+str((color.r,color.g,color.b,color.a))
            name='MI12_'+hashlib.sha256(key.encode()).hexdigest()[:16]
            if key not in materials:
                path='/Game/Prototype/SetMaterials/'+name
                mi=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset(name,'/Game/Prototype/SetMaterials',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
                unreal.MaterialEditingLibrary.set_material_instance_parent(mi,parent)
                unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi,'Tint',color)
                unreal.EditorAssetLibrary.save_loaded_asset(mi);materials[key]=mi
            comp.set_material(i,materials[key]);unlit|=parent.get_name()=='M_Unlit'
        comp.set_mobility(unreal.ComponentMobility.STATIC)
        # Very small trim needs fewer texels than a wall/tabletop.
        extent=comp.get_local_bounds()[1]-comp.get_local_bounds()[0]
        size=max(abs(extent.x*comp.get_world_scale().x),abs(extent.y*comp.get_world_scale().y),abs(extent.z*comp.get_world_scale().z))
        comp.set_editor_property('override_light_map_res',True)
        comp.set_editor_property('overridden_light_map_res',256 if size>650 else 128 if size>180 else 64 if size>65 else 32)
        if unlit:comp.set_cast_shadow(False)
    for comp in actor.get_components_by_class(unreal.LightComponent):
        # Stationary sun supplies moving-object shadows; indirect light is baked.
        table_lamp=isinstance(actor,unreal.PointLight) and abs(actor.get_actor_location().x-330)<1
        comp.set_mobility(unreal.ComponentMobility.STATIONARY if isinstance(actor,unreal.DirectionalLight) or table_lamp else unreal.ComponentMobility.STATIC)
        comp.set_editor_property('indirect_lighting_intensity',1.25)
        lights.append(actor.get_name())

for mesh in static_meshes.values():
    if not mesh.get_path_name().startswith('/Game/'):continue
    assert unreal.CECharacterAssets.prepare_static_lighting(mesh),mesh.get_path_name()
    mesh.set_editor_property('generate_mesh_distance_field',True)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)

for pos,radius in [(unreal.Vector(-145,-130,180),480),(unreal.Vector(375,-100,170),620),(unreal.Vector(1080,-100,170),620)]:
    a=actors.spawn_actor_from_class(unreal.SphereReflectionCapture,pos)
    a.tags=['CEArchitecture','CE12_Reflection'];a.set_folder_path('Cafeteria12/Reflejos')
    a.get_component_by_class(unreal.SphereReflectionCaptureComponent).set_editor_property('influence_radius',radius)
    fixed.append(a)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
settings=world.get_world_settings()
lm=settings.get_editor_property('lightmass_settings')
lm.num_indirect_lighting_bounces=5;lm.num_sky_lighting_bounces=3
lm.indirect_lighting_quality=2;lm.indirect_lighting_smoothness=.8
lm.volumetric_lightmap_detail_cell_size=50
settings.set_editor_property('lightmass_settings',lm)
settings.set_editor_property('force_no_precomputed_lighting',False)
levels.save_current_level()
unreal.EditorAssetLibrary.save_directory('/Game/Prototype',only_if_is_dirty=True,recursive=True)
interactive=[a.get_name() for a in actors.get_all_level_actors() if isinstance(a,(unreal.CEProp,unreal.CEStation,unreal.CECustomer,unreal.CECafeDoor))]
assert not interactive,interactive
(root/'Evidence/entrega12_set.json').write_text(json.dumps({'fixed_actors':len(fixed),'materials':len(materials),'lights':len(lights),'mesh_assets':len(static_meshes),'interactive_actors_baked':bool(interactive)},indent=2))
unreal.log('CE12_SET_READY '+str(len(fixed)))
