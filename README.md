# Cero Estrellas: El Simulador de Barista

Prototipo **0.8.0**, Unreal Engine **5.8.3**, Windows. Eres un barista agotado que intenta conseguir su despido. La jefa convierte los grandes desastres en éxitos comerciales.

Las comandas ahora son hojas tumbadas sobre la barra: dejan ver a los clientes. Las caídas usan límites anatómicos guardados, colisiones entre segmentos no adyacentes y masas por parte; un contacto fuerte puede hacer caer a otro cliente. La marcha tiene claves articulares y ajustes de contacto para pies, manos y taza. Piel, ropa, cintura e iluminación se han retocado, y la madera tiene relieve y brillo más suaves.

Llegan nueve perfiles, desde el punk de oficina hasta la abuela rave o el mago sin contrato. Cada visitante recibe una pequeña historia: entrevista, cita, crítica encubierta, audición o poema para un ex. Puedes ayudar preparando el desayuno y dando apoyo, o arruinar la situación con tu sabotaje. Las conversaciones conocen el pedido, la personalidad y el desenlace. Preguntas habituales de servicio o historia tienen respuestas autoradas inmediatas; la conversación libre conserva generación y voz.

El turno dura ocho minutos y termina con un ascenso involuntario. La jefa conserva llamadas espaciadas, sin cola. Diseño, pruebas y límites de esta revisión: [POLISH_08.md](Docs/POLISH_08.md). Los informes [0.7](Docs/POLISH_07.md) y [0.6](Docs/POLISH_06.md) son históricos. Entrega 0.8 cerrada: **116/116 comprobaciones jugables** durante el turno completo y **40/40 de conversación**, sobre el ejecutable definitivo. [Manifiesto y binario verificado](Evidence/entrega08_delivery.json). Falta una partida humana para valorar ritmo y diversión.

## Jugar

Abre **Jugar.bat** para ejecutar el paquete en `Release/Windows/CeroEstrellas.exe`. **Abrir_Editor.bat** abre el proyecto con `F:\Games\UE_5.8`. La cafetería se construye al pulsar Play; el mapa del editor contiene el punto de inicio.

| Control | Acción |
|---|---|
| WASD / ratón | Mover / mirar |
| E | Agarrar o soltar el objeto señalado |
| F | Usar, tomar pedido o servir; mantener para prensar; pulsar durante extracción para detenerla |
| Q | Provocar al cliente señalado |
| T | Abrir Dialogar mirando a un cliente; Escape cierra el panel |
| V | Grabar / detener voz durante una conversación; revisa la transcripción antes de enviar |
| Clic izquierdo | Lanzar el objeto agarrado; con manos vacías, empujar objetos o clientes cercanos |
| Mantener clic derecho | Verter taza o jarra; apuntar a otro recipiente ayuda a dirigir el chorro |
| Shift | Correr y desestabilizar la bandeja |
| Espacio | Saltar |
| Mantener Tab | Consultar todas las comandas |
| F1 | Mostrar u ocultar la ayuda completa |
| R / Escape | Reiniciar turno / salir |

## Dialogar con los clientes

Mira a un cliente y pulsa **T** para abrir la conversación. Escribe una frase y pulsa **Enter** o **Enviar**: recuperas el movimiento y puedes preparar pedidos mientras responde en subtítulos y voz. **T** vuelve a abrir el editor conservando el borrador; **Seguir jugando** oculta el panel sin cancelar la conversación. **Escape** termina la conversación y cancela la respuesta pendiente.

Para hablar, pulsa **Grabar voz** o **V**, habla y pulsa **Detener / transcribir** o V otra vez; revisa el texto reconocido y pulsa **Enviar**. La grabación se limita a quince segundos y solo comienza por esa acción explícita. Si grabas mientras te mueves, el editor se abre al recibir la transcripción para que la revises. **Escuchar voz** activa o silencia al cliente. Las palabras aparecen progresivamente; la voz y la emoción solo se aplican tras validar la respuesta completa. Un golpe nuevo interrumpe una respuesta pendiente a hechos anteriores; el cliente reacciona inmediatamente y puedes volver a hablarle.

