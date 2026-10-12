"""Author original production-prototype meshes in Blender, no downloaded art.
Run: blender --background --python Scripts/model_cafe.py
All exported coordinates are centimetres; articulated parts retain gameplay pivots.
"""
import bpy, math, json
from mathutils import Vector
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'SourceMeshes'
OUT.mkdir(exist_ok=True)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
manifest = {}

def bevel(obj, width=2, segments=3):
    mod=obj.modifiers.new('Rounded manufactured edges','BEVEL'); mod.width=width; mod.segments=segments
    bpy.context.view_layer.objects.active=obj; bpy.ops.object.modifier_apply(modifier=mod.name)
    for p in obj.data.polygons: p.use_smooth=True
    mod=obj.modifiers.new('Weighted surface normals','WEIGHTED_NORMAL'); mod.keep_sharp=True
    bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj

def box(pos, size, radius=2):
    bpy.ops.mesh.primitive_cube_add(size=1, location=pos); obj=bpy.context.object
    obj.scale=size; bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return bevel(obj,radius)

def ellipsoid(pos, size, segments=32, rings=16):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, radius=1, location=pos)
    obj=bpy.context.object; obj.scale=size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    for p in obj.data.polygons: p.use_smooth=True
    return obj

def tube(points, radius=1, sides=10):
    verts=[]; faces=[]
    for i, point in enumerate(points):
        tangent=Vector(points[min(i+1,len(points)-1)])-Vector(points[max(0,i-1)])
        tangent.normalize(); u=tangent.cross(Vector((0,1,0))).normalized()
        if u.length<.1: u=tangent.cross(Vector((1,0,0))).normalized()
        v=tangent.cross(u).normalized()
        r=radius[i] if isinstance(radius,list) else radius
        for s in range(sides): verts.append(Vector(point)+r*(u*math.cos(s*math.tau/sides)+v*math.sin(s*math.tau/sides)))
    for i in range(len(points)-1):
        for s in range(sides):
            a=i*sides+s; b=i*sides+(s+1)%sides; faces.append((a,b,b+sides,a+sides))
    faces += [tuple(reversed(range(sides))),tuple(range((len(points)-1)*sides,len(points)*sides))]
    mesh=bpy.data.meshes.new('Tube'); mesh.from_pydata(verts,[],faces); mesh.update()
    obj=bpy.data.objects.new('Tube',mesh); bpy.context.collection.objects.link(obj)
    for p in mesh.polygons: p.use_smooth=True
    return obj

def lathe(profile, segments=64, spout=False):
    verts=[]; faces=[]
    for ring,(r,z) in enumerate(profile):
        for i in range(segments):
            angle=i*math.tau/segments
            reach=3.5*max(0,-math.sin(angle))**18*max(0,(z-3)/9) if spout else 0
            verts.append(((r+reach)*math.cos(angle),(r+reach)*math.sin(angle),z))
    for ring in range(len(profile)-1):
        for i in range(segments):
            a=ring*segments+i; b=ring*segments+(i+1)%segments; faces.append((a,b,b+segments,a+segments))
    mesh=bpy.data.meshes.new('Turned'); mesh.from_pydata(verts,[],faces); mesh.update()
    obj=bpy.data.objects.new('Turned',mesh); bpy.context.collection.objects.link(obj)
    for p in mesh.polygons: p.use_smooth=True
    return obj

