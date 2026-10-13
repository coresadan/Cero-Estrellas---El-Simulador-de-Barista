> Informe histórico de la versión 0.6. La entrega actual y sus resultados están en [POLISH_07.md](POLISH_07.md).

# Revisión jugable y visual 0.6

## Cambios integrados

- Tres cuerpos originales con esqueleto: Crypto, Hipster e Influencer. Dieciocho huesos autorados, pesos interpolados en codos y rodillas, ropa y piel continuas, zapatos y cabello. Unreal puede añadir un nodo raíz FBX. Animación procedural en `CECustomerRig.cpp`, con poses de caminar, sentarse, levantarse, caer, gesticular, beber y grabar con móvil.
- Cara vinculada al desplazamiento del hueso de cabeza. Parpadeo, cejas según estado y apertura de boca desde la energía PCM, calculada a 30 muestras por segundo para Piper. Las frases grabadas usan una apertura aproximada por duración.
- Dos manos con dedos curvados, antebrazos y mangas. La muñeca sigue el cuerpo físico real; los agarres de taza, portafiltro, jarra, bandeja, silla y portátil tienen posiciones diferentes. La bandeja utiliza ambas manos. Las manos son geometría original con articulación procedural; no un rig de dedos independiente.
- Ropa mojada: cantidad y tipo de líquido recibidos, parche en coordenadas anteriores a la deformación que oscurece la tela y reduce su rugosidad. Sigue al cuerpo sin una esfera flotante. Vapor de recipientes calientes en un pool de 24 instancias, además de los 192 segmentos y 32 charcos existentes.
- Una caída impulsa objetos cercanos y los habilita para generar impactos secundarios. Se registra una consecuencia de cadena como máximo cada seis segundos por víctima. Un cruasán arrojado se puede quedar pegado ocho segundos y continúa siendo recuperable con E.
- La influencer graba incidentes que presencia con línea de visión. Un cliente sentado y molesto intenta apartar su portátil si viertes cerca, con doce segundos entre intentos.
- Turno de ocho minutos: apertura (0–90 s), hora punta (90–240), viralidad (240–420) y último minuto (420–480). Llegadas más frecuentes y pedidos con temperatura mínima de 60 C o cantidades mínimas de 55/170 ml. Comandas indican esas exigencias y `Serve` las comprueba. Termina con resultados y un ascenso; sigue en modo libre hasta R.
- La jefa conserva su enfriamiento de al menos 50 s, no acumula llamadas y selecciona preferentemente grabaciones del portátil, la huida o el desastre múltiple, sin repetir la bolsa de frases.
- Doce exclamaciones propias de clientes, grabadas previamente con Piper, y ambiente original sintetizado de máquina/calle/vajilla. Variación ligera en impactos, límite de frecuencia y máximo de dos exclamaciones simultáneas mediante Sound Concurrency. Los tres arquetipos comparten Davefx con ajustes de tono; no son tres modelos de voz distintos.
- Subtítulos contextuales y comandas de papel reemplazan los textos grandes flotantes de clientes. El editor conserva borrador, historial y estado al volver al juego.

## Conversación durante el juego

T abre el editor. Enviar devuelve el control inmediatamente mientras HTTP sigue pendiente. T vuelve al editor; Seguir jugando lo oculta sin cancelar. V comienza o detiene la captura explícita, incluso mientras te mueves. Al transcribir una grabación en movimiento se abre el editor para revisión; nunca se envía sin confirmar. Escape, alejamiento de más de 850 cm, salida del cliente o reinicio cancelan la conversación.

Las palabras se muestran progresivamente, pero solo JSON completo y validado modifica emoción y se convierte a voz. El emisor espacial sigue la cabeza. Una agresión al cliente interrumpe su voz y cancela la inferencia pendiente sobre hechos anteriores. Las reacciones grabadas son respuestas físicas autoradas, distintas del diálogo generado. No se promete inferencia instantánea: continúa usando CPU y el mismo modelo validado.

## Fuentes y reproducción

