# Revisión 0.10: cuerpos, mobiliario y presentación

Revisión 0.10.0. Corrige las regresiones visibles de 0.9 y amplía la presentación y las interacciones. Entrega empaquetada y verificada el 15 de octubre de 2026. El cierre técnico y sus límites se documentan abajo.

## Cambios

- El cargador resuelve las mallas `SM09_` en `/Game/Prototype/Meshes`. La revisión anterior buscaba tableros, platos, lámparas y detalles en `/Engine/BasicShapes`; comprobar que el archivo existía no demostraba que estuviera colocado en la partida.
- Portátiles orientados hacia los clientes. Sillas y platos acercados al alcance de sus brazos; manteles individuales y cubiertos pequeños, sin carteles que tapen caras.
- Nueve cuerpos originales, mandíbula y nariz continuas, distintas proporciones, cabello y hombros más contenidos. Los pesos de la manga dejan de arrastrar una región amplia del torso. Se conservan límites de física y LOD.
- Manos separadas, objetivos relativos a la altura real de la mesa, codos que doblan hacia fuera y orientación de muñecas que acompaña al antebrazo. Gestos independientes para hablar, beber, teléfono, proteger y secarse.
- Inicialización de la animación después de registrar el componente y orden de actualización explícito. La comprobación contrasta la pose calculada con los huesos evaluados. Cara y teléfono se acoplan a los sockets para seguir el mismo fotograma. El lado de cada brazo se obtiene del esqueleto importado, cuya conversión de ejes invertía los objetivos anteriores.
- Apoyo muscular breve al empezar una caída, límites articulares conservados y recuperación junto al lugar de aterrizaje, fuera de los tableros. Se mantienen dos cuerpos físicos completos como máximo.
- Luz cálida sobre mesas y relleno suave de ventana; contraste, reflejos y brillo de piel/tejidos revisados. El juego sigue usando el renderizador de Unreal 5.8.3 con DX11 y un presupuesto adecuado a 6 GB; no se ha reescrito el motor.
- Cuadernos, flores y bolsos físicos de visitantes. Devolver sus pertenencias ayuda a una historia; entregarlas a la persona equivocada o estropearlas puede arruinarla. Las pertenencias se reemplazan al ocupar de nuevo la mesa para acotar actores.
- Banco de 192 grabaciones de frases escritas, identificado por el hash UTF-8 del texto exacto. No sustituye una frase por una grabación genérica con otras palabras. Las respuestas libres conservan su síntesis de voz y validación.
- Salpicaduras de impacto reutilizadas: hasta 48 partículas visuales sin colisión, separadas de los paquetes que conservan volumen. Se mantienen la transferencia entre recipientes, las mezclas y los charcos acotados.
- Preparación anticipada de la voz al terminar el campo de texto. Solo se reproduce después de validar el JSON completo y comprobar que el turno no se ha cancelado. Nuevas preguntas de servicio e historia reciben respuestas inmediatas.

## Validación de la entrega

[Manifiesto del ejecutable](../Evidence/entrega10_delivery.json). El hash SHA-256 coincide con el binario compilado y todos los archivos C++ son anteriores al ejecutable entregado. [Compilación del editor](../Evidence/entrega10_closure_editor_build.log) y [empaquetado](../Evidence/entrega10_closure_package_build.log) correctos con UE 5.8.3. Para jugar, abrir [Jugar.bat](../Jugar.bat).

| Escenario | Resultado | Resolución real | Alcance |
|---|---|---|---|
| [Jugabilidad del paquete de cierre](../Evidence/Entrega10Closure/smoke_result.json) | 146/146 | 1920 × 1080 | Recetas, agarres, derrames, caídas, historias, manos renderizadas, colocación de portátiles y materiales. Recorrido rápido, sin esperar al final del turno. |
| [Diálogo del mismo ejecutable](../Evidence/Entrega10DialogueVerified/dialogue_result.json) | 43/43 | 1920 × 1080 | Ocho respuestas generadas reales, voz Piper, memoria, perspectiva, contexto físico, transcripción editable y cancelación. |
| [Turno completo previo](../Evidence/Entrega10DeliveryFinal/smoke_result.json) | 151/151 | 1280 × 720 | Ocho minutos hasta «Despido denegado». Anterior a los últimos ajustes de contacto de manos, movilidad de luces y guardas de diálogo; no se presenta como prueba del ejecutable definitivo. |

