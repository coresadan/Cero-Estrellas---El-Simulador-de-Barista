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
        mod=ob.modifiers.new('Cafe polygon budget','DECIMATE'); mod.ratio=.48 if slot in [0,1] else .28
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
    bones.extend([(f'upperarm_{side}','chest',(0,sign*23,126),(0,sign*28,104)),
                  (f'forearm_{side}',f'upperarm_{side}',(0,sign*28,104),(0,sign*31,83)),
                  (f'hand_{side}',f'forearm_{side}',(0,sign*31,83),(0,sign*31,75)),
                  (f'thigh_{side}','pelvis',(0,sign*10,77),(0,sign*10,43)),
                  (f'shin_{side}',f'thigh_{side}',(0,sign*10,43),(0,sign*10,9)),
                  (f'foot_{side}',f'shin_{side}',(0,sign*10,9),(-13,sign*10,4))])

manifest=[]
profiles=[('SK_Crypto',1.12),('SK_Hipster',.88),('SK_Influencer',.96),('SK_Punk',.95),
          ('SK_Astronaut',1.18),('SK_Diva',1.05),('SK_Biker',1.20),('SK_Granny',.86),('SK_Magician',.91)]
for index,(name,width) in enumerate(profiles):
    torso=loft([(0,0,80,11*width,14),(0,0,87,12*width,15),(0,0,105,14*width,18),
                (0,0,124,13*width,21),(0,0,130,10*width,18),(0,0,137,5.5,7)])
    # Shoulder bridges and tailored seams deform with chest and arm together.
    cloth=[torso,ellipsoid((0,-19,125),(6,5,5.5)),ellipsoid((0,19,125),(6,5,5.5))]; trousers=[ellipsoid((0,0,77),(11.5*width,16,10))]
    # One continuous face with a defined jaw, cheeks, bridge and nose.
    # Eye/mouth landmarks stay constant so accessories do not float on variants.
    jaw_width=[9.7,8.4,8.8,8.5,10.5,9.4,10.8,8.6,8.1][index]
    skin=[loft([(0,0,132,5,5.4),(0,0,144,5,5.4)]),
          ellipsoid((1,0,156),(11.8,jaw_width+1.9,14.3)),
          ellipsoid((-2,0,146.5),(8.8,jaw_width,6.6)),
          ellipsoid((-7,-5,151),(4,4.5,5)),ellipsoid((-7,5,151),(4,4.5,5)),
          ellipsoid((-10.4,0,155),(2.4,1.7,4.3)),ellipsoid((-12.4,0,152),(2.2,2.6,2)),
          ellipsoid((-10.2,0,148),(2.1,4,2.4)),
          ellipsoid((-9.6,-4.8,160),(2.7,3.6,1.8)),ellipsoid((-9.6,4.8,160),(2.7,3.6,1.8))]
    hands=[]; shoes=[]
    for sign in [-1,1]:
        cloth.append(loft([(0,sign*23,128,6.3,6.1),(0,sign*26,117,5.7,5.8),(0,sign*28,103,4.7,4.8)]))
        skin.append(loft([(0,sign*28,105,4.5,4.6),(0,sign*29.5,95,3.8,3.9),(0,sign*31,83,2.6,2.7)]))
        # Palm and fingers belong to the continuous skin surface, so no cap
        # floats between wrist and hand when the elbow flexes.
        skin.append(ellipsoid((-.6,sign*31,79.5),(2.7,3.1,4.3)))
        for finger in range(4):
            skin.append(ellipsoid((-1,sign*31+(finger-1.5)*1.5,73.6),(1.15,.88,2.9)))
        skin.append(ellipsoid((-3.2,sign*33,79),(1.8,1.25,2.7)))
        trousers.append(loft([(0,sign*10,79,9.3,9),(0,sign*10,60,8.2,8),(0,sign*10,43,6.8,6.9),
                              (0,sign*10,27,6.3,6.3),(0,sign*10,10,5.3,5.8)]))
        shoes.append(duplicate('SM_Shoe',(-4,sign*10,5),(.28,.32,.26)))
    hair=loft([(3,0,159,10.2,jaw_width+1.7),(2,0,165,11.5,jaw_width+2.1),
               (3,0,171,8.3,jaw_width*.8),(3,0,174,.6,.6)])
    hair_parts=[hair]
    if index==2: hair_parts.append(loft([(8,0,140,4,9),(9,0,154,4.5,11),(5,0,165,8,11)]))
    if index in [0,1,8]: hair_parts.append(ellipsoid((-7,-3,166),(4,8,3.2)))
    if index==7:
        for side in [-1,1]: hair_parts.append(ellipsoid((5,side*10,160),(5,5,5)))
    surfaces=[join_surface(cloth,0,True),join_surface(skin,1,True),join_surface(trousers,2,True),
              join_surface(hair_parts,3),join_surface(shoes,4)]
    # Weights blend around elbow/knee and spine; faces/accessories remain on the head.
    for ob in surfaces:
        for bone,_,_,_ in bones: ob.vertex_groups.new(name=bone)
        for v in ob.data.vertices:
            x,y,z=v.co; side='l' if y<0 else 'r'
            surface=ob.data.materials[0].name.split('.')[0]
            # Material regions are authoritative. The previous spatial rule
            # assigned the outside of trousers to hand/arm bones: swinging an
            # arm literally pulled the hip fabric like an elastic pocket.
            if surface=='Trousers':
                if z>76: weights={'pelvis':1}
                elif z>67:
                    t=(76-z)/9; weights={'pelvis':1-t,f'thigh_{side}':t}
                elif z>50: weights={f'thigh_{side}':1}
                elif z>36:
                    t=(50-z)/14; weights={f'thigh_{side}':1-t,f'shin_{side}':t}
                else: weights={f'shin_{side}':1}
            elif surface=='Clothing' and abs(y)<20 and z<105:
                t=max(0,min(1,(z-88)/17)); weights={'pelvis':1-t,'spine':t}
            elif surface=='Clothing' and z>116 and 17<abs(y)<24:
                t=max(0,min(1,(abs(y)-17)/7)); weights={'chest':1-t,f'upperarm_{side}':t}
            elif surface=='Leather': weights={f'foot_{side}':1}
            elif surface=='Hair' or z>147 or (surface=='Skin' and z>137 and (x<-6 or abs(y)>6 or z>145)): weights={'head':1}
            elif z>134 and abs(y)<12: weights={'neck':1}
            elif abs(y)>21 and z>67:
                if z>112: weights={f'upperarm_{side}':1}
                elif z>98:
                    t=max(0,min(1,(112-z)/14)); weights={f'upperarm_{side}':1-t,f'forearm_{side}':t}
                elif z>87: weights={f'forearm_{side}':1}
                elif z>80:
                    t=(87-z)/7; weights={f'forearm_{side}':1-t,f'hand_{side}':t}
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
    invalid=0; pants=0; trouser_indices=set()
    for polygon in mesh.data.polygons:
        if mesh.data.materials[polygon.material_index].name.split('.')[0]=='Trousers': trouser_indices.update(polygon.vertices)
    for v in mesh.data.vertices:
        if v.index in trouser_indices:
            pants+=1
            invalid+=any(mesh.vertex_groups[g.group].name.startswith(('upperarm','forearm','hand')) and g.weight>0 for g in v.groups)
    assert invalid==0, 'Trouser vertices must never follow arms or hands'
    manifest.append({'mesh':name,'vertices':len(mesh.data.vertices),'bones':len(bones),'original':True,'trouser_vertices':pants,'trouser_arm_influences':invalid})
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
