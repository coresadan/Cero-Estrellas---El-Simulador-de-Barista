# Cero Estrellas: El Simulador de Barista

Prototipo **0.3.0**, Unreal Engine **5.8.3**, Windows. Eres un barista agotado que intenta conseguir su despido. La jefa convierte los grandes desastres en éxitos comerciales.

## Jugar

Abre **Jugar.bat** para ejecutar el paquete en `Release/Windows/CeroEstrellas.exe`. **Abrir_Editor.bat** abre el proyecto con `F:\Games\UE_5.8`. La cafetería se construye al pulsar Play; el mapa del editor contiene el punto de inicio.

| Control | Acción |
|---|---|
| WASD / ratón | Mover / mirar |
| E | Agarrar o soltar el objeto señalado |
| F | Usar máquina o existencias; tomar pedido; entregar desayuno |
| Q | Provocar al cliente señalado |
| Clic izquierdo | Lanzar el objeto agarrado; con manos vacías, empujar objetos o clientes cercanos |
| Mantener clic derecho | Verter taza o jarra; apuntar a otro recipiente ayuda a dirigir el chorro |
| Shift | Correr y desestabilizar la bandeja |
| Espacio | Saltar |
| R / Escape | Reiniciar turno / salir |

## Preparar un desayuno

1. Los clientes entran por la puerta, caminan al mostrador y esperan. Acércate y pulsa **F** mirándolos para tomar nota. El pedido aparece en el panel derecho; después se dirigen a su mesa.
2. Mira el **molinillo** y pulsa F con las manos vacías para coger un portafiltro limpio. Pulsa F otra vez para moler una dosis; espera a que termine.
3. Lleva la dosis al **prensador**, delante del molinillo, y pulsa F. Después lleva el portafiltro a la **máquina espresso** y pulsa F para acoplarlo. La máquina rechaza una dosis sin prensar.
4. Coge una taza de las existencias con E, o pide una nueva con F en su soporte. Las tazas nuevas están vacías.
5. Mira la máquina con la taza en la mano y pulsa F para colocarla bajo el surtidor. Pulsa F otra vez para extraer **60 ml de espresso**. Espera a que termine y recoge la taza con E.
6. Para un **americano**, coloca la taza en el surtidor de agua con F y pulsa F otra vez para añadir 100 ml. Para un **latte**, deja el espresso en una superficie y usa la **lanza de vapor** con las manos vacías para coger una jarra con 120 ml de leche fría. F coloca la jarra; F otra vez calienta su leche hasta unos 65 °C. Recógela con E, apunta a la taza y mantén clic derecho para verter. Suelta el botón al alcanzar unos 100 ml de leche. La capacidad de la taza es 180 ml: lo que sobra se derrama. Retirar la jarra de la lanza interrumpe su calentamiento; retirar la taza durante la extracción deja que el café caiga al suelo.
7. Lleva el desayuno a la mesa y pulsa F mirando al cliente para entregarlo. Si pidió cruasán, también debes entregarlo. Una bandeja permite llevar y servir los dos objetos; estos siguen siendo cuerpos físicos independientes.
8. Con las manos vacías, F en la máquina permite retirar el portafiltro usado para volver a moler. El molinillo tiene doce dosis; F repone los granos cuando se agotan.

| Bebida | Receta |
|---|---|
| Espresso | 60 ml de café |
| Americano | 60 ml de café + 100 ml de agua |
| Latte | 60 ml de café + 100 ml de leche |

Los pedidos deben conservar suficiente cantidad y llegar calientes. Los clientes rechazan recetas incorrectas o café frío. El indicador de paciencia baja mientras esperan. Comer y marcharse satisfecho libera una mesa para otro cliente. Puedes volver a agarrar el desayuno ya servido para sabotearlo.

## El caos forma parte del turno

Hay tazas y bandejas físicas en los soportes, y reposición con F para tazas, bandejas y cruasanes. El cubo de residuos permite eliminar lo que lleves en la mano. Se limita la cantidad de objetos fuera de los soportes a 48 para evitar crecimiento indefinido.

Las tazas son huecas y contienen hasta 180 ml; las jarras, hasta 350 ml. Al inclinar un recipiente, su volumen baja progresivamente. Los chorros usan segmentos más finos que siguen la velocidad: pueden llenar otra taza, manchar al cliente, ahogar su portátil o formar charcos con contornos irregulares. El café y la leche se mezclan y tienen temperatura. Correr sobre charcos grandes produce resbalones y desestabiliza el agarre; los clientes también pueden caer. Las tazas arrojadas pueden romperse y dejar fragmentos físicos. Los bordes de las bandejas tienen colisión.

