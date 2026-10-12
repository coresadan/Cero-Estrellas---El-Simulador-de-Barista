"""Prepare the configured CPU model while the game starts; no player data sent."""
import configparser, json, time, urllib.request
from pathlib import Path
root=Path(__file__).resolve().parent.parent
settings=configparser.ConfigParser(strict=False)
settings.read(root/'Config/DefaultGame.ini',encoding='utf-8-sig')
model=settings.get('CeroEstrellas.AI','Model',fallback='qwen3:4b-instruct')
for attempt in range(20):
    try:
        with urllib.request.urlopen('http://127.0.0.1:11434/api/ps',timeout=2) as response: loaded=json.load(response)['models']
        if any(m['name']==model and m.get('size_vram',0)==0 and m.get('context_length')==2048 for m in loaded): break
        data=json.dumps({'model':model,'prompt':'','keep_alive':'20m','options':{'num_gpu':0,'num_thread':6,'num_ctx':2048}}).encode()
        with urllib.request.urlopen(urllib.request.Request('http://127.0.0.1:11434/api/generate',data,{'Content-Type':'application/json'}),timeout=60) as response: response.read()
        break
    except Exception:
        time.sleep(.5)
