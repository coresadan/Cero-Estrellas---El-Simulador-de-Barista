"""Synthetic conversation regression against the actual CPU dialogue service."""
import argparse, json, time, urllib.request
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('--model', default='qwen3:4b-instruct')
a = p.parse_args()
system = ('Eres cliente del restaurante Cero Estrellas; hablas con tu camarero, el usuario. '
          'Eres hipster del café, pedante. Tu pedido: latte con leche y cruasán. '
          'Responde TODAS sus preguntas sobre cualquier tema, recuerda la conversación. '
          'Respuesta en español, hasta 100 caracteres, sin añadir preguntas. '
          'Hablas en primera persona de TU desayuno y en segunda persona de las cosas del camarero. '
          'Los incidentes recibidos los provoca el camarero contra ti; no inventes incidentes. '
          'Devuelve solo JSON: {"dialogo":{"texto":"respuesta"},"reaccion":{"emocion":"neutral","cambio_enfado":0,"quiere_irse":false}}. '
          'emocion: neutral/enfadado/humillado/triste/divertido; cambio_enfado entre -0.1 y 0.3. '
          'Disculpas reducen enfado; preguntas normales no lo aumentan.')
messages = [{'role':'system','content':system},
            {'role':'user','content':'Camarero: ¿Quién eres y dónde estás?'},
            {'role':'assistant','content':'{"dialogo":{"texto":"Soy cliente de Cero Estrellas; tú eres mi camarero."},"reaccion":{"emocion":"neutral","cambio_enfado":0,"quiere_irse":false}}'}]
turns = ['¿Qué quieres desayunar?', '¿En qué restaurante estás y con quién hablas?',
         'No, pides tú. ¿Qué quieres desayunar?', '¿Cuál es la capital de Francia?',
         'Mi perro se llama Croqueta. Recuérdalo.', '¿Cómo se llama mi perro?',
         '¿Qué líquido te he echado en la camisa? Discúlpame.']
checks = [lambda s:'latte' in s or 'leche' in s,
          lambda s:'cero estrellas' in s and ('camarero' in s or 'barista' in s),
          lambda s:('latte' in s or 'leche' in s) and 'te sirvo' not in s,
          lambda s:'parís' in s or 'paris' in s, lambda s:'croqueta' in s,
          lambda s:'croqueta' in s and 'mi perro' not in s,
          lambda s:('café' in s or 'cafe' in s) and not any(x in s for x in ['te he echado','te he derramado','eché','he echado'])]
results = []
for i, turn in enumerate(turns):
    state = '[Incidentes recibidos: café derramado en tu camisa por el camarero.]\n' if i == 6 else '[Incidentes recibidos: ninguno.]\n'
    messages.append({'role':'user','content':state+'Camarero: '+turn})
    if i==6:
        messages.append({'role':'system','content':
            'Tú eres el cliente que RECIBIÓ los incidentes. El camarero te habla. '
            'Si hablas del incidente, contesta como víctima: el camarero actúa, tú reaccionas. '
            'No repitas su frase ni hables como si tú fueras él.'})
    flat_schema={'type':'object','properties':{'texto':{'type':'string','minLength':1,'maxLength':100},
        'emocion':{'type':'string','enum':['neutral','enfadado','humillado','triste','divertido']},
        'cambio_enfado':{'type':'number','enum':[-.1,0,.1,.2,.3]},'quiere_irse':{'type':'boolean'}},
        'required':['texto','emocion','cambio_enfado','quiere_irse'],'additionalProperties':False}
    schema={'type':'object','properties':{
        'dialogo':{'type':'object','properties':{'texto':flat_schema['properties']['texto']},'required':['texto'],'additionalProperties':False},
        'reaccion':{'type':'object','properties':{k:v for k,v in flat_schema['properties'].items() if k!='texto'},
            'required':['emocion','cambio_enfado','quiere_irse'],'additionalProperties':False}},
        'required':['dialogo','reaccion'],'additionalProperties':False}
    body = {'model':a.model,'messages':messages,'format':schema,'stream':True,'think':False,
            'keep_alive':'20m','options':{'num_gpu':0,'num_thread':6,'num_ctx':2048,'num_predict':128,'temperature':.1}}
    start=time.monotonic(); first=None; content=''; envelope={}
    with urllib.request.urlopen(urllib.request.Request('http://127.0.0.1:11434/api/chat',
         json.dumps(body,ensure_ascii=False).encode(),{'Content-Type':'application/json'}),timeout=90) as response:
        for line in response:
            row=json.loads(line); content+=row.get('message',{}).get('content','')
            if first is None and len(content)>14: first=time.monotonic()-start
            if row.get('done'): envelope=row
    wire=json.loads(content)
    answer={**wire.get('dialogo',{}),**wire.get('reaccion',{})}
    contract=(set(wire)=={'dialogo','reaccion'} and set(answer)==set(flat_schema['required']) and isinstance(answer.get('texto'),str)
        and 0<len(answer['texto'])<=100 and answer.get('emocion') in flat_schema['properties']['emocion']['enum']
        and type(answer.get('cambio_enfado')) in (float,int) and -.1<=answer['cambio_enfado']<=.3
        and type(answer.get('quiere_irse')) is bool)
    passed=contract and checks[i](answer['texto'].lower())
    results.append({'question':turn,'answer':answer,'passed':passed,'first':first,
                    'seconds':time.monotonic()-start,'eval_count':envelope.get('eval_count'),
                    'cached':envelope.get('prompt_eval_cached_count')})
    print(i,passed,round(results[-1]['seconds'],2),answer,flush=True)
    messages.append({'role':'assistant','content':content})
root=Path(__file__).resolve().parent.parent
(root/'Evidence'/('context_benchmark_'+a.model.replace(':','_')+'.json')).write_text(
    json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