Preguntas exactas como **qué has pedido**, **gracias**, **perdón**, **lo siento** o **dónde estamos** usan frases autoradas contextualizadas, sin esperar a una inferencia. La voz reutiliza las frases sintetizadas recientemente.

Los clientes saben que están desayunando en Cero Estrellas y que tú eres su camarero. Puedes hablar del pedido o de otros temas; recuerdan los intercambios recientes aunque cierres el panel. Una disculpa puede bajar el enfado y un insulto subirlo. Hablar normalmente no activa llamadas de la jefa. El cliente espera durante una conversación breve, mientras la cafetería continúa funcionando. La interfaz utiliza mensajes de juego y no muestra nombres del backend ni códigos de emoción.

## Configuración del desarrollo

En otro equipo, ejecuta una vez **Preparar_IA.bat** (requiere Internet para descargar herramientas y modelos) y después **Jugar.bat**. Los archivos quedan en `Tools/LocalAI`, sin instalar servicios de Windows. Configuración: `[CeroEstrellas.AI] Model` en `Config/DefaultGame.ini`; la preparación descarga ese modelo. Predeterminado: **Qwen3 4B Instruct Q4_K_M**, contexto 2048, seis hilos, `num_gpu=0`, 128 tokens por respuesta y retención de 20 minutos. La precarga comienza en segundo plano al abrir el juego. Whisper tiny usa CPU int8; Piper usa Davefx. Los tres arquetipos utilizan por ahora la misma voz. **Detener_IA.bat** libera los procesos propios.

Todo el diálogo se procesa en `127.0.0.1`: Ollama en **11434**, voz en **11435**. Una vez descargados los modelos no necesita Internet. Si la IA está apagada, falta un modelo o falla el micrófono, el panel muestra el error y permite volver a intentar o escribir. Un JSON inválido tiene un único reintento automático. No sustituye respuestas fallidas por frases que aparenten ser IA. La latencia y precisión dependen del hardware; puedes corregir la transcripción antes de enviarla.

La preparación de IA utiliza Python incluido en Unreal 5.8, con ruta predeterminada `F:\Games\UE_5.8`; si tu instalación está en otra ruta, ejecuta `Scripts/setup_ai.ps1 -EngineRoot "ruta_del_motor"`. El juego empaquetado y las herramientas de IA se mantienen en carpetas separadas: copiar solo `Release/Windows` no incluye los modelos ni el servicio de voz.

## Modelados propios

La revisión incorpora **24 mallas originales**: cerámica hueca y asa curva, jarra con pico, cruasán laminado, portafiltro y empuñadura, bordes de bandeja, piezas de ropa, cabeza con mandíbula y nariz, cabello, manos con dedos, zapatos, gafas, lanza de vapor, botellas, tolvas, macetas y hojas. Máquinas, mobiliario y estructura usan bordes biselados y accesorios; cerámica, piel, ropa y bollería tienen materiales diferenciados. Se conservan las dimensiones de interacción para mantener jugables las recetas y el transporte.

Las fuentes editables están en `SourceMeshes/CeroEstrellas_Modelos.blend` y `SourceMeshes/CeroEstrellas_Personajes.blend`, con exportaciones OBJ y FBX. `Scripts/model_cafe.py` reproduce los objetos y `Scripts/author_characters.py` los cuerpos con dieciocho huesos y pesos mezclados en codos y rodillas. Los personajes son estilizados, con claves articulares, poses y ajustes de contacto del esqueleto; no usan captura de movimiento.

