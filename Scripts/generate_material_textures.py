"""Original, tileable PBR surfaces and laptop artwork authored from math.
No external artwork; 512 px maps with mipmaps keep the cafe within a modest GPU budget.
"""
from pathlib import Path
import json, struct
import numpy as np
root=Path(__file__).resolve().parent.parent
out=root/'SourceTextures'; out.mkdir(exist_ok=True)
n=512; rng=np.random.default_rng(20261011)
y,x=np.mgrid[0:n,0:n]/n
freq=np.fft.fftfreq(n)*n
radius=freq[:,None]**2+freq[None,:]**2
def noise(scale):
    field=np.fft.ifft2(np.fft.fft2(rng.normal(size=(n,n)))*np.exp(-radius/(scale*scale))).real
    return (field-field.mean())/(field.std()+1e-9)
def tga(name,values):
    pixels=np.uint8(np.clip(values,0,1)*255)
    header=struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,n,n,24,32)
    (out/(name+'.tga')).write_bytes(header+pixels[...,::-1].tobytes())
def srgb(linear):
    return np.where(linear<=.0031308,linear*12.92,1.055*np.maximum(linear,0)**(1/2.4)-.055)
def surface(name,albedo,height,rough,metal=0,strength=1):
    gray=np.clip(albedo,.18,1)
    tga(name+'_A',srgb(np.repeat(gray[...,None],3,axis=2)))
    dx=(np.roll(height,-1,1)-np.roll(height,1,1))*strength
    dy=(np.roll(height,-1,0)-np.roll(height,1,0))*strength
    normal=np.stack((-dx,dy,np.ones_like(dx)),axis=2)
    normal/=np.linalg.norm(normal,axis=2)[...,None]
    tga(name+'_N',normal*.5+.5)
    orm=np.stack((np.clip(1-abs(height-height.mean())*.03,.92,1),np.clip(rough,.1,.98),np.ones_like(x)*metal),axis=2)
    tga(name+'_ORM',orm)
warp=noise(3)*.9+noise(12)*.13
grain=np.sin(2*np.pi*(y*72+warp*.8+np.sin(x*2*np.pi)*.6))
growth=np.sin(2*np.pi*(y*13+warp*.35))
wood=np.clip(.86+grain*.065+growth*.07+noise(70)*.012,.6,1)
surface('Wood',wood,grain*.09+growth*.04,.39+noise(18)*.045,strength=2.5)
flecks=np.clip(noise(170),-2,2)
stone=.91+noise(9)*.025+flecks*.032
surface('Stone',stone,flecks*.07,.5+noise(80)*.08,strength=.7)
weave=np.sin(x*2*np.pi*128)*np.sin(y*2*np.pi*128)
surface('Fabric',.91+weave*.024+noise(45)*.022,weave*.04,.88+noise(80)*.025,strength=.8)
brush=np.repeat(rng.uniform(-1,1,size=(n,1)),n,axis=1)
surface('Metal',.985+brush*.003,brush*.003,.29+brush*.012,.85,strength=.35)
skin_noise=noise(180)
surface('Skin',.97+noise(12)*.008,skin_noise*.008,.63+noise(35)*.025,strength=.8)
paint_noise=noise(160)
surface('Paint',.97+noise(25)*.003,paint_noise*.004,.62+noise(50)*.008,strength=.6)
# A quiet fictional finance screen, including the joke: all investments fell.
screen=np.zeros((n,n,3))+np.array([.025,.045,.053])
def rect(x0,y0,x1,y1,c): screen[y0:y1,x0:x1]=c
rect(0,0,512,44,[.07,.14,.15]); rect(24,70,488,72,[.12,.24,.25])
glyphs={'C':['01110','10001','10000','10000','10000','10001','01110'],
    'O':['01110','10001','10001','10001','10001','10001','01110'],
    'I':['11111','00100','00100','00100','00100','00100','11111'],
    'N':['10001','11001','11001','10101','10011','10011','10001'],
    '9':['01110','10001','10001','01111','00001','00001','01110'],
    '%':['11001','11010','00100','01000','10110','00110','00000'],
    '-':['00000','00000','00000','11111','00000','00000','00000']}
def text(value,px,py,scale,color):
    for char in value:
        for row,bits in enumerate(glyphs.get(char,['00000']*7)):
            for col,bit in enumerate(bits):
                if bit=='1': rect(px+col*scale,py+row*scale,px+(col+1)*scale,py+(row+1)*scale,color)
        px+=6*scale
text('COIN',24,12,3,[.6,.79,.74]); text('-99%',300,90,5,[.91,.43,.27])
for i in range(1,5): rect(24,150+i*52,488,151+i*52,[.065,.11,.12])
points=[(30,180),(80,193),(105,177),(160,231),(206,255),(260,241),(325,331),(380,361),(465,415)]
for (ax,ay),(bx,by) in zip(points,points[1:]):
    for u in np.linspace(0,1,max(abs(bx-ax),abs(by-ay))*2):
        px=int(ax+(bx-ax)*u); py=int(ay+(by-ay)*u); rect(px-1,py-1,px+2,py+2,[.86,.48,.3])
for i in range(3): rect(24,460+i*13,200-i*28,463+i*13,[.2,.31,.31])
# The front (-X) face is mirrored by the OBJ-to-Unreal handedness conversion.
# Compensate U only; an additional V flip would turn the text upside down.
tga('Screen_A',np.fliplr(screen))
manifest={'original':True,'resolution':n,'material_sets':['Wood','Stone','Fabric','Metal','Skin','Paint'],
    'maps_per_set':['A: sRGB albedo multiplier','N: tangent DirectX normal','ORM: occlusion / roughness / metallic'],
    'textures':19,'source_bytes':sum(p.stat().st_size for p in out.glob('*.tga'))}
(out/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('CE_TEXTURES_READY',manifest)
