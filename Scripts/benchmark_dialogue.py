"""Replay synthetic test fixtures against the real local model, not player data."""
import json, urllib.request
from pathlib import Path
root=Path(__file__).resolve().parent.parent
folder=root/'Evidence/DialogoPlayable'
results=[]
for number in [4,5]:
    blob=(folder/f'request_{number}.json').read_bytes()
    req=json.loads(blob.decode('utf-16' if blob.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig'))
    systems=[m['content'] for m in req['messages'] if m['role']=='system']
    system='\n'.join(systems)+' Los mensajes user son del barista. Tú eres el cliente. Nunca repitas sus preguntas: contéstalas desde tu papel.'
    messages=[{'role':'system','content':system}]
    for msg in req['messages']:
        if msg['role']=='system': continue
        if msg['role']=='assistant': msg={'role':'assistant','content':json.loads(msg['content'])['texto']}
        messages.append(msg)
    req['messages']=messages; req['model']='qwen2.5:3b'
    req['options']['num_thread']=6; req['options']['temperature']=.3
    data=json.dumps(req,ensure_ascii=False).encode('utf-8')
    with urllib.request.urlopen(urllib.request.Request('http://127.0.0.1:11434/api/chat',data,{'Content-Type':'application/json'}),timeout=90) as response: answer=json.load(response)
    results.append(answer); print(number,answer['message']['content'],flush=True)
(root/'Evidence/model_quality_benchmark.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