La revisión 0.5 integra nariz y mejillas en una superficie continua, reduce ojos y orejas, conecta el cuello con el torso y añade detalles de cuello de camisa. Las UV proyectadas y las normales corregidas permiten aplicar **19 texturas propias de 512 px y once materiales**: vetas, piedra moteada, tela tejida, pintura, piel y metal cepillado, con normal y rugosidad. El local combina sol entrando por las ventanas y sus sombras, luz cálida, relleno frío, reflexión ambiental y gradación moderada. Hay molduras, zócalos, frentes de madera estriada, sillas tapizadas con remates de latón, cuadros y reloj. Las ventanas muestran siluetas de edificios; los portátiles una interfaz financiera ficticia y la cafetera su estado en un pequeño panel frontal. Los avisos de interacción se sitúan abajo para despejar caras y objetos; los pedidos se consultan en el HUD. Los clientes entran por un pasillo separado del punto donde esperan el pedido y se apartan al cruzarse; las llegadas esperan a que termine una salida para evitar bloqueos en la puerta.

## Preparar un desayuno

1. Los clientes entran por la puerta, caminan al mostrador y esperan. Acércate y pulsa **F** mirándolos para tomar nota. El pedido aparece en las comandas de la barra y al mirar al cliente (mantén Tab para consultar todos); después se dirigen a su mesa.
2. Mira el **molinillo** y pulsa F con las manos vacías para coger un portafiltro limpio. Pulsa F otra vez para moler una dosis; espera a que termine.
3. Lleva la dosis al **prensador**, delante del molinillo, y **mantén F** hasta completar la presión. Soltar antes deja la dosis incompleta; vuelve a agarrar el filtro para intentarlo de nuevo. Después lleva el portafiltro a la **máquina espresso** y pulsa F para acoplarlo. La máquina rechaza una dosis sin prensar.
4. Coge una taza de las existencias con E, o pide una nueva con F en su soporte. Las tazas nuevas están vacías.
5. Mira la máquina con la taza en la mano y pulsa F para colocarla bajo el surtidor. Pulsa F otra vez para extraer **60 ml de espresso**. F durante la extracción la detiene antes: puedes preparar un café corto deliberadamente. Espera a que termine y recoge la taza con E.
6. Para un **americano**, coloca la taza en el surtidor de agua con F y pulsa F otra vez para añadir 100 ml. Para un **latte**, deja el espresso en una superficie y usa la **lanza de vapor** con las manos vacías para coger una jarra con 120 ml de leche fría. F coloca la jarra; F otra vez calienta su leche hasta unos 65 °C. Recógela con E, apunta a la taza y mantén clic derecho para verter. Suelta el botón al alcanzar unos 100 ml de leche. La capacidad de la taza es 180 ml: lo que sobra se derrama. Retirar la jarra de la lanza interrumpe su calentamiento; retirar la taza durante la extracción deja que el café caiga al suelo.
7. Lleva el desayuno a la mesa y pulsa F mirando al cliente para entregarlo. Si pidió cruasán, también debes entregarlo. Una bandeja permite llevar y servir los dos objetos; estos siguen siendo cuerpos físicos independientes.
8. Con las manos vacías, F en la máquina permite retirar el portafiltro usado para volver a moler. El molinillo tiene doce dosis; F repone los granos cuando se agotan.

| Bebida | Receta |
|---|---|
| Espresso | 60 ml de café |
| Americano | 60 ml de café + 100 ml de agua |
| Latte | 60 ml de café + 100 ml de leche |

Si la comanda pide extras, lleva la taza a la estación de **azúcar** (F añade 2 g) o **sirope** (F añade 6 ml). Para más espuma, usa el **espumador** con una jarra que contenga al menos 30 ml de leche a 50 °C; vierte después esa leche sobre el espresso. Los extras se transfieren con el líquido y se comprueban al servir.

Los pedidos deben conservar suficiente cantidad y llegar calientes. Los clientes rechazan recetas incorrectas o café frío. El indicador de paciencia baja mientras esperan. Comer y marcharse satisfecho libera una mesa para otro cliente. Puedes volver a agarrar el desayuno ya servido para sabotearlo.

## El caos forma parte del turno

Hay tazas y bandejas físicas en los soportes, y reposición con F para tazas, bandejas y cruasanes. El cubo de residuos permite eliminar lo que lleves en la mano. Se limita la cantidad de objetos fuera de los soportes a 48 para evitar crecimiento indefinido.