Objetos: `Scripts/model_cafe.py`. Personajes y agarre: `Scripts/author_characters.py`, `SourceMeshes/CeroEstrellas_Personajes.blend`, FBX `SK_*` y `SM_GripHand.obj`. Son geometrías originales; no se han comprado ni descargado modelos humanos.

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --python Scripts/author_characters.py
& '.\Tools\LocalAI\Python\Scripts\python.exe' Scripts/generate_polish_audio.py
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/build.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/build_content.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/smoke_test.ps1 -Quick -EvidenceName Entrega06Editor
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/package.ps1 -IterativeCook
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/smoke_test.ps1 -Packaged -EvidenceName Entrega06Playable
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/dialogue_test.ps1 -Packaged -EvidenceName Entrega06Dialogo
```

Para refrescar únicamente los recursos nuevos, ejecutar `Scripts/import_polish_content.py` como commandlet Python de Unreal. La importación FBX usa la ruta clásica, conserva los nombres de hueso y asigna los materiales por sección. El script de contenido completo incluye esta importación.

## Límites y validación

Son figuras estilizadas animadas proceduralmente sobre una malla esquelética, no humanos fotorrealistas ni captura de movimiento. Las caídas todavía son poses que empujan Chaos, no ragdolls físicos del cuerpo. Los clientes usan recorridos definidos y evitación por barridos. Los líquidos mantienen volumen y mezcla sin simulación volumétrica. Los sorbos consumen líquido gradualmente; no existe simulación de deglución.

Las pruebas automáticas ejercitan recetas, controles, voz, contratos, movimiento durante una conversación, cuerpos esqueléticos, vapor acotado, consecuencias de caída, grabación de testigos y las fases del turno con reloj real. La opción Quick omite solo la espera hasta el final del turno. Sus informes mantienen `human_playthrough:false`: diversión, ergonomía y escucha de las voces requieren una partida humana.

El escenario empaquetado `Entrega06Playable` ha superado 72 comprobaciones con el reloj real de ocho minutos, incluidas las cuatro fases, exigencias de los pedidos, líquidos, servicio, caídas e impactos secundarios. Informe: [smoke_result.json](../Evidence/Entrega06Playable/smoke_result.json). Capturas: [cliente y pedido](../Evidence/Entrega06Playable/03_pedido_y_clientes.png), [testigo grabando](../Evidence/Entrega06Playable/07b_influencer_graba.png) y [desastre encadenado](../Evidence/Entrega06Playable/09_directo_del_desastre.png).

Se midieron 28.234 fotogramas: mediana y percentil 95 de 16,67 ms, con un máximo de 136,80 ms. Estos datos corresponden al escenario automatizado a 1280 × 720, limitado a 60 FPS; no garantizan el mismo resultado en cualquier escena. En 236 muestras cada dos segundos, el pico total de GPU del escritorio fue 5001 MiB de 6144 MiB y el working set del juego 542,8 MiB. La lectura GPU incluye las demás aplicaciones y puede omitir picos entre muestras: [recursos](../Evidence/Entrega06Playable/resources_summary.json). El modelo cargado usa CPU, con `size_vram:0`: [registro](../Evidence/entrega06_cpu_model.json).

Se corrigió también el lanzamiento en frío: los servicios y la precarga tienen entrada y salidas redirigidas para que la consola de `Jugar.bat` pueda terminar y abrir el juego. `stop_ai.ps1` identifica tanto el lanzador Python como su proceso real, la precarga y los runners `llama-server.exe` por la ruta de este proyecto. Se eliminaron tres runners huérfanos que ocupaban unos 7,5 GB entre ellos; el cierre actualizado evita esa acumulación. El arranque de los servicios devolvió el control en 4,17 s; la precarga de modelos continúa en segundo plano y esa cifra no es el tiempo hasta obtener una respuesta.

La corrección final de cámara y muñecas se compiló para editor y ejecutable y pasó otras 68 comprobaciones rápidas: [informe](../Evidence/Entrega06FinalQuick/smoke_result.json), [agarre de jarra](../Evidence/Entrega06FinalQuick/02b_jarra_verter_a_mano.png) y [vista del desastre corregida](../Evidence/Entrega06FinalQuick/09_directo_del_desastre.png). Esta regresión omite la espera hasta el cierre; la validación de las cuatro fases corresponde al escenario completo anterior. Después solo cambió la comprobación automatizada de V para esperar al tick que procesa el teclado.

La conversación real del paquete final superó **37 comprobaciones**, con ocho respuestas estructuradas validadas y tres reproducciones Piper observadas. Se comprobaron el pedido, restaurante y papel de camarero, conversación libre, memoria, receptor del impacto, movimiento durante HTTP, cancelación por agresión nueva, transcripción editable y apertura del micrófono mediante V sin editor modal. La captura se abrió en el dispositivo real; la transcripción de contenido se verifica con un WAV de prueba, no con una conversación humana: [informe](../Evidence/Entrega06Dialogo/dialogue_result.json), [respuesta real](../Evidence/Entrega06Dialogo/01_dialogar_respuesta_real.png) y [contexto físico](../Evidence/Entrega06Dialogo/02_memoria_y_contexto_fisico.png). Ninguno de los dos logs finales contiene errores fatales ni fallos de compilación de materiales.

En los siete turnos siguientes, las primeras palabras tardaron **4,29–9,17 s** y el JSON completo **10,80–20,18 s**. El primer turno tardó 15,08 s hasta las palabras y 22,37 s hasta validar. La mejora de esta revisión es recuperar el control al enviar y tener reacciones físicas grabadas inmediatas; no demuestra una aceleración de la inferencia. La comparación conservada con el modelo 3B de la entrega 0.4 resulta más lenta en generación completa, con otro modelo y respuestas de longitud variable: [latencias](../Evidence/entrega06_dialogue_latency.json).

Durante la conversación se registraron 8352 fotogramas: mediana de 16,67 ms, percentil 95 de 21,48 ms y máximo de 140,65 ms. Las 74 muestras de recursos dieron un pico GPU total de escritorio de 4485 MiB y working set del juego de 526,6 MiB, con las mismas limitaciones del muestreo: [recursos durante diálogo](../Evidence/Entrega06Dialogo/resources_summary.json).

El ciclo final de apagado y arranque pasó: cero procesos propios restantes tras detener, una única instancia del modelo al arrancar, voz preparada y cero bytes de modelo en GPU. El lanzador devolvió el control en 4,49 s; la preparación continúa en segundo plano: [ciclo de servicios](../Evidence/entrega06_service_lifecycle.json). El paquete final está en `Release/Windows/CeroEstrellas.exe` y se abre mediante `Jugar.bat`; [compilación y archivo finales](../Evidence/entrega06_qa_final_package_build.log).

El [registro de entrega](../Evidence/entrega06_delivery.json) incluye los informes, la huella SHA-256 del ejecutable y la comprobación de que coincide con el binario compilado y es posterior a los archivos C++ actuales.
