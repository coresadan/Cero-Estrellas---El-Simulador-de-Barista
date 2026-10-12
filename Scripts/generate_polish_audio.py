"""Own ambient synthesis and authored immediate NPC lines; Piper CPU, offline."""
from pathlib import Path
import io, math, random, struct, wave, json
from piper import PiperVoice
root=Path(__file__).resolve().parent.parent
dest=root/'SourceAudio'/'Customers'; dest.mkdir(parents=True,exist_ok=True)
voice=PiperVoice.load(str(root/'Tools/LocalAI/Models/es_ES-davefx-medium.onnx'),use_cuda=False)
lines=[{'spill':'¡Mi camisa! ¡Esto no cotiza!','hit':'¡Eh! ¡Mis inversiones!','fall':'¡Me caigo! ¡Vaya desplome!','witness':'¡Esto es una burbuja de café!'},
       {'spill':'¡Qué acidez tan agresiva!','hit':'¡No pedí esta performance!','fall':'¡El suelo no es una mesa!','witness':'Esto merece una crítica demoledora.'},
       {'spill':'¡Mi outfit! ¡Lo has empapado!','hit':'¡Eh! ¡Eso está grabado!','fall':'¡Me caigo! ¡Sujeta el móvil!','witness':'¡No cortéis! ¡Esto se hace viral!'}]
for i,group in enumerate(lines):
    for cue,text in group.items():
        with wave.open(str(dest/f'NPC_{i}_{cue}.wav'),'wb') as out: voice.synthesize_wav(text,out)
(dest/'lines.json').write_text(json.dumps(lines,ensure_ascii=False,indent=2),encoding='utf-8')
rng=random.Random(601); rate=22050; duration=24; filtered=0; samples=[]
for n in range(rate*duration):
    t=n/rate; filtered=.965*filtered+.035*rng.uniform(-1,1)
    # Continuous hum plus slow street swells; no abrupt seam at loop boundary.
    hum=.025*math.sin(math.tau*60*t)+.012*math.sin(math.tau*120*t)
    traffic=.04*(.5+.5*math.sin(math.tau*t/duration))*filtered
    cups=sum(.045*math.sin(math.tau*freq*t)*math.exp(-((t-at)%duration)*14) for at,freq in [(3,1340),(11,1810),(18,1420)])
    samples.append(struct.pack('<h',int(max(-.9,min(.9,hum+traffic+cups))*25000)))
with wave.open(str(root/'SourceAudio/FX/CafeAmbience.wav'),'wb') as out:
    out.setnchannels(1); out.setsampwidth(2); out.setframerate(rate); out.writeframes(b''.join(samples))
print('CE_POLISH_AUDIO_READY 12 customer clips and bounded ambience loop')