Las tazas son huecas y contienen hasta 180 ml; las jarras, hasta 350 ml. Al inclinar un recipiente, su volumen baja progresivamente. Los chorros usan segmentos más finos que siguen la velocidad: pueden llenar otra taza, manchar al cliente, ahogar su portátil o formar charcos con contornos irregulares. El café y la leche se mezclan y tienen temperatura. Correr sobre charcos grandes produce resbalones y desestabiliza el agarre; los clientes también pueden caer. Las tazas arrojadas pueden romperse y dejar fragmentos físicos. Los bordes de las bandejas tienen colisión.

Los clientes entran por una puerta animada, siguen recorridos con detección de obstáculos y se sientan. Tienen siluetas de torso y cabeza propias, rodillas articuladas, seguimiento de cabeza, parpadeo y gestos. Las sillas son cuerpos físicos: agarrarlas o lanzarlas puede dejar al cliente sin apoyo. Las caídas desplazan objetos cercanos y los testigos con línea de visión comentan el desastre. Puedes provocarles, empujarles, entregarles un pedido incorrecto, lanzarles comida o empaparles. Los golpes fuertes y la humillación pueden hacerlos huir. Los portátiles se pueden desplazar y lanzar. Las reacciones inmediatas usan exclamaciones grabadas y subtítulos; Dialogar añade conversación libre y voz generada.

El panel izquierdo propone cuatro objetivos opcionales: servir dos desayunos, derramar 150 ml, provocar una caída y encadenar cuatro incidentes. Cada objetivo da un bonus. Los incidentes diferentes separados por menos de nueve segundos forman un **combo**, multiplican los puntos virales y elevan el nivel corporativo; entran clientes más deprisa. Las gotas de un mismo derrame no cuentan como una cadena infinita. La empresa sigue premiando tu sabotaje y el despido nunca llega.

La jefa **no habla al entrar, lanzar un objeto, servir, cometer un error o derramar un poco de café**. Interviene ante una huida por un gran desastre, un portátil destruido o un incidente que afecte a varios clientes. Hay al menos **50 segundos entre llamadas**, sin cola de comentarios pendientes. Las campañas pueden activar tres nuevas llamadas, respetando la misma pausa. Las dieciséis frases elegibles se barajan sin repetición hasta agotar la ronda; las dieciocho grabaciones originales incluyen dos que ya no se usan para las llamadas.

## Compilar y regenerar

Requiere UE 5.8.3 y herramientas C++ de Visual Studio. Desde PowerShell en la raíz:

```powershell
& 'F:\Games\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe' .\Scripts\generate_loop_assets.py
& '.\Tools\LocalAI\Python\Scripts\python.exe' .\Scripts\generate_material_textures.py
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\generate_boss_audio.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\generate_events_audio.ps1
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --python .\Scripts\model_cafe.py
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --python .\Scripts\author_characters.py
& '.\Tools\LocalAI\Python\Scripts\python.exe' .\Scripts\generate_polish_audio.py
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\build.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\build_content.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\smoke_test.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\package.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\smoke_test.ps1 -Packaged
```

`-EngineRoot` permite otra instalación. `package.ps1 -IterativeCook` actualiza los recursos que han cambiado; `-ReuseCook` solo es válido para cambios de código que no cambien contenido cocinado ni configuración. Los WAV de la jefa se generan previamente con Microsoft Helena Desktop; durante la partida se reproducen grabaciones. Las transcripciones salen de `CEBossLines.h`, para mantener voz y subtítulos sincronizados. Los once efectos son sonidos originales generados por el proyecto. Máquinas, pasos e impactos usan atenuación espacial; la radio de la jefa se oye por separado.

## Arquitectura y límites

El suavizado de fotogramas limita la partida a 60 FPS para reservar tiempo de CPU al diálogo y contener la carga gráfica. Es un límite configurado, no una garantía de 60 FPS en todas las escenas.