def export(name, objects, normalize=False):
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects: obj.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]; bpy.ops.object.join()
    obj=bpy.context.object; obj.name=name
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if name == 'SM_Head':
        # Fuse cheeks and nose into a continuous surface instead of intersecting beads.
        obj.data.remesh_voxel_size=.65
        bpy.ops.object.voxel_remesh()
        smooth=obj.modifiers.new('Face smoothing','SMOOTH'); smooth.factor=.65; smooth.iterations=4
        bpy.ops.object.modifier_apply(modifier=smooth.name)
        for polygon in obj.data.polygons: polygon.use_smooth=True
    # Triangulate the actual authored result, including rounded edges.
    mod=obj.modifiers.new('Export triangles','TRIANGULATE'); bpy.ops.object.modifier_apply(modifier=mod.name)
    mesh=obj.data
    points=[obj.matrix_world @ v.co for v in mesh.vertices]
    if normalize:
        lows=[min(p[a] for p in points) for a in range(3)]; highs=[max(p[a] for p in points) for a in range(3)]
        points=[Vector([(p[a]-(highs[a]+lows[a])/2)*100/(highs[a]-lows[a]) for a in range(3)]) for p in points]
    lines=['# Original Cero Estrellas mesh; centimetres','o '+name]
    lines += [f'v {p.x:.6f} {p.y:.6f} {p.z:.6f}' for p in points]
    # Explicit UV and split corner normals are required by Unreal Interchange.
    for poly in mesh.polygons:
        axis=max(range(3),key=lambda a:abs(poly.normal[a]))
        for vi in poly.vertices:
            p=points[vi]
            # Box-projected UVs: horizontal furniture needs XY, not a collapsed XZ strip.
            u,v=(p.y,p.z) if axis==0 else (p.x,p.z) if axis==1 else (p.x,p.y)
            lines.append(f'vt {(u+50)/100:.6f} {(v+50)/100:.6f}')
    normal_matrix=obj.matrix_world.to_3x3().inverted().transposed()
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            n=normal_matrix @ mesh.corner_normals[li].vector
            if normalize: n=Vector([n[a]*(highs[a]-lows[a])/100 for a in range(3)])
            n.normalize(); lines.append(f'vn {n.x:.6f} {n.y:.6f} {n.z:.6f}')
    cursor=1
    for poly in mesh.polygons:
        refs=[]
        for vi in poly.vertices: refs.append(f'{vi+1}/{cursor}/{cursor}'); cursor+=1
        lines.append('f '+' '.join(refs))
    (OUT/(name+'.obj')).write_text('\n'.join(lines)+'\n',encoding='ascii')
    manifest[name]={'vertices':len(points),'triangles':len(mesh.polygons),'original':True}
    obj.hide_set(True)
    return obj

export('SM_BevelCube',[box((0,0,0),(100,100,100),2)])
cup=lathe([(0.01,-12),(8.3,-12),(9,-11),(10.6,-4),(11,9),(10.8,11.4),(10.3,12),(9.7,11.7),(9.5,10.8),(9.1,-7.5),(8,-9),(.01,-9)])
handle=tube([(9,0,8),(13,0,9),(17,0,7),(19,0,3),(18,0,-2),(15,0,-6),(10,0,-6)],1.8,12)
export('SM_Cup',[cup,handle])
pitcher=lathe([(.01,-12),(8,-12),(8.5,-11),(9.6,8),(10,11),(9.7,12),(9,11.5),(8.7,9),(7.3,-9),(.01,-9)],spout=True)
export('SM_Pitcher',[pitcher,tube([(9,0,8),(15,0,8),(16,0,5),(16,0,-5),(12,0,-7),(9,0,-6)],1.1,10)])

# Curved crescent, overlapping pastry rolls and raised laminated folds.
parts=[]
for i in range(13):
    a=(i-6)*.22; x=math.sin(a)*33; y=math.cos(a)*18-12
    taper=1-abs(i-6)/8
    roll=ellipsoid((x,y,0),(7+6*taper,8+5*taper,3+7*taper),24,12)
    roll.rotation_euler.z=-a; parts.append(roll)
    if 1<i<11:
        points=[(x+math.sin(a)*4, y+math.cos(a)*4+math.sin(t)*8,math.cos(t)*8*taper+1) for t in [j*math.pi/12 for j in range(-6,7)]]
        parts.append(tube(points,.65,6))
export('SM_Croissant',parts,True)

def limb(name, profile):
    export(name,[lathe(profile,32)],True)