La prueba de diálogo cuenta aparte dos respuestas autoradas de servicio y ocho preparaciones anticipadas de voz. La corrección del pedido rechazó una respuesta y utilizó su único reintento; la respuesta aceptada fue «He pedido un latte con cruasán». Tras el derrame respondió «Me diste café en la camisa. Discúlpate». El dispositivo de micrófono se abre y cierra; Whisper procesa un WAV de prueba y su transcripción solo se envía después de confirmar. No se ha verificado una conversación humana por micrófono.

[Deformación](../Evidence/entrega10_deformation.json): nueve cuerpos aprobados, pesos normalizados y cero desplazamiento de pantalón al mover brazos. [LOD](../Evidence/entrega10_lods.json): dos niveles por cuerpo, unos 35–37 mil vértices de cerca y 13 mil a distancia. [Grabaciones](../Evidence/entrega10_voice_audit.json): 192 WAV y recursos importados asociados al hash del texto exacto. Son auditorías complementarias; las comprobaciones de huesos renderizados y voz exacta también se ejecutan dentro del paquete.

## Rendimiento medido y límites

GTX 1060 de 6 GB, Ryzen 2700X, 24 GB de RAM. Ambos escenarios finales se ejecutaron por separado a 1080p, sin otra instancia del juego, con renderizado sin ventana visible y límite de 60 FPS. [Resoluciones de los PNG](../Evidence/entrega10_resolution_audit.json): 23 capturas del recorrido y cinco del diálogo, todas de 1920 × 1080.

| Escenario final | Fotograma p95 | Fotograma máximo | Pico VRAM de todo el escritorio | Pico RAM del juego |
|---|---|---|---|---|
| [Jugabilidad](../Evidence/Entrega10Closure/resources_summary.json) | 16,67 ms | 300,66 ms | 4036 / 6144 MiB | 604,6 MiB |
| [Conversación con inferencia y voz](../Evidence/Entrega10DialogueVerified/resources_summary.json) | 26,42 ms | 281,52 ms | 4094 / 6144 MiB | 599,1 MiB |

Los máximos incluyen pausas puntuales: no se garantiza una cadencia constante de 60 FPS. La VRAM incluye Windows y otras aplicaciones; la RAM mostrada solo incluye los procesos del juego, no los servicios de voz y lenguaje. El muestreo cada dos segundos puede perder picos breves. Estos datos no sustituyen una medición jugando en una ventana normal ni una partida humana.

[Líquido comparado a 60/30 Hz](../Evidence/Entrega10Closure/liquid_comparison.json): el mismo caso de 32 emisiones deposita 64 ml en ambos escenarios. Las consultas bajan de 1120 a 544, un 51,4 %, con 0,92 frente a 0,47 ms de CPU acumulados en ese caso. Compara el mismo solucionador a dos frecuencias; no es una comparación completa contra otra versión ni contra un simulador de fluidos.

[Latencias de conversación](../Evidence/entrega10_dialogue_latency.json): en los turnos posteriores, el primer texto aparece en 4,88–19,11 s y la respuesta validada en 12,97–28,92 s, incluyendo el reintento cuando existe. La mediana de cinco turnos comparables fue 17,78 s frente a 15,55 s en 0.9: esta medición no demuestra una mejora de velocidad del lenguaje. Los atajos autorados evitan esa espera y la preparación anticipada de voz aprovecha el tiempo mientras llega la respuesta. El [estado del backend](../Evidence/entrega10_backend_health.json) confirma el modelo sin asignación de VRAM y la voz en CPU con un hilo de cálculo.

## Capturas revisadas