| Sistema | Responsabilidad |
|---|---|
| `ACEPlayer` / `ACEProp` | Agarre Chaos, lanzamiento, empujón, salto; recipientes, sillas y bordes de bandeja físicos |
| `ACEStation` | Molienda, prensado animado, portafiltro, extracción, vapor, agua, azúcar, sirope, espuma y reposición |
| `ACELiquidSystem` | 192 segmentos y 32 charcos de cinco lóbulos; mezcla, transferencia, impactos y resbalones |
| `ACECustomer` / `ACECafeDoor` | Entrada, pedido, asiento, paciencia, receta, caída, testigos y salida |
| `CEBodyAnimation` / `CEGuestMotion` / `CECustomerRig` / `CEHands` | Poses esqueléticas, física articulada, recuperación, cara, energía de voz y agarres visibles |
| `CECustomerStories` / `CECustomerSocial` | Historias con ayuda y sabotaje, nueve personalidades, protección, limpieza y comentarios |
| `UCECafeSubsystem` | Objetivos, confianza traicionada, combos, viralidad, campañas y llamadas poco frecuentes |
| `UCEDialogueComponent` | Panel Slate, captura de micrófono, HTTP, memoria por cliente, validación JSON y reproducción Piper |
| `ACEGameMode` / `ACEHUD` | Cafetería, llegadas continuas, pedidos visibles y validación opcional |

DX11, reflejos en espacio de pantalla, mapas PBR con mipmaps y vetas propias en los mostradores y mesas, sin Lumen, ray tracing ni simulación volumétrica de fluidos. Las texturas y su generador están en `SourceTextures` y `Scripts/generate_material_textures.py`; requieren NumPy, disponible en el entorno de voz preparado por el proyecto. Los líquidos usan un sistema acotado de gotas y superficies, con mezcla y cantidad; no simulan presión, olas ni una malla de fluido continua. El agarre utiliza el [Physics Handle de Unreal](https://dev.epicgames.com/documentation/unreal-engine/physics-components-in-unreal-engine?application_version=5.8).

La máquina ofrece los pasos de una preparación simplificada; no simula la química de extracción ni las burbujas de espuma. Los clientes usan mallas esqueléticas originales con transiciones, seguimiento de cabeza, parpadeo, gestos, sorbos y grabación con móvil. La boca sigue la energía PCM de la voz generada, sin reconocimiento fonético. Las caídas simulan el esqueleto con Chaos, hasta dos clientes simultáneos; un tercero usa una reacción animada. La cápsula de pie se desactiva durante la caída física y vuelve al recuperarse. Los impactos menores pueden activar física parcial de cabeza. Cada visita usa rutas definidas y barridos de colisión, sin navegación general. La captura utiliza AudioCapture; Whisper vive en un servicio externo local. Los nueve perfiles comparten el modelo de voz con ajustes de tono, además de frases grabadas propias. Whisper, Ollama y Piper usan CPU para reservar VRAM al juego.

El script `smoke_test.ps1` activa `-CeroSmokeTest` y guarda informe y capturas en `Evidence/RealismoPlayable`. `-EvidenceName RealismoEditor` permite separar la prueba del editor. Solo durante esa prueba se ignoran las pulsaciones físicas externas. Hay preparaciones de escena directas para probar recetas y transferencia de líquido, y controles simulados para prensado, servicio, lanzamiento, volcado, agarre de silla y salto. Esto no sustituye una partida humana para valorar diversión y ergonomía.

Consultar [POLISH_08.md](Docs/POLISH_08.md) para la revisión actual, [POLISH_07.md](Docs/POLISH_07.md) para el histórico, [POLISH_06.md](Docs/POLISH_06.md) para el histórico y [DIALOGUE_ARCHITECTURE.md](Docs/DIALOGUE_ARCHITECTURE.md) para esta entrega. [VALIDATION_MODELOS_DIALOGO.md](Docs/VALIDATION_MODELOS_DIALOGO.md), [VALIDATION_REALISMO.md](Docs/VALIDATION_REALISMO.md), [VALIDATION_LOOP.md](Docs/VALIDATION_LOOP.md) y [VALIDATION.md](Docs/VALIDATION.md) conservan las comprobaciones anteriores.
