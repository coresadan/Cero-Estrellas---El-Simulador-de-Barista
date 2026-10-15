"""Own bounded sound layers and latte artwork, generated without external assets."""
from pathlib import Path
import numpy as np, wave, struct, json
root=Path(__file__).resolve().parent.parent
rate=22050; rng=np.random.default_rng(9009)
def wav(name,values):
    with wave.open(str(root/'SourceAudio/FX'/(name+'.wav')),'wb') as f:
        f.setnchannels(1); f.setsampwidth(2); f.setframerate(rate)
        f.writeframes(np.int16(np.clip(values,-.95,.95)*30000).tobytes())
t=np.arange(rate*24)/rate
street=np.zeros_like(t)
for freq in [43,67,91,131]: street+=.012*np.sin(2*np.pi*freq*t+freq)
street*=.5+.5*np.sin(2*np.pi*t/24)**2
wav('CafeStreet',street)
room=.009*np.sin(2*np.pi*50*t)+.006*np.sin(2*np.pi*100*t)
for at,freq in [(2,1520),(8,1700),(15,1320),(21,1950)]:
    d=(t-at)%24; room+=.075*np.sin(2*np.pi*freq*d)*np.exp(-d*20)
wav('CafeRoom',room)
t=np.arange(int(rate*1.7))/rate
pulse=((t%1)<.17)|(((t%1)>.27)&((t%1)<.44))
wav('PhoneRing',pulse*.12*(np.sin(2*np.pi*880*t)+.5*np.sin(2*np.pi*1100*t)))
t=np.arange(rate*2)/rate; opera=np.zeros_like(t)
for at,freq in [(0,330),(.55,415),(1.1,494)]:
    d=t-at; env=np.where(d>=0,np.exp(-np.maximum(d,0)*4)*np.minimum(np.maximum(d,0)*25,1),0)
    opera+=.12*env*(np.sin(2*np.pi*freq*t)+.25*np.sin(2*np.pi*freq*2*t))
wav('OperaWarmup',opera)
n=512; y,x=np.mgrid[-1:1:complex(n),-1:1:complex(n)]
r=np.sqrt(x*x+y*y); a=np.arctan2(y,x)
swirl=np.sin(30*r+3*a+2*np.sin(a))*.5+.5
heart=((x*.85)**2+(y+.16)**2-.24)**3-(x*.85)**2*(y+.16)**3
cream=np.clip(.76+swirl*.14,0,1)
heartmask=np.clip(-heart*150+.4,0,1)
color=np.stack([cream,cream*.86,cream*.66],axis=-1)
color=color*(1-heartmask[...,None]) + np.array([.97,.90,.72])*heartmask[...,None]
pixels=np.uint8(np.clip(color,0,1)*255)
header=struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,n,n,24,32)
(root/'SourceTextures/Foam_A.tga').write_bytes(header+pixels[...,::-1].tobytes())
(root/'Evidence/entrega09_authored_effects.json').write_text(json.dumps({'original':True,'sounds':['CafeStreet','CafeRoom','PhoneRing','OperaWarmup'],'texture':'Foam_A','resolution':n},indent=2),encoding='utf-8')
print('CE09_EFFECTS_READY')
