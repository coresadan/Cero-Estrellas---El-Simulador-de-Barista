# Validación del hito 1

Fecha: 11 de octubre de 2026.

Motor utilizado: **Unreal Engine 5.8.3**, instalado en `F:\Games\UE_5.8`.
Equipo de ejecución: NVIDIA GeForce GTX 1060 6 GB, 24 GB de RAM y AMD Ryzen 7 2700X. El registro del motor confirma **D3D11**.

## Compilación y contenido

- Compilación real de `CeroEstrellasEditor`, Win64 Development: completada.
- Generación del mapa, materiales e importación de ocho grabaciones del Jefe: completada con cero errores de commandlet.
- Configuración de compilación actualizada a `BuildSettingsVersion.V7`, requerida por esta instalación de UE 5.8.3.

## Prueba ejecutada con el motor

Resultado: **PASS** en [smoke_result.json](../Evidence/EditorPlayable/smoke_result.json).

La prueba utiliza eventos de entrada de Unreal, sin invocar directamente las acciones del jugador:

| Comprobación | Resultado |
|---|---|
| E agarra una taza mediante el Physics Handle | PASS |
| Clic derecho inclina y vacía una taza | PASS |
| Clic izquierdo lanza el cruasán y este colisiona con un cliente | PASS |
| E agarra la bandeja | PASS |
| D mueve al jugador por el escenario | PASS |
| Shift activa la velocidad de carrera | PASS |
| El cliente reacciona y su salida se confirma | PASS |
| El Jefe reproduce un elogio con subtítulos | PASS |
| Las ocho grabaciones están disponibles y tienen duración válida | PASS |
| Se activa la frase de ShitAdvisor tras la salida | PASS |

## Revisión visual

Se han inspeccionado capturas de la cafetería, el agarre, la bandeja, la reacción del cliente y la radio. Se aumentó el tamaño de la interfaz y se separó el rótulo principal de las etiquetas de clientes. La versión final mueve el lema corporativo a la pared del menú para evitar otro solapamiento.

La escena usa formas y materiales provisionales. No representa arte final, animación facial ni una evaluación humana de la diversión o del control de la bandeja.

## Paquete de Windows

Resultado: **PASS** en [smoke_result.json](../Evidence/FirstPlayable/smoke_result.json), ejecutando el paquete real de Windows.

- Compilación Win64 Development, cook, staging y archivo: completados. Registros: [cook y empaquetado completo](../Evidence/FirstPlayable/full_package_build.log) y [compilación final](../Evidence/FirstPlayable/package_build.log).
- Ejecutable entregado: `Release/Windows/CeroEstrellas.exe`, disponible mediante `Jugar.bat`.
- Todas las comprobaciones de la tabla anterior pasan también en el paquete. El registro confirma audio cargado y reproducción de la frase de ShitAdvisor después de la salida del cliente.
- Los materiales y ocho grabaciones se incluyen mediante `DirectoriesToAlwaysCook` en `DefaultGame.ini`; se verificaron en el contenido empaquetado y en ejecución.
- La prueba ignora la entrada física de la ventana y utiliza eventos simulados del controlador. Esto evita interferencias externas; no afecta a los controles de una partida normal.
- Capturas revisadas del paquete: [cafetería](../Evidence/FirstPlayable/01_cafeteria.png), [reacción del cliente](../Evidence/FirstPlayable/04_cliente_reaccion.png) y [elogio del Jefe](../Evidence/FirstPlayable/06_jefe_marketing.png). Registro: [packaged_runtime.log](../Evidence/FirstPlayable/packaged_runtime.log).

## Límites de esta validación

- `human_playthrough: false`: no se ha realizado una partida humana completa.
- Micrófono, Whisper, Ollama y Piper/Silero todavía no están integrados.
- Los NPC usan respuestas y salidas predefinidas; el Jefe emplea grabaciones sintetizadas previamente.
- No se han medido los picos de VRAM con Whisper ejecutándose simultáneamente, ni latencias de IA.
- Ragdolls, fractura de cerámica, Niagara/decals y daño a portátiles quedan para hitos posteriores.
