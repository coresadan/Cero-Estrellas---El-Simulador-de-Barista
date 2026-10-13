> Informe histórico de la versión 0.7. La revisión actual está en [POLISH_08.md](POLISH_08.md).

# Revisión jugable y visual 0.7

## Personajes y corrección de las caderas

La regla anterior de pesos clasificaba por altura y distancia al centro sin distinguir materiales. Algunos vértices de la zona exterior del pantalón quedaban vinculados a huesos de brazos o manos. La regla nueva trata primero pantalones y zapatos; esas superficies no pueden recibir influencias de brazos. Cada personaje tiene 6599 vértices de pantalón. La prueba de Blender mueve brazos y antebrazos, comprueba que las manos se desplazan y exige que esos vértices del pantalón permanezcan inmóviles y que los pesos sumen uno: [prueba de deformación](../Evidence/entrega07_deformation.json).

Los tres cuerpos originales se han regenerado con hombros más separados de la cadera y con antebrazo, palma y dedos unidos en la superficie de piel. Ojos más pequeños y siluetas de ropa más contenidas. Las mallas tienen entre 19731 y 19956 vértices. Se conservan las fuentes editables y la dirección artística caricaturesca.

`CEBodyAnimation` transfiere las poses locales a un proxy de animación mediante una copia en `PreUpdate`, antes de la evaluación en los hilos de animación. `CECustomerRig` usa ahora `USkeletalMeshComponent`. El importador crea en cada paquete un Physics Asset con cápsulas y articulaciones, ligado a los huesos del modelo.

Las caídas habilitan cuerpos Chaos reales y aplican un impulso al pecho. Se permiten como máximo dos clientes en caída física simultánea; un tercero conserva una reacción animada. La recuperación captura la pose física y vuelve a la animación en 0,8 segundos. Un impacto pequeño puede simular solo la cabeza con un controlador de orientación durante 0,75 s. Las colisiones propias del rig están desactivadas para mantener estable este esqueleto de comedia.

## Preparación y servicio

Se añaden tres estaciones accesibles: azúcar, sirope y espumador. F añade dos gramos de azúcar o una pulsación de seis mililitros de sirope. El sirope ocupa capacidad de la taza y se registra dentro del agua de la mezcla. El espumador requiere al menos 30 ml de leche a 50 C y trabaja durante 1,5 s. La lanza de vapor también crea una capa menor de espuma cuando la leche se calienta.

La espuma es una representación acotada del aire incorporado, no volumen líquido adicional. Se visualiza como una superficie sobre la bebida. Espuma, azúcar y sirope viajan proporcionalmente con los segmentos de líquido al verter; el recipiente conserva los extras restantes. Se registran únicamente los extras de la porción que cabe en el recipiente de destino. Las comandas pueden exigir uno de esos extras y el servicio los comprueba.

El prensador se mueve durante 0,55 s, el portafiltro gira al encajar y los controles del vapor y las bombas muestran su actividad. Preparar un desayuno correcto genera confianza; agredir después a ese cliente da un bonus viral una sola vez al consumir esa confianza.

## Restaurante y acontecimientos

Los clientes sospechosos apartan su taza o portátil cuando te acercas con líquido. Un cliente esperando en el mostrador puede retroceder. Los testigos comentan el desastre entre ellos si están cerca y tienen línea de visión; cada intercambio tiene pausa y no se ejecuta mientras huyen o están caídos.

La viralidad activa hasta tres acontecimientos por turno: visita del crítico, experiencia agresiva y degustación corporativa. El primero necesita 1000 puntos y no empieza antes de 150 s; los siguientes necesitan 2500 y 4000 puntos, con al menos 90 s entre acontecimientos. Cada uno afecta a las nuevas visitas durante 80 s. Los críticos son más exigentes; quienes buscan la experiencia agresiva toleran mejor los incidentes; los visitantes de la degustación consumen el desayuno más deprisa.

Hay tres llamadas nuevas grabadas con la voz de la jefa. Usan su pausa de al menos 50 s y se descartan si la radio está ocupada: no añaden una cola ni comentarios por cada acción.

## Interfaz y conversación

Tres comandas físicas sobre la barra se actualizan durante el turno. La interfaz habitual conserva un resumen pequeño, subtítulos e interacción contextual. Mantener Tab muestra los pedidos; F1 muestra u oculta la ayuda completa. Mirar a un cliente muestra únicamente su comanda.

Frases exactas habituales, como `qué has pedido`, `gracias`, `perdón`, `lo siento` o `dónde estamos`, reciben una respuesta autorada coherente con pedido y estado. Se construye y valida el mismo contrato JSON antes de aplicarla. Se cuentan como `ServiceReplies` y se registran con `CE_SERVICE_REPLY`, separados de las respuestas del modelo; no son respuestas generadas ni sustitutos ante un fallo de HTTP. La conversación libre continúa por el modelo y mantiene los papeles de cliente y camarero.

El prompt se ha reducido, pide hasta 80 caracteres y selecciona cuatro incidentes recientes. La memoria queda acotada a ocho intercambios y 1800 caracteres por cliente, con 24 identidades. El schema conserva un máximo de 100 caracteres de texto y la validación completa antes de reproducir la voz o aplicar metadatos.

