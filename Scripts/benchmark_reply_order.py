"""Replay a synthetic packaged regression with text before reaction metadata."""
import json, time, urllib.request
from pathlib import Path
root=Path(__file__).resolve().parent.parent
blob=(root/'Evidence/Entrega05Dialogo/request_8.json').read_bytes()
req=json.loads(blob.decode('utf-16' if blob.startswith(b'\xff\xfe') else 'utf-8-sig'))
flat=req['format']
req['format']={'type':'object','properties':{
    'dialogo':{'type':'object','properties':{'texto':flat['properties']['texto']},'required':['texto'],'additionalProperties':False},
    'reaccion':{'type':'object','properties':{k:v for k,v in flat['properties'].items() if k!='texto'},
                'required':['emocion','cambio_enfado','quiere_irse'],'additionalProperties':False}},
    'required':['dialogo','reaccion'],'additionalProperties':False}
example='{"dialogo":{"texto":"respuesta"},"reaccion":{"emocion":"neutral","cambio_enfado":0,"quiere_irse":false}}'
req['messages'][0]['content']=req['messages'][0]['content'].replace(
    '{"texto":"respuesta","emocion":"neutral","cambio_enfado":0,"quiere_irse":false}',example)
for message in req['messages']:
    if message['role']=='assistant':
        flat_answer=json.loads(message['content'])
        message['content']=json.dumps({'dialogo':{'texto':flat_answer.pop('texto')},'reaccion':flat_answer},ensure_ascii=False,separators=(',',':'))
req['stream']=True
req['messages'].append({'role':'system','content':
    'Tú eres el cliente que RECIBIÓ los incidentes. El camarero te habla. '
    'Si hablas del incidente, contesta como víctima: el camarero actúa, tú reaccionas. '
    'No repitas su frase ni hables como si tú fueras él.'})
start=time.monotonic(); content=''; first=None
with urllib.request.urlopen(urllib.request.Request('http://127.0.0.1:11434/api/chat',
     json.dumps(req,ensure_ascii=False).encode(),{'Content-Type':'application/json'}),timeout=90) as response:
    for line in response:
        row=json.loads(line); content+=row.get('message',{}).get('content','')
        if first is None and len(content)>25: first=time.monotonic()-start
answer=json.loads(content)
result={'answer':answer,'first_text_seconds':first,'total_seconds':time.monotonic()-start,
        'cached':row.get('prompt_eval_cached_count'),'generated_tokens':row.get('eval_count')}
(root/'Evidence/reply_order_benchmark.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False),flush=True)