limb('SM_Sleeve',[(.1,-50),(16,-49),(21,-42),(23,-15),(25,24),(19,44),(7,50),(.1,50)])
limb('SM_Forearm',[(.1,-50),(11,-48),(13,-26),(17,20),(18,39),(12,49),(.1,50)])
limb('SM_Trouser',[(.1,-50),(14,-49),(17,-35),(20,0),(24,30),(21,48),(.1,50)])
limb('SM_Shin',[(.1,-50),(12,-49),(12,-30),(17,12),(18,37),(15,49),(.1,50)])
hands=[ellipsoid((0,0,0),(18,24,36),24,12)]
for finger in range(4): hands.append(ellipsoid((-3,(finger-1.5)*12,-34),(8,6,21-abs(finger-1.5)*3),16,8))
hands.append(ellipsoid((-10,-24,-5),(9,8,22),16,8))
export('SM_Hand',hands,True)
shoe=[box((0,0,-6),(85,38,14),5),ellipsoid((-8,0,2),(40,18,15),24,12),ellipsoid((29,0,9),(13,16,20),24,12)]
export('SM_Shoe',shoe,True)

# More human jaw, cheekbones and an integrated nose; front points along -X.
head=ellipsoid((1,0,3),(20,20,28),48,24)
for v in head.data.vertices:
    p=v.co
    if p.z<-3: p.y*=.72+.28*(p.z+28)/25
    if p.x<0: p.x*=.78 if p.z<-5 else .92
export('SM_Head',[head,ellipsoid((-15,0,0),(4,3.4,8),24,12),ellipsoid((-18,0,-4),(4,4,3),24,12)],True)
torso=lathe([(.1,-50),(30,-49),(34,-36),(35,-10),(40,20),(42,34),(32,44),(12,50),(.1,50)],40)
for v in torso.data.vertices: v.co.x*=.9
export('SM_Torso',[torso],True)
hair=[ellipsoid((3,0,18),(20.5,21,13),32,16)]
for i in range(6):
    a=(i-3)*.32; hair.append(ellipsoid((-12+math.sin(a)*8,math.cos(a)*14-8,24+math.sin(i)),(7,3,3),20,10))
export('SM_Hair',hair,True)
export('SM_Glasses',[tube([(-18,y+5*math.sin(t),5+3.2*math.cos(t)) for t in [j*math.tau/32 for j in range(33)]],.55,8) for y in [-7,7]]+[tube([(-18,-2,5),(-19,0,5),(-18,2,5)],.5,8)])
export('SM_Wand',[tube([(0,0,20),(0,0,9),(5,4,1),(7,11,-10),(7,11,-20)],1.5,12)])
export('SM_Bottle',[lathe([(.1,-50),(20,-50),(23,-45),(23,8),(20,23),(9,32),(8,48),(7,50),(.1,50)],32)],True)
export('SM_Pot',[lathe([(.1,-50),(27,-50),(32,-45),(45,40),(48,44),(48,50),(42,50),(40,40),(28,-40),(.1,-40)],32)],True)
leaf=ellipsoid((0,0,0),(5,20,50),20,12)
for v in leaf.data.vertices: v.co.x+=math.sin((v.co.z+50)*math.pi/100)*18
export('SM_Leaf',[leaf],True)
export('SM_Hopper',[lathe([(.1,-50),(10,-50),(10,-35),(34,0),(45,40),(44,50),(.1,50)],40)],True)
export('SM_SeatBack',[box((0,0,0),(100,100,100),8)])
tray=[]
for side in [-1,1]:
    tray.append(box((0,side*34,3),(105,2.5,7),1.2))
    tray.append(box((side*54,0,3),(2.5,65,7),1.2))
export('SM_TrayRim',tray)
export('SM_Filter',[lathe([(.01,-3),(9,-3),(10,-2),(10,2),(9.3,3),(8,3),(8,1),(.01,1)],48)])
export('SM_FilterGrip',[box((18,0,0),(28,5,5),2.3)])
export('SM_SteamKnob',[lathe([(.01,-4),(6,-4),(7,-2),(7,2),(6,4),(.01,4)],32)])
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'CeroEstrellas_Modelos.blend'))
print('CE_MODELS_READY',len(manifest),sum(m['triangles'] for m in manifest.values()),'triangles')
