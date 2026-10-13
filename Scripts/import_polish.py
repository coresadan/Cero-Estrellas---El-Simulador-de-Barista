"""Idempotent UE assets for wet fabric, steam, customer voices and ambient mix."""
import unreal, os
root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
tools = unreal.AssetToolsHelpers.get_asset_tools()
L = unreal.MaterialEditingLibrary
mat = unreal.load_asset('/Game/Prototype/M_Fabric')
L.delete_all_material_expressions(mat)

def node(cls, x=0, y=0):
    return L.create_material_expression(mat, cls, x, y)

def link(a, b, pin, output=''):
    assert L.connect_material_expressions(a, output, b, pin), str(type(b))+' input '+pin

def scalar(value):
    n = node(unreal.MaterialExpressionConstant)
    n.set_editor_property('r', value)
    return n
tint = node(unreal.MaterialExpressionVectorParameter, -800, 0)
tint.set_editor_property('parameter_name', 'Tint')
tint.set_editor_property('default_value', unreal.LinearColor(0.8, 0.7, 0.5, 1))
wet = node(unreal.MaterialExpressionScalarParameter, -800, 300)
wet.set_editor_property('parameter_name', 'Wetness')
wet.set_editor_property('default_value', 0)
position = node(unreal.MaterialExpressionPreSkinnedPosition, -1000, 420)
interp = node(unreal.MaterialExpressionVertexInterpolator, -800, 420)
link(position, interp, 'VS')
centre = node(unreal.MaterialExpressionConstant3Vector, -1000, 560)
centre.set_editor_property('constant', unreal.LinearColor(-12, 0, 108, 1))
patch = node(unreal.MaterialExpressionSphereMask, -600, 420)
patch.set_editor_property('attenuation_radius', 32)
patch.set_editor_property('hardness_percent', 45)
link(interp, patch, 'A')
link(centre, patch, 'B')
wet_patch = node(unreal.MaterialExpressionMultiply, -400, 420)
link(wet, wet_patch, 'A')
link(patch, wet_patch, 'B')
uv = node(unreal.MaterialExpressionTextureCoordinate, -1100, -100)
uv.set_editor_property('u_tiling', 2)
uv.set_editor_property('v_tiling', 2)
samples = {}
for suffix in ['A', 'N', 'ORM']:
    s = node(unreal.MaterialExpressionTextureSample, -800, -200)
    s.set_editor_property('texture', unreal.load_asset('/Game/Prototype/Textures/Fabric_' + suffix))
    if suffix == 'N':
        s.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    if suffix == 'ORM':
        s.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    link(uv, s, 'UVs')
    samples[suffix] = s
base = node(unreal.MaterialExpressionMultiply, -400, -100)
link(tint, base, 'A')
link(samples['A'], base, 'B', 'RGB')
dark = node(unreal.MaterialExpressionMultiply)
link(base, dark, 'A')
link(scalar(0.48), dark, 'B')
color = node(unreal.MaterialExpressionLinearInterpolate)
link(base, color, 'A')
link(dark, color, 'B')
link(wet_patch, color, 'Alpha')
rough = node(unreal.MaterialExpressionLinearInterpolate)
link(samples['ORM'], rough, 'A', 'G')
link(scalar(0.25), rough, 'B')
link(wet_patch, rough, 'Alpha')
L.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
L.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
L.connect_material_property(samples['N'], 'RGB', unreal.MaterialProperty.MP_NORMAL)
L.connect_material_property(samples['ORM'], 'R', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
mat.set_editor_property('used_with_skeletal_mesh', True)
L.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
path = '/Game/Prototype/M_Steam'
mat = unreal.load_asset(path) or tools.create_asset('M_Steam', '/Game/Prototype', unreal.Material, unreal.MaterialFactoryNew())
L.delete_all_material_expressions(mat)
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('used_with_instanced_static_meshes', True)
col = node(unreal.MaterialExpressionConstant3Vector)
col.set_editor_property('constant', unreal.LinearColor(0.65, 0.66, 0.63, 1))
opacity = node(unreal.MaterialExpressionDepthFade)
edge = node(unreal.MaterialExpressionFresnel)
soft = node(unreal.MaterialExpressionOneMinus); link(edge,soft,'')
alpha = node(unreal.MaterialExpressionMultiply); link(soft,alpha,'A'); link(scalar(.025),alpha,'B')
link(alpha, opacity, 'Opacity')
link(scalar(8), opacity, 'FadeDistance')
L.connect_material_property(col, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
L.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
L.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
tasks = []
for folder, destination in [('Customers', '/Game/Prototype/Audio/Customers'), ('FX', '/Game/Prototype/Audio/FX'), ('Boss', '/Game/Prototype/Audio')]:
    for filename in os.listdir(os.path.join(root, 'SourceAudio', folder)):
        if not filename.endswith('.wav') or (folder == 'FX' and filename != 'CafeAmbience.wav') or (folder == 'Boss' and not filename.startswith('Boss_Event_')):
            continue
        t = unreal.AssetImportTask()
        t.set_editor_property('filename', os.path.join(root, 'SourceAudio', folder, filename))
        t.set_editor_property('destination_path', destination)
        t.set_editor_property('automated', True)
        t.set_editor_property('replace_existing', True)
        t.set_editor_property('save', True)
        tasks.append(t)
tools.import_asset_tasks(tasks)
amb = unreal.load_asset('/Game/Prototype/Audio/FX/CafeAmbience')
amb.set_editor_property('looping', True)
unreal.EditorAssetLibrary.save_loaded_asset(amb)
lim = unreal.load_asset('/Game/Prototype/Audio/SC_Customers') or tools.create_asset('SC_Customers', '/Game/Prototype/Audio', unreal.SoundConcurrency, unreal.SoundConcurrencyFactory())
rules = lim.get_editor_property('concurrency')
rules.set_editor_property('max_count', 2)
rules.set_editor_property('resolution_rule', unreal.MaxConcurrentResolutionRule.STOP_OLDEST)
rules.set_editor_property('retrigger_time', 0.1)
lim.set_editor_property('concurrency', rules)
unreal.EditorAssetLibrary.save_loaded_asset(lim)
unreal.log('CE_POLISH_IMPORT_READY wet fabric, steam, 24 voices, 3 event calls, ambience, concurrency')
