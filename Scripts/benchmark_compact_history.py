"""Replay real recorded turns with dialogue-only history, keeping current facts."""
import argparse, json, time, urllib.request
from pathlib import Path
root=Path(__file__).resolve().parent.parent
folder=root/'Evidence/Entrega05DialogoVerificado'
p=argparse.ArgumentParser(); p.add_argument('--model'); p.add_argument('--context-checks',action='store_true'); args=p.parse_args()
results=[]
cases=[(3,'restaurant_roles'),(4,'player_correction'),(7,'name_recall'),(8,'physical_recipient')] if args.context_checks else [(7,'name_recall'),(8,'physical_recipient'),(10,'confirmed_transcript')]
for number,case in cases:
    data=(folder/f'request_{number}.json').read_bytes()
    req=json.loads(data.decode('utf-16' if data.startswith(b'\xff\xfe') else 'utf-8-sig'))
    if args.model: req['model']=args.model
    original=req['messages']
    last=max(i for i,m in enumerate(original) if m['role']=='user')
    compact=original[:3]
    for message in original[3:last]:
        if message['role']=='user':
            compact.append({'role':'user','content':'Camarero: '+message['content'].split('\nCamarero: ',1)[-1]})
        elif message['role']=='assistant':
            compact.append({'role':'assistant','content':json.loads(message['content'])['dialogo']['texto']})
    compact.extend(original[last:]); req['messages']=compact
    start=time.monotonic(); content=''; first=None
    with urllib.request.urlopen(urllib.request.Request('http://127.0.0.1:11434/api/chat',
         json.dumps(req,ensure_ascii=False).encode(),{'Content-Type':'application/json'}),timeout=90) as response:
        for line in response:
            row=json.loads(line); content+=row.get('message',{}).get('content','')
            if first is None and len(content)>25: first=time.monotonic()-start
    answer=json.loads(content); text=answer['dialogo']['texto']; lower=text.lower()
    passed=bool(text)
    if case=='restaurant_roles': passed='cero estrellas' in lower and ('camarero' in lower or 'barista' in lower)
    if case=='player_correction': passed=('latte' in lower or 'leche' in lower) and 'camisa' not in lower and 'te sirvo' not in lower
    if case=='name_recall': passed='croqueta' in lower and 'mi perro' not in lower
    if case=='physical_recipient':
        passed='café' in lower and not any(word in lower for word in ('te he echado','te he derramado','eché','no has echado','no me has echado','no has derramado'))
    result={'case':case,'passed':passed,'answer':answer,'first_text_seconds':round(first or 0,2),
            'total_seconds':round(time.monotonic()-start,2),'prompt_tokens':row.get('prompt_eval_count'),
            'cached_prompt_tokens':row.get('prompt_eval_cached_count'),'generated_tokens':row.get('eval_count')}
    results.append(result); print(json.dumps(result,ensure_ascii=False),flush=True)
output='compact_history_benchmark.json' if not args.model else 'compact_model_benchmark_'+args.model.replace(':','_')+'.json'
(root/'Evidence'/output).write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
assert all(r['passed'] for r in results), 'Compact history changed the meaning of a reply.'
