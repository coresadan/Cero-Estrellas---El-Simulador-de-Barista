"""Direct backend check using source-authored stories and a real UE request schema.

This is deliberately reported separately from Unreal end-to-end dialogue tests.
"""
from pathlib import Path
import json, re, time, urllib.request

root=Path(__file__).resolve().parent.parent
source=(root/'Source/CeroEstrellas/CECustomerStories.cpp').read_text(encoding='utf-8')
stories=re.findall(r'\{TEXT\("([^"]+)"\),TEXT\("([^"]+)"\),TEXT\("([^"]+)"\),TEXT\("([^"]+)"\),TEXT\("([^"]+)"\)\}',source)
voices=re.findall(r'TEXT\("([^"]+)"\)',source.split('const TCHAR* Voices[]={',1)[1].split('};',1)[0])
assert len(stories)==5 and len(voices)==9
folder=root/'Evidence/Entrega08Stories'
folder.mkdir(exist_ok=True)
requests=sorted((root/'Evidence/Entrega08DialogoFinal').glob('request_*.json'),key=lambda p:int(p.stem.split('_')[-1]))
assert requests,'Run the real packaged dialogue scenario first.'
raw=requests[0].read_bytes()
base=json.loads(raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig'))
cases=[
    (3,'¿Qué tienes que hacer después de desayunar?',['entrevista','trabajo','currículum','curriculum','puesto']),
    (2,'¿Cómo reconocerás a tu cita?',['flor','cita']),
    (1,'¿Por qué estás tomando notas del restaurante?',['crític','critic','reseña','evalu','restaurante']),
    (5,'¿Para qué estás calentando la voz?',['audición','audicion','ópera','opera','cantar']),
    (8,'¿Para quién estás escribiendo el poema?',['ex','pareja'])
]
results=[]
for index,(look,question,keywords) in enumerate(cases):
    title,setup,goal,good,bad=stories[index]
    body={k:v for k,v in base.items() if k!='messages'}
    body['stream']=False
    context={'animo_actual':'neutral','incidentes_recibidos':[],
             'historia_cotidiana':{'situacion':setup,'recibiste_apoyo':False,'resuelta':False}}
    body['messages']=[
        {'role':'system','content':'Eres cliente del restaurante Cero Estrellas; hablas con tu camarero, el usuario. '+voices[look]+' Tu pedido: Latte · café + leche + cruasán. Responde en español, hasta 80 caracteres. No inventes hechos. Sigue el schema JSON.'},
        {'role':'user','content':'[Estado del cliente; incidentes recibidos del camarero]\n'+json.dumps(context,ensure_ascii=False,separators=(',',':'))+'\nCamarero: '+question}
    ]
    started=time.monotonic()
    request=urllib.request.Request('http://127.0.0.1:11434/api/chat',data=json.dumps(body,ensure_ascii=False).encode('utf-8'),headers={'Content-Type':'application/json'})
    with urllib.request.urlopen(request,timeout=90) as reply: envelope=json.load(reply)
    answer=json.loads(envelope['message']['content']); words=answer.get('dialogo',{}).get('texto',''); reaction=answer.get('reaccion',{})
    contract=set(answer)=={'dialogo','reaccion'} and set(answer['dialogo'])=={'texto'} and set(reaction)=={'emocion','cambio_enfado','quiere_irse'} and 1<=len(words)<=100 and reaction['emocion'] in ['neutral','enfadado','humillado','triste','divertido'] and reaction['cambio_enfado'] in [-.1,0,.1,.2,.3] and type(reaction['quiere_irse']) is bool
    relevant=any(keyword in words.lower() for keyword in keywords)
    row={'story':title,'profile':look,'question':question,'answer':words,'contract_valid':contract,'mentions_story_context':relevant,'seconds':round(time.monotonic()-started,2)}
    results.append(row)
    (folder/f'case_{index}.json').write_text(json.dumps({'request':body,'provider':envelope,'check':row},ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(row,ensure_ascii=False),flush=True)
report={'passed':all(r['contract_valid'] and r['mentions_story_context'] for r in results),'scope':'direct_backend_check_not_unreal_end_to_end','human_semantic_review':False,'cases':results}
(folder/'story_dialogue_result.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
assert report['passed'],'Inspect direct story responses; this check does not prove complete semantic coherence.'
