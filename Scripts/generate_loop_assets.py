"""Generate original geometry, appliance sounds and the single-source boss transcript."""
from pathlib import Path
import json, math, random, re, struct, wave

root = Path(__file__).resolve().parent.parent
mesh_dir = root / 'SourceMeshes'
fx_dir = root / 'SourceAudio' / 'FX'
mesh_dir.mkdir(exist_ok=True)
fx_dir.mkdir(parents=True, exist_ok=True)
lines = re.findall(r'TEXT\("([^"\n]*)"\)', (root / 'Source/CeroEstrellas/CEBossLines.h').read_text(encoding='utf-8'))
(root / 'SourceAudio/Boss/lines.json').write_text(json.dumps(lines, ensure_ascii=False, indent=2), encoding='utf-8')

# Ceramic shell, rim and inner bowl; actual centimetres, centred at the physics origin.
n = 64
rings = [(11,-12),(11,10),(10.5,12),(9.5,12),(9.3,-9),(0,-9),(0,-12),(11,-12)]
verts = [(r*math.cos(i*math.tau/n), r*math.sin(i*math.tau/n), z) for r,z in rings for i in range(n)]
obj = ['# Original hollow ceramic cup, units centimetres', 'o SM_Cup']
obj += [f'v {x:.6f} {y:.6f} {z:.6f}' for x,y,z in verts]
obj += [f'vt {i/n:.6f} {ring/(len(rings)-1):.6f}' for ring in range(len(rings)) for i in range(n)]
for ring in range(len(rings)):
    for i in range(n):
        radial = 1 if ring < 3 else -1 if ring < 5 else 0
        vertical = 1 if ring in (2,3,5) else -1 if ring >= 6 else 0
        x,y,z=radial*math.cos(i*math.tau/n),radial*math.sin(i*math.tau/n),vertical
        length=math.sqrt(x*x+y*y+z*z) or 1
        obj.append(f'vn {x/length:.6f} {y/length:.6f} {z/length:.6f}')
for ring in range(len(rings)-1):
    for i in range(n):
        a=ring*n+i+1; b=ring*n+(i+1)%n+1; c=(ring+1)*n+(i+1)%n+1; d=(ring+1)*n+i+1
        if rings[ring][0] == 0 and rings[ring+1][0] == 0:
            continue
        if rings[ring+1][0] == 0:
            obj.append(f'f {a}/{a}/{a} {b}/{b}/{b} {c}/{c}/{c}')
        elif rings[ring][0] == 0:
            obj.append(f'f {a}/{a}/{a} {c}/{c}/{c} {d}/{d}/{d}')
        else:
            obj.append(f'f {a}/{a}/{a} {b}/{b}/{b} {c}/{c}/{c} {d}/{d}/{d}')
(mesh_dir / 'SM_Cup.obj').write_text('\n'.join(obj)+'\n', encoding='ascii')

def body_mesh(name, profile):
    # Original clothed silhouette: shoulders, waist and jaw instead of scaled spheres.
    segments=32
    lines=['# Original procedural character part, centimetres', 'o '+name]
    points=[(x*100*math.cos(i*math.tau/segments), y*100*math.sin(i*math.tau/segments), z*100)
            for z,x,y in profile for i in range(segments)]
    lines += [f'v {x:.6f} {y:.6f} {z:.6f}' for x,y,z in points]
    lines += [f'vt {i/segments:.6f} {r/(len(profile)-1):.6f}' for r in range(len(profile)) for i in range(segments)]
    lines += [f'vn {math.cos(i*math.tau/segments):.6f} {math.sin(i*math.tau/segments):.6f} 0' for r in profile for i in range(segments)]
    for r in range(len(profile)-1):
        for i in range(segments):
            a=r*segments+i+1; b=r*segments+(i+1)%segments+1; c=b+segments; d=a+segments
            lines.append(f'f {a}/{a}/{a} {b}/{b}/{b} {c}/{c}/{c} {d}/{d}/{d}')
    (mesh_dir/(name+'.obj')).write_text('\n'.join(lines)+'\n',encoding='ascii')
body_mesh('SM_Torso',[(-.5,.02,.02),(-.46,.37,.35),(-.2,.34,.34),(.12,.41,.43),(.32,.36,.50),(.43,.28,.41),(.5,.12,.16)])
body_mesh('SM_Head',[(-.5,.03,.03),(-.39,.27,.28),(-.2,.43,.41),(.05,.47,.47),(.25,.48,.5),(.41,.36,.39),(.5,.04,.04)])

rng=random.Random(714)
rate=22050
def sound(name, duration, fn):
    samples=[]
    for i in range(int(rate*duration)):
        t=i/rate
        env=min(1,t/.015)*min(1,(duration-t)/.08)
        samples.append(struct.pack('<h', int(max(-.98,min(.98,fn(t)))*env*22000)))
    with wave.open(str(fx_dir / (name+'.wav')), 'wb') as wav:
        wav.setnchannels(1); wav.setsampwidth(2); wav.setframerate(rate); wav.writeframes(b''.join(samples))
sound('Grinder',1.6,lambda t: .25*math.sin(t*math.tau*145)+rng.uniform(-.25,.25))
sound('Brew',3.2,lambda t: .13*math.sin(t*math.tau*65)+.08*math.sin(t*math.tau*130)+rng.uniform(-.07,.07))
sound('Steam',2.5,lambda t: rng.uniform(-.25,.25)*(1+.2*math.sin(t*20)))
sound('Splash',.3,lambda t: rng.uniform(-.35,.35)*math.exp(-t*12)+.15*math.sin(t*math.tau*(180-t*280)))
sound('Click',.10,lambda t: rng.uniform(-.4,.4)*math.exp(-t*55))
sound('Door',.8,lambda t: (.25*math.sin(t*math.tau*850)+.15*math.sin(t*math.tau*1230))*math.exp(-t*4))
sound('Order',.35,lambda t: .2*math.sin(t*math.tau*(600 if t<.16 else 900)))
sound('Thump',.2,lambda t: .65*math.sin(t*math.tau*(75-t*180))*math.exp(-t*20))
sound('Ceramic',.65,lambda t: rng.uniform(-.4,.4)*math.exp(-t*9)+.2*math.sin(t*math.tau*2400)*math.exp(-t*12))
sound('Electric',.6,lambda t: rng.uniform(-.23,.23)*math.exp(-t*4)+.15*math.sin(t*math.tau*85))
sound('Footstep',.13,lambda t: rng.uniform(-.18,.18)*math.exp(-t*35)+.15*math.sin(t*math.tau*105)*math.exp(-t*40))
print(f'Generated cup, character silhouettes, 11 sound effects and {len(lines)} boss transcripts.')
