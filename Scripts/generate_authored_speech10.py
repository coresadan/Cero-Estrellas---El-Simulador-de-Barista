"""Pre-record the exact authored customer captions using the installed CPU voice."""
from pathlib import Path
import re, hashlib, json, wave
from piper import PiperVoice
root=Path(__file__).resolve().parent.parent
dest=root/'SourceAudio/Dialogue'; dest.mkdir(parents=True,exist_ok=True)
voice=PiperVoice.load(str(root/'Tools/LocalAI/Models/es_ES-davefx-medium.onnx'),use_cuda=False)
lines=set()
for name in ['CECustomer.cpp','CECustomerStories.cpp','CECustomerSocial.cpp','CECustomerRig.cpp','CEProp.cpp','CELiquids.cpp','CEDialogue.cpp']:
    source=(root/'Source/CeroEstrellas'/name).read_text(encoding='utf-8-sig')
    for literal in re.findall(r'TEXT\("((?:[^"\\]|\\.)*)"\)',source):
        text=literal.replace('\\n','\n').replace('\\"','"')
        if 18<=len(text)<=190 and not any(c in text for c in ['%','/','_','[',']','{','}']) and (' ' in text) and not text.startswith(('Camarero:','Eres cliente','Te escucha. Puedes','El cliente está','El cliente reformula')):
            lines.add(text)
rows=[]
for text in sorted(lines):
    name='Speech_'+hashlib.md5(text.encode('utf-8')).hexdigest()[:16]
    path=dest/(name+'.wav')
    if not path.exists():
        with wave.open(str(path),'wb') as wav: voice.synthesize_wav(text,wav)
    rows.append({'asset':name,'text':text})
(dest/'lines.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
print('CE10_EXACT_SPEECH_READY',len(rows))
