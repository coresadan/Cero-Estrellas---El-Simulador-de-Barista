"""Summarize validated packaged replies; separate visible text from completion."""
import argparse, json, re, statistics
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('evidence',nargs='?',default='Entrega05DialogoFinal')
p.add_argument('--output',default='entrega05_dialogue_latency.json')
p.add_argument('--baseline',default='dialogue_latency.json')
a=p.parse_args()
root=Path(__file__).resolve().parent.parent
folder=root/'Evidence'/a.evidence
def read_json(path):
    raw=path.read_bytes()
    return json.loads(raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig'))
report=read_json(folder/'dialogue_result.json')
assert report['passed'], 'Only summarize a completed, passing conversation scenario.'
timings=re.findall(r'CE_DIALOGUE_REPLY .*?first_text=([\d.]+) complete=([\d.]+)',
                   (folder/'runtime.log').read_text(encoding='utf-8-sig',errors='replace'))
accepted=list(folder.glob('accepted_*.json'))
files=sorted(accepted or folder.glob('response_*.json'),key=lambda path:int(path.stem.split('_')[-1]))
rows=[]
for path in files:
    envelope=read_json(path)
    try:
        answer=json.loads(envelope['message']['content'])
        if set(answer)=={'dialogo','reaccion'}:
            answer={**answer['dialogo'],**answer['reaccion']}
    except (ValueError,KeyError): continue
    if set(answer)!={'texto','emocion','cambio_enfado','quiere_irse'}: continue
    rows.append({'file':path.name,'text':answer['texto'],
        'backend_seconds':round(envelope['total_duration']/1e9,2),
        'load_seconds':round(envelope['load_duration']/1e9,2),
        'prompt_tokens':envelope['prompt_eval_count'],
        'cached_prompt_tokens':envelope.get('prompt_eval_cached_count',0),
        'generated_tokens':envelope['eval_count']})
assert len(rows)==len(timings)==report['validated_live_replies']==8, 'Incomplete or mismatched evidence.'
for row,(first,complete) in zip(rows,timings):
    row.update(first_visible_seconds=float(first),validated_seconds=float(complete))
baseline=read_json(root/'Evidence'/a.baseline)
previous=statistics.median(baseline['replies'][i]['backend_seconds'] for i in [0,4,5,6,7]) if isinstance(baseline,dict) else statistics.median(row['total_seconds'] for row in baseline)
matched=statistics.median(rows[i]['backend_seconds'] for i in [0,4,5,6,7])
summary={'version':report['version'],'evidence':a.evidence,'baseline':a.baseline,'rejected_attempts':max(0,len(list(folder.glob('response_*.json')))-len(files)),'model':read_json(files[0])['model'],
    'followup_visible_seconds':{'min':min(r['first_visible_seconds'] for r in rows[1:]),
        'max':max(r['first_visible_seconds'] for r in rows[1:])},
    'followup_validated_seconds':{'min':min(r['validated_seconds'] for r in rows[1:]),
        'max':max(r['validated_seconds'] for r in rows[1:])},
    'previous_five_turn_median_seconds':previous,
    'matching_five_turn_median_seconds':matched,
    'matching_median_reduction_percent':round((previous-matched)/previous*100,1),
    'comparison_note':'Same hardware, real packaged tests; matching order, name, recall, incident and confirmed transcript. Other CPU load and reply lengths can vary.',
    'replies':rows}
output=root/'Evidence'/a.output
output.write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in summary.items() if k!='replies'},ensure_ascii=False,indent=2))
