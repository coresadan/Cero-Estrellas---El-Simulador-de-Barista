# Revisión 0.8: caras visibles, caídas y pequeñas historias

## Mostrador e interacción

Las tres comandas dejan de ser paneles verticales de 78 × 48 cm. Son hojas de 36 × 62 cm tumbadas sobre la barra, con texto orientado hacia el camarero; la vista de los clientes queda despejada. Tab conserva la consulta conjunta y F1 la ayuda.

El prensado requiere mantener F durante 0,55 s. El filtro se apoya bajo el prensador y la mano acompaña la presión. Soltar antes o alejarse interrumpe sin fabricar una dosis prensada. El filtro terminado vuelve a la mano si está libre. F durante la extracción cierra el surtidor y consume la dosis: puedes preparar deliberadamente un café corto. Se mantienen el giro del filtro y sonidos de herramienta, vapor, extracción y cerámica.

## Cuerpo y presentación

Las rodillas y los codos tienen límites de bisagra orientados a su eje de flexión, con rangos asimétricos. Torso, cabeza, hombros, caderas, muñecas y tobillos tienen rangos propios. Masas por segmento, más iteraciones del solucionador, CCD, proyección moderada y colisión entre cuerpos no adyacentes buscan conservar la silueta. Los cuerpos adyacentes no chocan entre sí. El perfil anatómico se copia explícitamente al perfil predeterminado del recurso antes de guardarlo; sin ese paso Unreal recuperaba los límites anteriores. Un contacto fuerte del ragdoll con otro cliente puede provocar otra caída, con una pausa de seis segundos por emisor. Se mantiene el máximo de dos ragdolls completos simultáneos.

La recuperación mezcla la pose física durante 1,1 s. Un ciclo de marcha con ocho claves articulares sigue la distancia recorrida; el personaje bloqueado no continúa caminando sobre el sitio. Las piernas ajustan el contacto con el suelo mediante una solución de dos huesos. Los brazos alcanzan la mesa o acercan la taza a la boca. La limpieza de una mancha tiene un gesto propio y un cliente mojado puede apartar ligeramente su silla.

Se regeneran los tres cuerpos originales con menor reducción de polígonos en piel y ropa: 27951–28337 vértices. Se conservan los pantalones sin pesos de brazos y la cintura de la camisa mezcla pelvis y columna. Detalles de ropa siguen el pecho mediante sockets. Aumenta ligeramente la luz de relleno para leer mejor las caras. La madera tiene relieve y brillo más moderados.

Nueve perfiles de apariencia y conversación: cripto-bro, hipster, influencer, punk de oficina, astronauta de fin de semana, diva de la ópera, motorista, abuela rave y mago sin contrato. Usan tres cuerpos base, nueve combinaciones de piel, cabello y ropa y accesorios distintos; no son nueve mallas anatómicas nuevas. Las llegadas recorren los perfiles en una ronda.

## Historias opcionales

Cada visitante habitual recibe una de cinco situaciones: entrevista, cita a ciegas, crítico encubierto, audición o poema para un ex. La situación se presenta al llegar, puede consultarse conversando y se muestra al mirar al personaje.

Ayudar requiere servir su desayuno y darle apoyo o una sugerencia durante la conversación. Un insulto reconocible o un incidente de gravedad suficiente arruina la situación. La salida sin resolver también cuenta como fracaso. Cada historia se resuelve una sola vez, deja una reacción contextual y da 80 puntos al ayudar o 160 al sabotear. Estos desenlaces no activan por sí mismos la radio de la jefa.

El contexto de conversación incluye situación, apoyo recibido y desenlace. El personaje conserva su personalidad extravagante además de la historia y el pedido. Las preguntas habituales sobre la historia y las frases cortas de ánimo pueden usar respuestas autoradas inmediatas; la conversación libre sigue usando el modelo. Se añaden variantes frecuentes de pedido, agradecimiento y disculpa.

Tras una agresión, el prompt especifica que la disculpa pertenece al camarero. Algunas atribuciones explícitas incorrectas se rechazan y se reformulan una vez. Esta protección no demuestra coherencia semántica completa.

El resumen de cierre conserva prioridad durante 18 s después del turno. Una historia que se resuelve en ese intervalo mantiene sus contadores y reacción, pero no sustituye el aviso de despido denegado.

## Validación

Cierre del 13 de octubre de 2026 con Unreal Engine 5.8.3. Compilación de juego y editor y empaquetado correctos: [registro de compilación](../Evidence/entrega08_delivery_package_build.log). El contenido se regeneró, importó y cocinó durante esta revisión; el último empaquetado reutiliza ese contenido porque solo cambió C++ de cierre de turno y de preparación del escenario de prueba.

El [recorrido final del ejecutable](../Evidence/Entrega08Delivery/smoke_result.json) pasa **116/116 comprobaciones**, incluidos los ocho minutos reales de turno, receta y extras, prensado interrumpible, caras despejadas, rodillas, recuperación, contacto real entre ragdolls, nueve perfiles, diez desenlaces de historias y prioridad del resumen final. No hay errores fatales ni errores de compilación de materiales en el registro. La [prueba de deformación](../Evidence/entrega08_deformation.json) pasa para los tres cuerpos: mover los brazos no desplaza ningún vértice de los pantalones.

