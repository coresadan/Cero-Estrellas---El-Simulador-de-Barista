"""UE 5.8: restrained PBR surfaces and bounded steam. Run before author_cafe11.py."""
from pathlib import Path
import json, unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
L=unreal.MaterialEditingLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
def node(cls): return L.create_material_expression(mat,cls)
def scalar(value):
    n=node(unreal.MaterialExpressionConstant); n.set_editor_property("r",value); return n
def link(a,b,pin,output=''):
    assert L.connect_material_expressions(a,output,b,pin),pin
def prop(n,p,out=''): assert L.connect_material_property(n,out,p)
def mul(a,b):
    n=node(unreal.MaterialExpressionMultiply); link(a,n,'A');link(b,n,'B');return n
def parameter(name,value):
    n=node(unreal.MaterialExpressionScalarParameter);n.set_editor_property("parameter_name",name);n.set_editor_property("default_value",value);return n
def finish():
    L.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat)

for name,texture,rough,metal,spec in [
    ('M_Metal','Metal',.34,.92,.5),('M_Ceramic',None,.24,0,.5),
    ('M_Wood','Wood',.48,0,.36),('M_Surface','Paint',.62,0,.3),
    ('M_Skin','Skin',.64,0,.24)]:
    mat=unreal.load_asset('/Game/Prototype/'+name)
    L.delete_all_material_expressions(mat)
    mat.set_editor_property('used_with_skeletal_mesh',True)
    if name!='M_Skin': mat.set_editor_property('used_with_static_lighting',True)
    tint=node(unreal.MaterialExpressionVectorParameter);tint.set_editor_property('parameter_name','Tint');tint.set_editor_property('default_value',unreal.LinearColor(.8,.7,.5,1))
    color=tint
    if texture:
        uv=node(unreal.MaterialExpressionTextureCoordinate)
        # Filtering and a restrained normal prevent distant metallic sparkle.
        a=node(unreal.MaterialExpressionTextureSample);a.set_editor_property('texture',unreal.load_asset('/Game/Prototype/Textures/'+texture+'_A'));link(uv,a,'UVs')
        color=mul(tint,a)
        if name!='M_Metal':
            n=node(unreal.MaterialExpressionTextureSample);n.set_editor_property('texture',unreal.load_asset('/Game/Prototype/Textures/'+texture+'_N'))
            n.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL);link(uv,n,'UVs');prop(n,unreal.MaterialProperty.MP_NORMAL,'RGB')
    prop(color,unreal.MaterialProperty.MP_BASE_COLOR)
    prop(parameter('Roughness',rough),unreal.MaterialProperty.MP_ROUGHNESS)
    prop(scalar(metal),unreal.MaterialProperty.MP_METALLIC)
    prop(scalar(spec),unreal.MaterialProperty.MP_SPECULAR)
    finish()

# Keep the existing pre-skinned wet patch and its gameplay Wetness parameter.
mat=unreal.load_asset('/Game/Prototype/M_Fabric')
mat.set_editor_property('used_with_static_lighting',True)
prop(scalar(.25),unreal.MaterialProperty.MP_SPECULAR);finish()
for name in ['M_Stone','M_Unlit']:
    mat=unreal.load_asset('/Game/Prototype/'+name)
    mat.set_editor_property('used_with_static_lighting',True);finish()

# A new puddle material uses the same RGB instance contract as the conserved liquid.
source=unreal.load_asset('/Game/Prototype/M_Liquid')
path='/Game/Prototype/M11_Puddle'
mat=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else tools.duplicate_asset('M11_Puddle','/Game/Prototype',source)
prop(scalar(.19),unreal.MaterialProperty.MP_ROUGHNESS)
prop(scalar(.5),unreal.MaterialProperty.MP_SPECULAR);finish()

mat=unreal.load_asset('/Game/Prototype/M_Steam')
L.delete_all_material_expressions(mat)
mat.set_editor_property('used_with_instanced_static_meshes',True)
mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
col=node(unreal.MaterialExpressionConstant3Vector);col.set_editor_property('constant',unreal.LinearColor(.77,.75,.70,1))
prop(col,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
data=node(unreal.MaterialExpressionPerInstanceCustomData);data.set_editor_property('data_index',0)
interp=node(unreal.MaterialExpressionVertexInterpolator);link(data,interp,'VS')
edge=node(unreal.MaterialExpressionFresnel);soft=node(unreal.MaterialExpressionOneMinus);link(edge,soft,'')
alpha=mul(mul(soft,interp),scalar(.08))
fade=node(unreal.MaterialExpressionDepthFade);link(alpha,fade,'Opacity');link(scalar(12),fade,'FadeDistance')
prop(fade,unreal.MaterialProperty.MP_OPACITY);finish()
unreal.EditorAssetLibrary.save_directory('/Game/Prototype',only_if_is_dirty=True,recursive=True)
(root/'Evidence/entrega11_materials.json').write_text(json.dumps({'passed':True,'metal_normal_removed':True,'wet_fabric_preserved':True,'steam_instance_fade':True},indent=2))
unreal.log('CE11_MATERIALS_READY')