[Manos, madera y orientación del portátil](../Evidence/Entrega10Closure/12_caderas_y_manos_corregidas.png), [chorro al verter](../Evidence/Entrega10Closure/04_liquido_y_charcos.png), [caída articulada y taza liberada](../Evidence/Entrega10Closure/15_caida_fisica_real.png), [nueve visitantes](../Evidence/Entrega10Closure/18_clientela_extravagante.png), [historia y campaña de la jefa](../Evidence/Entrega10Closure/22_caida_y_campana.png) y [respuesta al derrame](../Evidence/Entrega10DialogueVerified/02_memoria_y_contexto_fisico.png).

## Incidencias detectadas y corregidas

- El primer recorrido `Entrega10Delivery` se detuvo porque el secado visual alteraba `WetMl`, el registro de líquido recibido. Se retiró esa modificación: el dato conserva los hechos físicos. Las ejecuciones fallidas se conservan como diagnósticos y no cuentan como entregas aprobadas.
- La inversión de ejes del FBX enviaba ambas manos al lado contrario del torso. El objetivo ahora utiliza el signo real del hombro importado. La prueba contrasta los huesos renderizados, sus longitudes y la distancia a la superficie física de la mesa; no comprueba solamente valores calculados.
- Una respuesta libre invirtió quién pide el desayuno: «Tú pides, no yo. Mi desayuno es tuyo». Se añadió ese caso a la regresión y al rechazo de perspectiva, conservando un único reintento con contexto factual. Es una validación de contradicciones explícitas, no un detector semántico universal.
- Los últimos cambios de movilidad del decorado y los focos oscurecían las paredes. Se retiraron después de comparar las capturas: este local se construye en ejecución y usa iluminación dinámica. Las carpetas `Entrega10FinalGraphics` y `Entrega10ReleaseVerified` conservan esos diagnósticos; no es la referencia gráfica de cierre.
- Algunas ejecuciones antiguas solicitaban 1080p pero Unreal reducía la ventana. Las pruebas sin ventana visible ahora añaden `-ForceRes`; el tamaño se comprueba en los PNG. `Entrega10Dialogue1080` y `Entrega10Contact1080` no sirven como mediciones a 1080p, aunque sus comprobaciones funcionales son útiles.
- La prueba de orientación se centra en el portátil del cliente recién llegado. Un portátil golpeado por las etapas anteriores puede girarse físicamente sin haber sido lanzado; su orientación posterior al sabotaje no valida cómo se coloca al llegar. `Entrega10ReleaseVerified` detectó esa mezcla de escenarios y se conserva como ejecución fallida.
- Piper limita su sesión ONNX a un hilo de ejecución y uno de coordinación, sin espera activa, para reducir la competencia con el juego y el modelo de diálogo. Esto no implica por sí solo una mejora de latencia del lenguaje.

## Reproducir y límites

`Scripts/author_characters.py` regenera los nueve cuerpos; `generate_authored_speech10.py` produce el banco de frases con el modelo de voz instalado. `import_polish10.py` importa cuerpos, LOD, grabaciones y materiales. `import_speech10.py` añade grabaciones que falten sin regenerar geometría. La compilación precede a la importación para disponer de las funciones de preparación del esqueleto y la física.

Los personajes conservan una dirección estilizada y poses procedurales; no usan captura de movimiento. Las voces comparten un modelo base con ajustes de tono. Las frases dinámicas fuera de la conversación que no tengan grabación exacta quedan subtituladas. La preparación anticipada de voz no cambia la velocidad del modelo de lenguaje ni autoriza a reproducir una respuesta incompleta. La revisión no constituye una partida humana ni una conversación humana por micrófono.

## Comandos del cierre

Desde la raíz, después de compilar y empaquetar:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/smoke_test.ps1 -Packaged -Quick -Offscreen -ResX 1920 -ResY 1080 -EvidenceName Entrega10Closure
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/start_ai.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/dialogue_test.ps1 -Packaged -Offscreen -ResX 1920 -ResY 1080 -EvidenceName Entrega10DialogueVerified
```

Ejecutar cada escenario por separado. Para repetir el turno de ocho minutos, omitir `-Quick` y usar otro nombre de evidencia. `-Offscreen` renderiza la escena sin mostrar la ventana; no equivale a una prueba humana en una ventana normal. Los escenarios incluyen preparación directa de situaciones y controles simulados, además de peticiones HTTP reales en el de diálogo.