El [muestreo de recursos](../Evidence/Entrega08Delivery/resources_summary.json) obtuvo 218 muestras, sin otra instancia del juego: pico de memoria GPU del escritorio **3668/6144 MiB** y memoria residente del juego **579,5 MiB**. El recorrido registró p95 **16,67 ms** y máximo **192,82 ms**; el máximo muestra que todavía existen pausas puntuales. Estas cifras pertenecen al recorrido automatizado sin inferencia simultánea.

Los recorridos anteriores se conservan como diagnóstico: uno detectó que una historia ocultaba el resumen final y otro que el traslado de la taza del escenario derramaba sus extras. Ambos casos se corrigieron y se repitió el turno completo; esos informes anteriores fallidos no se cuentan como validación de entrega.

El [escenario final de conversación](../Evidence/Entrega08DeliveryDialogo/dialogue_result.json) pasa **40/40 comprobaciones**: ocho respuestas reales, memoria del pedido y nombre, contexto de restaurante/camarero y de café recibido, Whisper con confirmación del texto, Piper, cancelación de inferencia y descarte de voz tardía. Se contabilizan por separado dos respuestas autoradas y un acierto de caché. El dispositivo de micrófono se abrió; la transcripción comprobada procede del WAV de prueba, no de una conversación humana.

Las [latencias medidas](../Evidence/entrega08_dialogue_latency.json) fueron **5,05–12,16 s** hasta texto visible y **14,10–26,06 s** hasta validación completa en los turnos posteriores. La mediana de cinco turnos comparables fue **18,34 s**, frente a **17,19 s** en 0.7: esta medición no demuestra una mejora de velocidad del modelo. Los atajos autorados sí evitan esa espera y la conversación mantiene el movimiento disponible. Con inferencia real, p95 de fotograma **25,60 ms**, máximo **128,70 ms**; [recursos](../Evidence/Entrega08DeliveryDialogo/resources_summary.json): pico GPU del escritorio **3697/6144 MiB**, juego **553,5 MiB**. Modelo y voz usaron CPU.

Una [prueba separada del contexto narrativo](../Evidence/Entrega08Stories/story_dialogue_result.json) envió cinco consultas reales al backend, una por historia: las cinco devolvieron contrato válido y mencionaron el contexto esperado. Es una prueba directa del servicio, no cinco recorridos de diálogo dentro de Unreal ni una revisión semántica humana.

El [manifiesto de entrega](../Evidence/entrega08_delivery.json) registra rutas, comprobaciones y SHA-256. El binario empaquetado coincide con el compilado y es posterior al último cambio C++. Abre **Jugar.bat** en la raíz del proyecto.

### Capturas revisadas

- [Mostrador con comandas tumbadas](../Evidence/Entrega08Delivery/17_mostrador_despejado.png).
- [Clientela y accesorios](../Evidence/Entrega08Delivery/18_clientela_extravagante.png).
- [Latte con espuma y extras](../Evidence/Entrega08Delivery/13_latte_espuma_y_extras.png).
- [Caída articulada](../Evidence/Entrega08Delivery/15_caida_fisica_real.png) y [contacto en cadena](../Evidence/Entrega08Delivery/19_contacto_y_caida_en_cadena.png).
- [Resumen final conservado](../Evidence/Entrega08Delivery/11_despido_denegado.png).
- [Conversación](../Evidence/Entrega08DeliveryDialogo/01_dialogar_respuesta_real.png), [contexto del café recibido](../Evidence/Entrega08DeliveryDialogo/02_memoria_y_contexto_fisico.png) y [transcripción editable](../Evidence/Entrega08DeliveryDialogo/03_transcripcion_whisper.png).

### Reproducir las pruebas

Desde la raíz del proyecto, con una sola instancia del juego:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/smoke_test.ps1 -Packaged -Offscreen -EvidenceName Entrega08Delivery
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/start_ai.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/dialogue_test.ps1 -Packaged -Offscreen -EvidenceName Entrega08DeliveryDialogo
```

`-Offscreen` usa renderizado real y permite capturas, sin una ventana que pueda cerrarse accidentalmente. No usa NullRHI. Las cifras de rendimiento obtenidas así describen este escenario DX11 a 1280 × 720, limitado a 60 FPS; no garantizan el mismo resultado en una partida con ventana. `profile_playable.ps1 -OnlyEvidence -EvidenceName <nombre>` puede ejecutarse en paralelo para medir ese proceso; la memoria GPU incluye todo el escritorio y se muestrea cada dos segundos.

Las pruebas preparan algunas escenas directamente y otras usan controles simulados. La dosificación comienza con una taza física colocada en el punto de agarre y con el jugador en la posición de servicio: evita que un traslado brusco del escenario derrame los ingredientes antes de comprobar las estaciones. Las pruebas separadas de volcado, transferencia y derrames mantienen sus comprobaciones de volumen. Algunas capturas de la galería contienen objetos sobrantes del montaje de prueba; no son una partida humana limpia.

La animación conserva el sistema propio de poses y claves en C++, con ajustes de contacto; no es captura de movimiento ni una biblioteca de animaciones cinematográficas. El líquido sigue siendo una simulación acotada de segmentos y manchas. Las historias son situaciones breves con desenlaces, no árboles narrativos extensos. Los perfiles comparten la voz Piper con ajustes de tono. Una partida humana y una conversación real por micrófono siguen siendo necesarias para valorar ritmo, comedia y ergonomía.
