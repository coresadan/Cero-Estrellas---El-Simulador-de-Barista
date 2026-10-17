"""Self-contained viewer built only from actual game screenshots."""
from pathlib import Path
import base64,html,struct,json
root=Path(__file__).resolve().parent.parent
out=root/'Evidence/Comparativa12';out.mkdir(exist_ok=True)
def figure(folder,name,label):
    p=root/'Evidence'/folder/name;data=p.read_bytes()
    assert struct.unpack('>II',data[16:24])==(1920,1080),p
    return '<figure><figcaption>'+html.escape(label)+'</figcaption><img alt="'+html.escape(label)+'" loading="lazy" src="data:image/png;base64,'+base64.b64encode(data).decode()+'"></figure>'
body=''
for title,name in [('Personajes','18_clientela_extravagante.png'),('Servicio y HUD','20_cafetera_y_materiales.png'),('Mesa y cliente','12_caderas_y_manos_corregidas.png')]:
    body+='<h2>'+title+'</h2><div class="pair">'+figure('Entrega11Closure',name,'0.11 · Antes')+figure('Entrega12Closure',name,'0.12 · Ahora')+'</div>'
for title,name in [('Salón ampliado','24_salon_ampliado.png'),('Cada cliente trae sus cosas','25_clientes_y_dispositivos.png'),('Las seis comandas','26_hud_seis_comandas.png'),('Móvil físico en la mano','27_movil_en_mano.png'),('Tablet mojada','28_tablet_mojada.png')]:
    body+='<h2>'+title+'</h2>'+figure('Entrega12Closure',name,'0.12 · Captura real a 1920 × 1080')
for title,name in [('El sombrero, sin bigote','01_dialogar_respuesta_real.png'),('El portátil entra cerrado con su dueño','04_conversacion_libre.png')]:
    body+='<h2>'+title+'</h2>'+figure('Entrega12Dialogue',name,'0.12 · Captura real del diálogo y las llegadas')
doc='''<!doctype html><html lang="es"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Cero Estrellas · Revisión 0.12</title><style>body{margin:32px;background:#102421;color:#f3e4c8;font:16px system-ui}main{max-width:1600px;margin:auto}h1{font-size:32px}h2{margin-top:36px;color:#e5b976;font-size:22px}.pair{display:grid;grid-template-columns:1fr 1fr;gap:16px}figure{margin:0}img{width:100%;cursor:zoom-in}img:active{position:fixed;inset:0;margin:auto;width:auto;max-width:100vw;max-height:100vh;z-index:2}figcaption{background:#1d3530;padding:10px}p{max-width:1000px;line-height:1.6}@media(max-width:1000px){.pair{grid-template-columns:1fr}}</style><main><h1>Más salón, menos pantallas de serie</h1><p>Seis mesas, un HUD nuevo y dispositivos que llegan con sus propietarios. Capturas reales de escenarios automatizados: los movimientos, visitantes y objetos desplazados por la física pueden variar. Mantén pulsada una imagen para ampliarla.</p>'''+body
doc+='<p>Estas capturas documentan la integración y el aspecto visible. No sustituyen una partida humana ni garantizan una tasa constante de fotogramas.</p></main></html>'
(out/'Comparativa.html').write_text(doc,encoding='utf-8')
print(out/'Comparativa.html')