Los clientes entran por una puerta animada, siguen recorridos con detección de obstáculos y se sientan. Tienen siluetas de torso y cabeza propias, rodillas articuladas, seguimiento de cabeza, parpadeo y gestos. Las sillas son cuerpos físicos: agarrarlas o lanzarlas puede dejar al cliente sin apoyo. Las caídas desplazan objetos cercanos y los testigos con línea de visión comentan el desastre. Puedes provocarles, empujarles, entregarles un pedido incorrecto, lanzarles comida o empaparles. Los golpes fuertes y la humillación pueden hacerlos huir. Los portátiles se pueden desplazar y lanzar. Los diálogos de clientes son textos locales, todavía sin voz generada.

El panel izquierdo propone cuatro objetivos opcionales: servir dos desayunos, derramar 150 ml, provocar una caída y encadenar cuatro incidentes. Cada objetivo da un bonus. Los incidentes diferentes separados por menos de nueve segundos forman un **combo**, multiplican los puntos virales y elevan el nivel corporativo; entran clientes más deprisa. Las gotas de un mismo derrame no cuentan como una cadena infinita. El despido sigue en 0 %: la empresa premia tu sabotaje.

La jefa **no habla al entrar, lanzar un objeto, servir, cometer un error o derramar un poco de café**. Interviene ante una huida por un gran desastre, un portátil destruido o un incidente que afecte a varios clientes. Hay al menos **50 segundos entre llamadas**, sin cola de comentarios pendientes. Las dieciséis frases elegibles se barajan sin repetición hasta agotar la ronda; las dieciocho grabaciones originales incluyen dos que ya no se usan para las llamadas.

## Compilar y regenerar

Requiere UE 5.8.3 y herramientas C++ de Visual Studio. Desde PowerShell en la raíz:

```powershell
& 'F:\Games\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe' .\Scripts\generate_loop_assets.py
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\generate_boss_audio.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\build.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\build_content.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\smoke_test.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\package.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\smoke_test.ps1 -Packaged
```

`-EngineRoot` permite otra instalación. `package.ps1 -ReuseCook` solo es válido para cambios de código que no cambien contenido cocinado ni configuración. Los WAV de la jefa se generan previamente con Microsoft Helena Desktop; durante la partida se reproducen grabaciones. Las transcripciones salen de `CEBossLines.h`, para mantener voz y subtítulos sincronizados. Los once efectos son sonidos originales generados por el proyecto. Máquinas, pasos e impactos usan atenuación espacial; la radio de la jefa se oye por separado.

## Arquitectura y límites

| Sistema | Responsabilidad |
|---|---|
| `ACEPlayer` / `ACEProp` | Agarre Chaos, lanzamiento, empujón, salto; recipientes, sillas y bordes de bandeja físicos |
| `ACEStation` | Molienda, prensado, portafiltro, extracción, calentamiento de jarra, agua y reposición |
| `ACELiquidSystem` | 192 segmentos y 32 charcos de cinco lóbulos; mezcla, transferencia, impactos y resbalones |
| `ACECustomer` / `ACECafeDoor` | Entrada, pedido, asiento, paciencia, receta, caída, testigos y salida |
| `UCECafeSubsystem` | Objetivos opcionales, combos, puntos virales y llamadas poco frecuentes |
| `ACEGameMode` / `ACEHUD` | Cafetería, llegadas continuas, pedidos visibles y validación opcional |

DX11, reflejos en espacio de pantalla y vetas procedurales en los mostradores y mesas, sin Lumen, ray tracing ni simulación volumétrica de fluidos. Los líquidos usan un sistema acotado de gotas y superficies, con mezcla y cantidad; no simulan presión, olas ni una malla de fluido continua. El agarre utiliza el [Physics Handle de Unreal](https://dev.epicgames.com/documentation/unreal-engine/physics-components-in-unreal-engine?application_version=5.8).

La máquina ofrece los pasos de una preparación simplificada; no simula la química de extracción ni las burbujas de espuma. Las figuras humanas tienen geometría original y articulación procedural, pero siguen siendo provisionales. Las caídas son poses animadas que empujan cuerpos Chaos cercanos; no son ragdolls esqueléticos ni personajes fotorrealistas. Cada visita usa rutas definidas y barridos de colisión, sin navegación general. Micrófono, Whisper, Ollama y Piper/Silero siguen pendientes y se integrarán por hitos separados.

El script `smoke_test.ps1` activa `-CeroSmokeTest` y guarda informe y capturas en `Evidence/RealismoPlayable`. `-EvidenceName RealismoEditor` permite separar la prueba del editor. Solo durante esa prueba se ignoran las pulsaciones físicas externas. Hay preparaciones de escena directas para probar recetas y transferencia de líquido, y controles simulados para prensado, servicio, lanzamiento, volcado, agarre de silla y salto. Esto no sustituye una partida humana para valorar diversión y ergonomía.

Consultar [VALIDATION_REALISMO.md](Docs/VALIDATION_REALISMO.md) para esta entrega. [VALIDATION_LOOP.md](Docs/VALIDATION_LOOP.md) conserva las comprobaciones de 0.2.0 y [VALIDATION.md](Docs/VALIDATION.md) las del primer prototipo.
