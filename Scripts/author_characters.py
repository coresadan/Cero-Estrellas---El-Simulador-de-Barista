"""Original, weighted customer bodies and first-person grip; Blender 5.2.

Centimetres, front = -X. Own meshes/materials only, no downloaded characters.
The FBX preserves named joints for the bounded procedural animation in UE.
"""
import bpy, math, json
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'SourceMeshes'
bpy.ops.wm.open_mainfile(filepath=str(OUT / 'CeroEstrellas_Modelos.blend'))
sources = {n: bpy.data.objects[n] for n in ['SM_Head', 'SM_Hair', 'SM_Hand', 'SM_Shoe']}
for ob in bpy.data.objects: ob.hide_set(True); ob.select_set(False)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = .01

materials = []
for name in ['Clothing', 'Skin', 'Trousers', 'Hair', 'Leather']:
    m = bpy.data.materials.new(name); materials.append(m)

def ellipsoid(pos, scale):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=16, radius=1, location=pos)
    ob = bpy.context.object; ob.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return ob

def loft(rings, sides=32):
    vertices = []; faces = []
    for x,y,z,rx,ry in rings:
        vertices.extend([(x+rx*math.cos(a*math.tau/sides), y+ry*math.sin(a*math.tau/sides), z) for a in range(sides)])
    for r in range(len(rings)-1):
        for s in range(sides):
            a=r*sides+s; b=r*sides+(s+1)%sides
            faces.append((a,b,b+sides,a+sides))
    faces.extend([tuple(reversed(range(sides))), tuple(range((len(rings)-1)*sides,len(rings)*sides))])
    mesh=bpy.data.meshes.new('TailoredSurface'); mesh.from_pydata(vertices,[],faces); mesh.update()
    ob=bpy.data.objects.new('TailoredSurface',mesh); bpy.context.collection.objects.link(ob)
    return ob

def duplicate(source, pos, scale):
    ob=bpy.data.objects.new('AuthoredPart',sources[source].data.copy()); bpy.context.collection.objects.link(ob)
    ob.location=pos; ob.scale=scale
    return ob

def join_surface(parts, slot, fuse=False):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in parts: ob.hide_set(False); ob.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]; bpy.ops.object.join(); ob=bpy.context.object
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    if fuse:
        ob.data.remesh_voxel_size=.65; bpy.ops.object.voxel_remesh()
        mod=ob.modifiers.new('Tailoring','SMOOTH'); mod.factor=.6; mod.iterations=3
        bpy.ops.object.modifier_apply(modifier=mod.name)
        mod=ob.modifiers.new('Cafe polygon budget','DECIMATE'); mod.ratio=.28
        bpy.ops.object.modifier_apply(modifier=mod.name)
    ob.data.materials.clear(); ob.data.materials.append(materials[slot])
    for p in ob.data.polygons: p.use_smooth=True
    # Box-projected UVs, metres-like frequency independent of skinny triangles.
    uv=ob.data.uv_layers.new(name='SurfaceUV')
    for p in ob.data.polygons:
        axis=max(range(3),key=lambda a:abs(p.normal[a]))
        for li in p.loop_indices:
            v=ob.data.vertices[ob.data.loops[li].vertex_index].co
            a,b=(v.y,v.z) if axis==0 else (v.x,v.z) if axis==1 else (v.x,v.y)
            uv.data[li].uv=(a/55,b/55)
    return ob

bones=[('root',None,(0,0,0),(0,0,12)), ('pelvis','root',(0,0,77),(0,0,94)),
       ('spine','pelvis',(0,0,104),(0,0,122)), ('chest','spine',(0,0,126),(0,0,136)),
       ('neck','chest',(0,0,139),(0,0,147)), ('head','neck',(0,0,155),(0,0,171))]
for sign,side in [(-1,'l'),(1,'r')]:
    bones.extend([(f'upperarm_{side}','chest',(0,sign*20,126),(0,sign*25,104)),
                  (f'forearm_{side}',f'upperarm_{side}',(0,sign*25,104),(0,sign*27,83)),
                  (f'hand_{side}',f'forearm_{side}',(0,sign*27,83),(0,sign*27,75)),
                  (f'thigh_{side}','pelvis',(0,sign*10,77),(0,sign*10,43)),
                  (f'shin_{side}',f'thigh_{side}',(0,sign*10,43),(0,sign*10,9)),
                  (f'foot_{side}',f'shin_{side}',(0,sign*10,9),(-13,sign*10,4))])

