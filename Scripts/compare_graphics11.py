"""Make a portable before/after viewer from real runtime captures and reports."""
from pathlib import Path
import base64,json,html,struct
root=Path(__file__).resolve().parent.parent
old=root/'Evidence/Entrega10Closure'
new=root/'Evidence/Entrega11Closure'
out=root/'Evidence/Comparativa11';out.mkdir(exist_ok=True)
scenes=[('Mostrador','20_cafetera_y_materiales.png'),('Clientela','18_clientela_extravagante.png'),('Manos y mesa','12_caderas_y_manos_corregidas.png'),('Líquidos','04_liquido_y_charcos.png'),('Charcos','04b_charco_persistente.png')]
rows=[];audit=[]
for title,name in scenes:
    pair=[]
    for folder in [old,new]:
        path=folder/name;data=path.read_bytes();width,height=struct.unpack('>II',data[16:24])
        audit.append({'file':str(path.relative_to(root)),'width':width,'height':height})
        assert (width,height)==(1920,1080),path
        pair.append('data:image/png;base64,'+base64.b64encode(data).decode())
    extra=''
    lumen=root/'Evidence/Entrega11Lumen'/name
    if lumen.exists():
        data=lumen.read_bytes();width,height=struct.unpack('>II',data[16:24]);assert (width,height)==(1920,1080),lumen
        extra='<figure><figcaption>0.11 · Lumen por software (experimental)</figcaption><img src="data:image/png;base64,'+base64.b64encode(data).decode()+'"></figure>'
        audit.append({'file':str(lumen.relative_to(root)),'width':width,'height':height})
    rows.append(f'<section><h2>{html.escape(title)}</h2><div class="pair"><figure><figcaption>0.10 · captura anterior</figcaption><img src="{pair[0]}"></figure><figure><figcaption>0.11 · DX11 e iluminación precalculada</figcaption><img src="{pair[1]}"></figure>{extra}</div></section>')
report=json.loads((new/'smoke_result.json').read_text(encoding='utf-8-sig'))
doc='''<!doctype html><html lang="es"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Cero Estrellas · Comparativa 0.11</title><style>body{background:#102421;color:#f3e4c8;font:16px system-ui;margin:28px}h1{font-size:30px}h2{font-size:20px;color:#e5b976}.pair{display:grid;grid-template-columns:1fr 1fr;gap:16px}figure{margin:0}img{width:100%;cursor:zoom-in}img:active{position:fixed;inset:0;margin:auto;width:auto;max-width:100vw;max-height:100vh;z-index:2}figcaption{padding:8px;background:#1d3530}p{max-width:960px;line-height:1.6}@media(max-width:1000px){.pair{grid-template-columns:1fr}}</style><h1>Cero Estrellas · Revisión gráfica 0.11</h1><p>Capturas reales a 1920 × 1080. Mismos escenarios automatizados; el instante de animación, las salpicaduras y algunos visitantes pueden variar. Mantén pulsada una imagen para verla ampliada.</p>'''
doc+=''.join(rows)
metrics=[]
for label,folder,filename in [('0.11 · DX11',new,'smoke_result.json'),('0.11 · Lumen',root/'Evidence/Entrega11Lumen','smoke_result.json'),('0.11 · DX11 con diálogo',root/'Evidence/Entrega11Dialogue','dialogue_result.json')]:
    data=json.loads((folder/filename).read_text(encoding='utf-8-sig'))
    resources=json.loads((folder/'resources_summary.json').read_text(encoding='utf-8-sig'))
    metrics.append(f'<tr><td>{label}</td><td>{data["frame_p95_ms"]:.2f} ms</td><td>{data["frame_max_ms"]:.2f} ms</td><td>{resources["total_desktop_gpu_peak_mib"]} MiB</td></tr>')
doc+='<h2>Mediciones de esta entrega · GTX 1060 6 GB</h2><table cellpadding="10"><thead><tr><th>Escenario</th><th>p95</th><th>Máximo</th><th>VRAM de todo el escritorio</th></tr></thead><tbody>'+''.join(metrics)+'</tbody></table><p>Escenarios ejecutados por separado, con límite de 60 FPS. El diálogo usa CPU. La VRAM incluye otras aplicaciones y se muestrea cada dos segundos; no es memoria exclusiva del juego ni garantiza recoger todos los picos.</p>'
doc+=f'<p>Comprobaciones del paquete: {"aprobadas" if report["passed"] else "fallidas"}. Fotograma p95: {report.get("frame_p95_ms",0):.2f} ms. Las pruebas automáticas y estas capturas no sustituyen una partida humana.</p></html>'
(out/'Comparativa.html').write_text(doc,encoding='utf-8')
(root/'Evidence/entrega11_resolution_audit.json').write_text(json.dumps(audit,indent=2),encoding='utf-8')
print(out/'Comparativa.html')
