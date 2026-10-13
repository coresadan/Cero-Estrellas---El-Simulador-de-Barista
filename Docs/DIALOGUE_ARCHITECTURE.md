# Conversación 0.8

`ACEPlayer` abre `UCEDialogueComponent` con T sobre un cliente. El editor Slate captura la entrada mientras escribes; Enviar vuelve al control del personaje sin cancelar HTTP. Seguir jugando oculta el editor conservando borrador y turno; T lo recupera. Escape termina la conversación y cancela captura, voz y peticiones. El procesador Slate permanece durante la conversación para que Escape funcione también fuera del editor.

V inicia o detiene la grabación explícita durante el movimiento. Al recibir esa transcripción se abre el editor para revisión; nunca se envía automáticamente. Máximo quince segundos de micrófono, memoria volátil y cero grabaciones persistidas. Una distancia mayor de 850 cm, salida del cliente o reinicio cierran la conversación. Mientras espera, el cliente hace un gesto de atención; los subtítulos progresivos permiten seguir preparando desayunos.

## Petición a Ollama

Unreal usa `POST http://127.0.0.1:11434/api/chat` con `stream:true`, `think:false` y un JSON Schema en `format`. El mensaje de sistema sitúa al NPC como cliente del restaurante Cero Estrellas y al jugador como su camarero. Contiene arquetipo, contrato y pedido; permite hablar de cualquier tema y pide responder todas las preguntas en hasta 80 caracteres, sin añadir preguntas. Un ejemplo neutro de identidad refuerza los roles; no contiene café derramado ni insultos que puedan contaminar otros temas.

El mensaje `user` distingue con etiquetas el estado del cliente de la frase literal del camarero. Solo se incluyen incidentes atribuidos a la identidad del NPC: derrames globales no se describen como agresiones recibidas. Identidad y pedido no se duplican en cada turno; los estados vacíos o falsos se omiten, excepto la lista de incidentes, que declara explícitamente que no ha ocurrido ninguno. Cuando el cliente ha recibido incidentes, una nota `system` después del mensaje refuerza que él es la víctima y que debe contestar, sin repetir la frase del camarero.

El historial del prompt contiene solo las palabras del camarero y las respuestas del cliente, con sus roles. Los hechos actuales se envían una vez, sin repetir estados, notas de perspectiva ni metadatos JSON antiguos. Se mantienen hasta ocho intercambios y 1800 caracteres de texto combinado, retirando los más antiguos cuando se alcanza un límite. Esto reduce la evaluación de contexto y evita renovar un prefijo largo al séptimo turno de una conversación breve. El JSON original validado se conserva internamente para recuperar la última respuesta al reabrir el panel. Ejemplo de hechos después de un incidente:

```json
{
  "animo_actual": "molesto",
  "impactos_recibidos": 2,
  "incidentes_recibidos": [
    { "accion": "customer_hit", "objeto": "Café", "gravedad": 0.5 }
  ]
}
```

El schema separa palabras y reacción en dos objetos cerrados. `dialogo` se genera antes de `reaccion`: las palabras pueden aparecer antes de que el modelo termine los metadatos. Se conserva el schema tras observar errores de nombres de campos con el modo JSON libre; evita una segunda inferencia por esos errores. Respuesta dentro de `message.content`:

```json
{
  "dialogo": {
    "texto": "Acepto tus disculpas, pero mi camisa no es un filtro V60."
  },
  "reaccion": {
    "emocion": "enfadado",
    "cambio_enfado": -0.1,
    "quiere_irse": false
  }
}
```

El schema exige texto de 1–100 caracteres, una emoción del enum, un cambio de enfado de `-0.1`, `0`, `0.1`, `0.2` o `0.3`, y un booleano de salida. C++ exige los dos objetos y sus campos exactos; normaliza al contrato interno plano `{texto, emocion, cambio_enfado, quiere_irse}` y vuelve a validar antes de aplicar una respuesta.

Emociones admitidas: `neutral`, `enfadado`, `humillado`, `triste`, `divertido`. El validador interno rechaza campos extra, texto vacío o de más de 350 caracteres, emociones desconocidas, cambios fuera de [-0.1, 0.3] y respuestas incompletas. Las entradas del jugador se limitan a 500 caracteres. Un cliente no puede atender dos peticiones simultáneas. Cerrar, reiniciar o cambiar de cliente invalida las callbacks anteriores y cancela sus HTTP requests.

Un receptor HTTP de UE 5.8 recibe NDJSON en un buffer compartido con bloqueo, limitado a 128 KB. El game thread consume líneas completas UTF-8 y acumula `message.content`; un lector de cadenas JSON muestra progresivamente `texto`, incluyendo escapes completos. La vista previa no cambia enfado, salida ni voz. Solo la respuesta completa, terminada y validada se aplica al NPC y se envía a TTS. Cerrar descarta la vista previa y cancela el turno sin acceder a objetos Unreal desde el hilo HTTP.

