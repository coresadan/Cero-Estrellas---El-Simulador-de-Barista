"""Blender: own puddle outline, curved eyebrows and close-up ceramic geometry."""
from pathlib import Path
import math,bpy
root=Path(__file__).resolve().parent.parent
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
def export(name,verts,faces):
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update()
    obj=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(obj)
    uv=mesh.uv_layers.new(name='SurfaceUV')
    for p in mesh.polygons:
        axis=max(range(3),key=lambda a:abs(p.normal[a]))
        for li in p.loop_indices:
            co=mesh.vertices[mesh.loops[li].vertex_index].co
            a,b=(co.y,co.z) if axis==0 else (co.x,co.z) if axis==1 else (co.x,co.y)
            uv.data[li].uv=(a/100+.5,b/100+.5)
    for p in mesh.polygons:p.use_smooth=True
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.wm.obj_export(filepath=str(root/'SourceMeshes'/(name+'.obj')),export_selected_objects=True,forward_axis='Y',up_axis='Z',global_scale=1)
    obj.hide_set(True)
# Flat centre and a softly rolled edge; only 97 vertices, no overlapping cylinders.
n=48;verts=[(0,0,.45)]
for ring,z in [(.92,.45),(1,0)]:
    for i in range(n):
        a=i*math.tau/n;r=45*(1+.075*math.sin(a*3+.4)+.045*math.cos(a*5))
        verts.append((r*ring*math.cos(a),r*ring*math.sin(a),z))
faces=[]
for i in range(n):
    j=(i+1)%n;faces.extend([(0,1+i,1+j),(1+i,1+n+i,1+n+j,1+j)])
export('SM11_Puddle',verts,faces)
# A rounded arch in the same 100-unit bounding envelope used by the face rig.
verts=[]
for i in range(13):
    t=i/12;y=-50+100*t;z=8*math.sin(t*math.pi)
    for j in range(8):
        a=j*math.tau/8;verts.append((math.cos(a)*40,y,z+math.sin(a)*40))
faces=[]
for i in range(12):
    for j in range(8):
        a=i*8+j;b=i*8+(j+1)%8;faces.append((a,b,b+8,a+8))
faces.extend([tuple(reversed(range(8))),tuple(range(96,104))])
export('SM11_Brow',verts,faces)
bpy.ops.wm.save_as_mainfile(filepath=str(root/'SourceMeshes/CeroEstrellas_Detalles11.blend'))
print('CE11_DETAILS_READY')