El servicio de voz permite transcripción y síntesis concurrentes con un bloqueo independiente por modelo. La síntesis tiene caché LRU en RAM, máximo 64 frases y 16 MiB, sin guardar audios del jugador. Las respuestas incluyen `X-Voice-Cache` para comprobar aciertos reales. Los 24 clips de reacciones y ambiente son recursos propios generados previamente.

## Validación de esta entrega

Código compilado en Unreal 5.8.3 y paquete Windows actualizado. El ejecutable archivado coincide por SHA-256 con el compilado y es posterior a todas las fuentes C++.

- [Turno completo de ocho minutos](../Evidence/Entrega07Playable/smoke_result.json): 99 comprobaciones correctas, con reloj real, antes de los últimos ajustes de cápsula y voz pendiente.
- [Regresión del paquete final](../Evidence/Entrega07FinalQuick/smoke_result.json): 96 comprobaciones correctas. Incluye desactivar las consultas de la cápsula de pie durante la caída articulada.
- [Deformación de los tres personajes](../Evidence/entrega07_deformation.json): ningún vértice de pantalón se desplaza por mover los brazos; pesos normalizados.
- [Diálogo real del paquete final](../Evidence/Entrega07Dialogo/dialogue_result.json): 40 comprobaciones correctas, ocho respuestas generadas validadas, cuatro voces generadas reproducidas y dos respuestas autoradas de servicio. Una voz autorada se reproduce y otra se descarta por un golpe durante la síntesis; se verifica un acierto real de caché. El dispositivo de micrófono se abre y Whisper reconoce el WAV de prueba, con confirmación antes del envío.

Se han inspeccionado las capturas del ejecutable final: [caderas y manos](../Evidence/Entrega07FinalQuick/12_caderas_y_manos_corregidas.png), [extras](../Evidence/Entrega07FinalQuick/13_latte_espuma_y_extras.png), [defensa del desayuno](../Evidence/Entrega07FinalQuick/14_cliente_protege_desayuno.png), [caída articulada](../Evidence/Entrega07FinalQuick/15_caida_fisica_real.png) y [local y personajes en movimiento](../Evidence/Entrega07FinalQuick/16_eventos_y_comandas.png). Son escenas de validación preparadas; no un recorrido humano. La caída convive con sillas y mesas y puede acabar en poses exageradas propias del estilo.

El turno completo registró p50 y p95 de 16,67 ms y un máximo de 162,01 ms. Su muestreo de recursos alcanzó 3703 MiB de GPU sobre 6144 disponibles y 542,3 MiB de memoria residente del juego; durante parte de ese recorrido se compilaba en segundo plano. La regresión final, sin compilación simultánea, registró p95 de 16,67 ms, pico de GPU de escritorio de 2691 MiB y 534,1 MiB residentes del juego. [Recursos de la regresión](../Evidence/Entrega07FinalQuick/resources_summary.json). La GPU incluye escritorio y otras aplicaciones, y el muestreo cada dos segundos puede omitir picos breves. El límite de 60 FPS no garantiza ese rendimiento en todas las escenas.

Las pruebas de acontecimientos preparan directamente viralidad y tiempos para cubrir las tres variantes; no demuestran su equilibrio durante una partida espontánea.

[Medición de conversación](../Evidence/entrega07_dialogue_latency.json), mismo equipo y modelo que 0.6: mediana de cinco turnos comparables de 19,18 a 17,19 s, reducción observada del 10,4 %. Las respuestas posteriores muestran texto entre 4,53 y 10,41 s y terminan validadas entre 12,61 y 22,88 s. La longitud de salida y la carga del sistema varían; no implica una mejora garantizada en cada turno. Los atajos autorados evitan la inferencia para esas frases exactas; su voz sigue dependiendo de síntesis o caché.

Durante el escenario de diálogo, p95 de fotograma fue 22,90 ms, máximo 142,66 ms; GPU de escritorio hasta 2688 MiB y memoria residente del juego hasta 547 MiB. [Recursos con diálogo](../Evidence/Entrega07Dialogo/resources_summary.json). El modelo se comprobó cargado en CPU con `size_vram:0`; estas cifras de memoria residente del juego no incluyen el proceso del modelo ni el servicio de voz.

Se revisaron también las capturas de [conversación](../Evidence/Entrega07Dialogo/01_dialogar_respuesta_real.png) y [respuesta autorada](../Evidence/Entrega07Dialogo/05_respuesta_de_servicio.png). La validación estructural y los casos de identidad no garantizan toda la semántica: en la respuesta al derrame el cliente dijo «Fue el café, disculpa. No fue intención», una disculpa incongruente con su papel de víctima. Queda como límite observado de la conversación generada, no como comportamiento correctamente resuelto por estas pruebas.

[Manifiesto del ejecutable y evidencias](../Evidence/entrega07_delivery.json).

Los recorridos automáticos ejercitan preparación, extras, rechazo y aceptación, confianza, interacción social, radio, cuerpos articulados, recuperación, voz, contratos y cancelación. Sus informes conservan `human_playthrough:false`: quedan pendientes la valoración humana de diversión y ergonomía y una conversación real por micrófono. La locomoción sigue siendo procedural; no es captura de movimiento. Los líquidos siguen usando el sistema acotado de segmentos y manchas, sin simulación volumétrica.