`Jugar.bat` inicia una precarga del modelo en segundo plano. Se mantiene cargado 20 minutos; los prompts y hechos se serializan de forma compacta, se expresa el ánimo como estado cualitativo y se retiró el reloj de paciencia que invalidaba el prefijo en cada turno. El límite de generación baja de 256 a 128 tokens. La interfaz muestra instrucciones de juego, sin nombres del modelo, servicios, hardware o emociones internas. Los detalles permanecen en esta documentación y los logs de desarrollo.

El modelo propone una reacción verbal y un cambio acotado de enfado. Unreal sigue siendo autoridad sobre físicas, recetas, puntos y salidas. Una respuesta etiquetada `neutral` o `divertido` no puede incrementar el enfado aunque el modelo proponga un número positivo. Una petición de marcharse solo se acepta si el enfado acumulado supera el umbral del juego; se confirma al llegar a la puerta. El sistema existente de la jefa conserva su enfriamiento y no recibe una orden directa del LLM.

La API y las respuestas estructuradas siguen la [documentación de Ollama](https://docs.ollama.com/api/chat) y su [contrato de Structured Outputs](https://docs.ollama.com/capabilities/structured-outputs).

## Voz

Unreal captura float PCM mediante `AudioCaptureCore`, convierte a mono PCM16 y construye un WAV en memoria. `POST http://127.0.0.1:11435/transcribe` envía ese WAV al servicio Python propio; Whisper tiny int8 devuelve `{"text":"..."}`. No guarda las grabaciones. Se puede editar el reconocimiento.

La respuesta validada se envía a `/speak` como `{"text":"..."}`. Piper devuelve WAV PCM16 mono y Unreal lo reproduce desde un `USoundWaveProcedural` mediante audio espacial que sigue la cabeza del cliente, con ajustes de tono por arquetipo. El PCM se reduce a una envolvente de energía de 30 ventanas por segundo que anima la boca durante la reproducción; no es reconocimiento de fonemas. El panel puede silenciar esta voz. Se mantiene separada de las grabaciones de la jefa.

Fuentes: [faster-whisper](https://github.com/SYSTRAN/faster-whisper), [Piper](https://github.com/OHF-Voice/piper1-gpl), [voz Davefx y ficha del modelo](https://huggingface.co/rhasspy/piper-voices/tree/main/es/es_ES/davefx/medium). Las licencias originales se conservan en las distribuciones descargadas y la ficha de voz en `Tools/LocalAI/Models/PIPER_MODEL_CARD.txt`.

## Recursos

En `-CeroDialogueTest`, el procesador Slate bloquea teclado y botones físicos externos, y el campo consume los caracteres externos. El escenario usa el mismo campo y método de envío del juego; solo permite su Escape inyectado para comprobar cierre y foco. Esta protección evita turnos añadidos accidentalmente durante la automatización y no se activa en una partida normal.

Qwen3 4B Instruct Q4_K_M: contexto 2048, seis hilos, temperatura 0.1, un runner, `num_gpu=0`. Las pruebas exploratorias descartaron configuraciones menores que confundían el papel de cliente y camarero o se atribuían las posesiones del jugador. La [API de conversación](https://docs.ollama.com/api/chat) soporta streaming y respuestas estructuradas. Whisper tiny usa cuatro hilos CPU e int8; Piper, ONNX Runtime CPU. La GPU queda para Unreal. La captura es nativa y la inferencia Whisper vive en el proceso local externo.

La memoria es temporal, no se guarda entre partidas. El historial conserva hasta ocho intercambios y 1800 caracteres de palabras por cliente, máximo 24 identidades. Se conservan hasta 64 incidentes globales y se envían los últimos cuatro del cliente elegido; cantidad y tipo de líquido recibido también se conservan en su estado físico. El límite es de caracteres, no un contador exacto de tokens: el modelo mantiene su contexto de 2048 tokens. Las llamadas tienen timeout total y de actividad de 90 segundos para LLM y 30 para voz, incluyendo posibles cargas iniciales. El campo de texto usa un borrador compartido por edición y transcripción, con refresco explícito de Slate aunque conserve el foco. Un JSON inválido tiene un único reintento, diferido al siguiente tick para no destruir una callback mientras se ejecuta. No se fabrican respuestas generadas si el servicio falla; los atajos habituales de servicio son frases autoradas independientes. Las herramientas se instalan dentro de este proyecto; `Detener_IA.bat` detiene únicamente sus procesos propios: servicios de voz, precarga, servidor de conversación y sus runners `llama-server.exe`, identificados por la ruta de este proyecto. El lanzamiento redirige entrada, salida y errores de los servicios para que la consola no espere indefinidamente a esos procesos.

## Interrupciones físicas y reacción inmediata

Un `customer_hit` del cliente seleccionado detiene la voz y cancela la inferencia pendiente si existe un stream. Esto impide aplicar una respuesta basada en hechos previos a una agresión nueva. La captura/transcripción no se cancela por un evento físico leve; salida o cierre sí cancelan todos los estados. La siguiente frase incluye los hechos actualizados.

Veinticuatro exclamaciones grabadas previamente reaccionan al derrame, impacto, caída o desastre presenciado. Son frases autoradas, no respuestas generadas ni sustitutos ante fallos de HTTP. Comparten límite de dos voces simultáneas y pausas por cliente. La jefa sigue siendo independiente con enfriamiento y confirmación de salida.

## Respuestas autoradas de servicio y caché de voz (0.7)

Frases exactas habituales (`qué has pedido`, `cuál es tu pedido`, `gracias`, `perdón`, `lo siento`, `dónde estamos`, con variantes sin tilde y signos de interrogación) construyen un JSON autorado con el pedido y estado real. Se valida antes de aplicarlo y queda en la misma memoria acotada de conversación. `ServiceReplies` y `CE_SERVICE_REPLY` los distinguen de `ValidReplies` y `CE_DIALOGUE_REPLY`; no se cuentan como inferencias reales ni reemplazan un fallo de HTTP.

El prompt pide hasta 80 caracteres, conserva el ejemplo neutral de identidad y no repite un ejemplo literal de JSON en las instrucciones. El schema mantiene el límite estricto de 100 caracteres. `motivo_de_la_visita` añade el papel del crítico o de los clientes de una campaña vigente. Se comparan las latencias con los mismos ocho turnos de 0.6 y el mismo modelo, conservando las respuestas y tiempos completos.

Whisper y Piper usan bloqueos independientes y un servidor HTTP con hilos para que una transcripción no bloquee el servidor de salud o una respuesta de voz ya disponible. La caché LRU de WAV por texto está limitada a 64 frases y 16 MiB en RAM; no contiene grabaciones del jugador ni se escribe a disco. `X-Voice-Cache: hit/miss` permite comprobar la reutilización.

La síntesis lleva además un contador `SpeechGeneration`: un nuevo golpe invalida el WAV pendiente aunque la inferencia haya terminado. La callback comprueba ese contador antes de reproducir. La siguiente respuesta puede reutilizar el mismo WAV en caché, pero la anterior no se reproduce tarde. Esto no cancela la transcripción del micrófono. La prueba de conversación incluye este caso y distingue voz autorada reproducida, WAV descartado y acierto real de caché.

## Evidencia de cierre 0.7

El [escenario final empaquetado](../Evidence/Entrega07Dialogo/dialogue_result.json) pasa 40 comprobaciones, incluidas ocho respuestas generadas reales, reproducción Piper, transcripción Whisper, apertura del dispositivo, cancelación de stream y descarte de WAV tardío. Los contadores separan dos respuestas autoradas y una voz autorada reproducida. [Latencias frente a 0.6](../Evidence/entrega07_dialogue_latency.json): mediana comparable 17,19 s frente a 19,18 s; siguen existiendo variación y errores semánticos. El [informe jugable](POLISH_07.md) documenta una disculpa incongruente observada y las comprobaciones humanas pendientes.

## Historias y perfiles 0.8

La personalidad se obtiene de `GuestPersonality`, con nueve perfiles. El JSON de contexto incorpora `historia_cotidiana` con situación, apoyo recibido, resolución y desenlace. `StoryConversation` registra apoyo o un insulto reconocible; `TickStory` exige el desayuno servido para resolver con éxito. Los puntos y desenlaces los decide Unreal.

Se reconocen variantes exactas de pedido, agradecimiento y disculpa, preguntas sobre la historia y frases breves de ánimo. Se contabilizan como respuestas autoradas, sin confundirlas con inferencias. Un texto compuesto o una conversación libre sigue por el modelo.

Tras recibir una agresión, se refuerza el papel de víctima. Atribuciones explícitas como «no fue mi intención» o «te he derramado» se rechazan y permiten un único reintento. No es un detector semántico completo. Una caída, resbalón o destrucción de portátil también invalida inferencia y voz pendientes del cliente afectado, preservando la transcripción.

## Evidencia de cierre 0.8

El [escenario empaquetado definitivo](../Evidence/Entrega08DeliveryDialogo/dialogue_result.json) pasa 40/40 comprobaciones, con ocho respuestas generadas reales y contadores separados para respuestas autoradas, Piper y caché. La captura revisada del incidente responde «Me echaste café en la camisa. Discúlpate.»; no se atribuye la agresión al cliente en ese caso. Esto no garantiza corrección semántica para todas las conversaciones.

[Latencias](../Evidence/entrega08_dialogue_latency.json): texto visible en 5,05–12,16 s y respuesta validada en 14,10–26,06 s en turnos posteriores. Mediana comparable 18,34 s frente a 17,19 s en 0.7; la generación libre no mejoró en esta medición. Los atajos de servicio e historia evitan la inferencia y recuperan el control de inmediato.

El [informe de revisión](POLISH_08.md) contiene los resultados del turno completo, recursos, capturas y límites. La [prueba directa de las cinco historias](../Evidence/Entrega08Stories/story_dialogue_result.json) comprueba el backend por separado; no se presenta como una partida narrativa end-to-end. El micrófono abre y detiene su dispositivo, y Whisper transcribe el WAV confirmado; sigue pendiente la conversación humana por micrófono.