manifest=[]
for index,(name,width) in enumerate([('SK_Crypto',1.0),('SK_Hipster',.92),('SK_Influencer',.85)]):
    torso=loft([(0,0,78,12*width,15),(0,0,86,13*width,17),(0,0,105,15*width,20),
                (0,0,124,14*width,23),(0,0,130,11*width,19),(0,0,137,5.5,7)])
    cloth=[torso]; trousers=[ellipsoid((0,0,78),(13,18,12))]
    skin=[loft([(0,0,132,5.5,6),(0,0,145,5.7,6)]), duplicate('SM_Head',(-1,0,155),(.65,.58,.56))]
    hands=[]; shoes=[]
    for sign in [-1,1]:
        cloth.append(loft([(0,sign*20,128,7.8,8),(0,sign*23,117,7,7.5),(0,sign*25,103,5.8,6)]))
        skin.append(loft([(0,sign*25,105,5.4,5.6),(0,sign*26,95,4.8,5),(0,sign*27,83,3.3,3.8)]))
        hands.append(duplicate('SM_Hand',(0,sign*27,78),(.17,.15,.12)))
        trousers.append(loft([(0,sign*10,79,9.3,9),(0,sign*10,60,8.2,8),(0,sign*10,43,6.8,6.9),
                              (0,sign*10,27,6.3,6.3),(0,sign*10,10,5.3,5.8)]))
        shoes.append(duplicate('SM_Shoe',(-4,sign*10,5),(.28,.32,.26)))
    hair=ellipsoid((2,0,164),(13.6,12.6,10.1))
    for v in hair.data.vertices:
        if v.co.x<0: v.co.z=max(-2,v.co.z)
    surfaces=[join_surface(cloth,0,True),join_surface(skin,1,True),join_surface(trousers,2,True),
              join_surface([hair]+([ellipsoid((9,0,150),(5,12,18))] if index==2 else []),3),join_surface(shoes,4),join_surface(hands,1)]
    # Weights blend around elbow/knee and spine; faces/accessories remain on the head.
    for ob in surfaces:
        for bone,_,_,_ in bones: ob.vertex_groups.new(name=bone)
        for v in ob.data.vertices:
            x,y,z=v.co; side='l' if y<0 else 'r'
            if ob.data.materials[0].name.startswith('Hair') or z>147: weights={'head':1}
            elif z>134 and abs(y)<12: weights={'neck':1}
            elif abs(y)>18 and z>67:
                if z>112: weights={f'upperarm_{side}':1}
                elif z>98:
                    t=max(0,min(1,(112-z)/14)); weights={f'upperarm_{side}':1-t,f'forearm_{side}':t}
                elif z>85: weights={f'forearm_{side}':1}
                else: weights={f'hand_{side}':1}
            elif z<85:
                if z>70: weights={'pelvis':1}
                elif z>50: weights={f'thigh_{side}':1}
                elif z>36:
                    t=(50-z)/14; weights={f'thigh_{side}':1-t,f'shin_{side}':t}
                elif z>11: weights={f'shin_{side}':1}
                else: weights={f'foot_{side}':1}
            else:
                t=max(0,min(1,(z-95)/28)); weights={'spine':1-t,'chest':t}
            for bone,w in weights.items():
                if w>0: ob.vertex_groups[bone].add([v.index],w,'REPLACE')
    bpy.ops.object.select_all(action='DESELECT')
    for ob in surfaces: ob.select_set(True)
    bpy.context.view_layer.objects.active=surfaces[0]; bpy.ops.object.join(); mesh=bpy.context.object; mesh.name=name
    bpy.ops.object.armature_add(); rig=bpy.context.object; rig.name='CE_Rig_'+name
    bpy.ops.object.mode_set(mode='EDIT'); rig.data.edit_bones.remove(rig.data.edit_bones[0])
    for bone,parent,head,tail in bones:
        b=rig.data.edit_bones.new(bone); b.head=head; b.tail=tail
        if parent: b.parent=rig.data.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    mod=mesh.modifiers.new('CustomerSkeleton','ARMATURE'); mod.object=rig; mesh.parent=rig
    bpy.ops.object.select_all(action='DESELECT'); rig.select_set(True); mesh.select_set(True); bpy.context.view_layer.objects.active=rig
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')), use_selection=True, object_types={'ARMATURE','MESH'},
        add_leaf_bones=False, bake_anim=False, axis_forward='Y', axis_up='Z', use_mesh_modifiers=True,
        apply_unit_scale=True, apply_scale_options='FBX_SCALE_ALL',mesh_smooth_type='FACE')
    manifest.append({'mesh':name,'vertices':len(mesh.data.vertices),'bones':len(bones),'original':True})
    rig.hide_set(True); mesh.hide_set(True)

# Curled fingers and palm, a dedicated grip instead of an open dangling hand.
grip=[ellipsoid((0,0,0),(4.4,3.7,2.2))]
for i in range(4):
    grip.extend([ellipsoid((1.6,(i-1.5)*1.65,-1.5),(2.8,.9,1.1)),ellipsoid((3.7,(i-1.5)*1.65,-3),(1,.9,1.7))])
grip.append(ellipsoid((-.5,-3.7,-1),(2.8,1.1,1.4)))
ob=join_surface(grip,1,True); ob.name='SM_GripHand'
tri=ob.modifiers.new('Triangles','TRIANGULATE'); bpy.context.view_layer.objects.active=ob; bpy.ops.object.modifier_apply(modifier=tri.name)
bpy.ops.object.select_all(action='DESELECT'); ob.select_set(True)
bpy.ops.wm.obj_export(filepath=str(OUT/'SM_GripHand.obj'), export_selected_objects=True, forward_axis='Y', up_axis='Z', global_scale=1)
(OUT/'character_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'CeroEstrellas_Personajes.blend'))
print('CE_CHARACTERS_READY',manifest)
