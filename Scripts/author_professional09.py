"""Original hero objects; Blender 5.2, centimetres, no downloaded meshes."""
from pathlib import Path
import runpy, math, bpy
from mathutils import Vector
root=Path(__file__).resolve().parent.parent
base=runpy.run_path(str(root/'Scripts/model_cafe.py'))
box,lathe,tube,export=[base[k] for k in ['box','lathe','tube','export']]
# A real shell, rather than a solid cuboid with details pasted on it.
export('SM09_EspressoShell',[
    box((0,0,-31),(145,65,5),2),box((0,-30,0),(140,5,60),2),
    box((-70,0,0),(5,61,60),2),box((70,0,0),(5,61,60),2),
    box((0,0,31),(140,61,5),2),box((0,30,22),(138,5,18),2)],True)
export('SM09_TableTop',[box((0,0,0),(125,115,8),3.8)],True)
export('SM09_Pendant',[lathe([(.01,16),(4,16),(6,12),(10,5),(23,-6),(26,-8),(25,-10),(23,-8),(8,8),(3,13),(.01,13)])],True)
export('SM09_Saucer',[lathe([(.01,-1.5),(13,-1.5),(17,-.5),(18,1),(17.5,1.6),(12,.3),(.01,.2)],48)],True)
export('SM09_GaugeRim',[lathe([(7,-1),(9,-1),(9.5,0),(9,1),(7,1),(7,-1)],48)],True)
export('SM09_Knob',[lathe([(.01,-3),(5,-3),(6,-2),(6,2),(5,4),(.01,4)],32)],True)
# Continuous tapered pastry with sculpted lamination, not intersecting balls.
points=[]; radii=[]
for i in range(49):
    a=-1.48+i*2.96/48; points.append((math.sin(a)*33,math.cos(a)*22-12,0))
    radii.append(1.1+10.5*math.sin(math.pi*i/48)**.75)
pastry=[tube(points,radii,24)]
for i in range(5,45,5):
    p=Vector(points[i]); tangent=(Vector(points[i+1])-Vector(points[i-1])).normalized()
    side=Vector((-tangent.y,tangent.x,0)); r=radii[i]*1.025
    fold=[p+side*(math.sin(t)*r)+Vector((0,0,math.cos(t)*r)) for t in [j*math.pi/16 for j in range(-8,9)]]
    pastry.append(tube(fold,.45,6))
export('SM_Croissant',pastry,True)
# Folded collar and lapel shapes replace the visible rectangular chest blocks.
export('SM09_Collar',[tube([(-12,-8,1),(-13,-5,-4),(-14,0,-7),(-13,5,-4),(-12,8,1)],1.2,8)],True)
bpy.ops.wm.save_as_mainfile(filepath=str(root/'SourceMeshes/CeroEstrellas_Arte09.blend'))
print('CE09_HERO_MESHES_READY')
