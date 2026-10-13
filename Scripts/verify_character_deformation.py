"""Blender regression: arm motion cannot deform trousers or leave unweighted vertices."""
import bpy, json
from pathlib import Path
root=Path(__file__).resolve().parent.parent
bpy.ops.wm.open_mainfile(filepath=str(root/'SourceMeshes/CeroEstrellas_Personajes.blend'))
results=[]
for name in ['SK_Crypto','SK_Hipster','SK_Influencer']:
    mesh=bpy.data.objects[name]; rig=mesh.parent
    mesh.hide_set(False); rig.hide_set(False)
    trousers=set()
    for polygon in mesh.data.polygons:
        if mesh.data.materials[polygon.material_index].name.split('.')[0]=='Trousers': trousers.update(polygon.vertices)
    def coordinates():
        bpy.context.view_layer.update()
        evaluated=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
        return [v.co.copy() for v in evaluated.data.vertices]
    for bone in rig.pose.bones: bone.rotation_mode='XYZ'; bone.rotation_euler=(0,0,0)
    before=coordinates()
    for side,sign in [('l',-1),('r',1)]:
        rig.pose.bones['upperarm_'+side].rotation_euler=(sign*.8,.2,sign*.3)
        rig.pose.bones['forearm_'+side].rotation_euler=(-1.,0,0)
    after=coordinates()
    assert len(before)==len(after)==len(mesh.data.vertices)
    pants_motion=max((after[i]-before[i]).length for i in trousers)
    assert pants_motion<.001,(name,'arm pulls trousers',pants_motion)
    unweighted=sum(abs(sum(g.weight for g in v.groups)-1)>1e-4 for v in mesh.data.vertices)
    assert unweighted==0,(name,'invalid skin weights',unweighted)
    hand_indices=[v.index for v in mesh.data.vertices if any(mesh.vertex_groups[g.group].name.startswith('hand_') and g.weight>.9 for g in v.groups)]
    hand_motion=max((after[i]-before[i]).length for i in hand_indices)
    assert hand_motion>5,(name,'hands did not move',hand_motion)
    results.append({'mesh':name,'trouser_vertices':len(trousers),'max_trouser_displacement_from_arm_motion_cm':pants_motion,'max_hand_motion_cm':hand_motion,'invalid_weight_sums':unweighted,'passed':True})
path=root/'Evidence/entrega08_deformation.json'
path.write_text(json.dumps({'passed':True,'characters':results},indent=2),encoding='utf-8')
print('CE_DEFORMATION_REGRESSION_PASS',results)
