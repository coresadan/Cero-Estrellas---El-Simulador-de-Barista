# Validación del bucle de cafetería 0.2.0

11 de octubre de 2026. Unreal Engine **5.8.3**, Win64 Development, DX11, GTX 1060 de 6 GB, 24 GB de RAM.

## Estado de la entrega

La primera ejecución con el motor pasó **29 comprobaciones**: [informe del editor](../Evidence/LoopEditor/smoke_result.json). Esa ejecución verificó el nuevo bucle antes de los ajustes finales de capturas, articulación y fragmentos de cerámica.

El **paquete final de Windows pasa las 34 comprobaciones** de la secuencia ampliada: [smoke_result.json](../Evidence/LoopPlayable/smoke_result.json). El ejecutable entregado es `Release/Windows/CeroEstrellas.exe`; se abre con `Jugar.bat`. La compilación final del editor y el cook/empaquetado completo también terminan correctamente.

La revisión del registro y las capturas detectó un material de líquido sin shader válido y un intervalo de detección de resbalones demasiado largo. Se corrigieron la conexión del interpolador de vértices y la bandera de materiales para instancias, se rehízo el cook y se redujo el intervalo de detección a 50 ms. En la última ejecución no hay errores de materiales ni materiales de sustitución; el resbalón al correr pasa.

## Comprobaciones funcionales

La prueba usa F simulado para coger un portafiltro, moler, acoplarlo, colocar la taza, extraer espresso, tomar un pedido y servir. Usa clic derecho simulado para volcar y clic izquierdo para lanzar. Las preparaciones de escena directas permiten comprobar recetas, existencias, mezcla de leche, transferencia balística y capacidad sin depender de la selección aleatoria de clientes.

| Área | Comprobación |
|---|---|
| Preparación | Molienda consume una dosis; portafiltro se acopla; espresso sale caliente y llena la taza |
| Recetas | Latte mezcla espresso y leche; americano mezcla espresso y agua; capacidad máxima de 180 ml |
| Desayunos | Cliente entra caminando, se toma el pedido, rechaza receta incorrecta, acepta la correcta y exige el cruasán cuando corresponde |
| Existencias | Tazas nuevas vacías y bandejas nuevas disponibles |
| Líquidos | Volcado continuo; trayectoria con colisión llena otra taza; cantidad transferida y cantidad derramada registradas |
| Desastres | Café alcanza al cliente; charcos provocan resbalón al correr; una taza arrojada produce fragmentos físicos; un cruasán impacta y provoca huida |
| Jefa | Silencio al empezar y ante incidentes pequeños; elogio tras huida confirmada; rechazo de llamadas durante el intervalo; silencio tras terminar el intervalo hasta otro desastre; nueva frase diferente |
| Audio | Dieciocho grabaciones con duración válida; sonidos de máquinas y acciones importados |

## Evidencia

- [Compilación del editor](../Evidence/LoopPlayable/editor_build.log).
- [Generación de contenido](../Evidence/LoopPlayable/content_build.log): cero errores; una advertencia de prioridad de la configuración del pool de texturas.
- [Cook y empaquetado completo](../Evidence/LoopPlayable/full_package_build.log).
- [Compilación final del paquete](../Evidence/LoopPlayable/package_build.log).
- [Registro del ejecutable](../Evidence/LoopPlayable/packaged_runtime.log).
- Capturas producidas por el ejecutable: máquina, pedidos, líquido en movimiento, charco, reacción y llamada.

Se han inspeccionado las capturas finales de [extracción](../Evidence/LoopPlayable/02_maquina_completa.png), [pedidos](../Evidence/LoopPlayable/03_pedido_y_clientes.png), [volcado](../Evidence/LoopPlayable/04_liquido_y_charcos.png), [charco](../Evidence/LoopPlayable/04b_charco_persistente.png), [reacción](../Evidence/LoopPlayable/05_reaccion_y_gestos.png) y [llamada de la jefa](../Evidence/LoopPlayable/06_desastre_y_jefa.png). Las mesas llevan números y el cliente muestra el suyo al esperar el desayuno.

La primera llamada final fue la frase 3 y la siguiente la 16, después de esperar realmente el intervalo de 50 segundos. Las llamadas durante el intervalo no se reprodujeron ni reaparecieron al terminarlo. La prueba del ejecutable también rechaza errores de compilación de materiales en el registro.

## Límites

- `human_playthrough: false`: las comprobaciones automatizadas y capturas no valoran la diversión ni sustituyen una prueba humana del turno.
- Los líquidos conservan cantidades de los recipientes y responden a colisiones mediante gotas y charcos acotados. No es una simulación volumétrica de fluidos. El pool puede rechazar nuevas emisiones cuando está saturado; la máquina está dimensionada para el máximo de clientes de esta escena.
- El modelo de extracción es jugable y simplificado. La leche espumada se prepara en su propia estación; no se simula la física interna de una lanza de vapor.
- Los personajes siguen siendo modelos provisionales con articulación procedural. Hay gestos y recorridos, sin captura de movimiento, navegación general, animación facial avanzada ni ragdolls esqueléticos.
- Las voces de la jefa son grabaciones sintetizadas previamente. No hay STT, LLM ni TTS durante la partida.
- No se ha medido el presupuesto de VRAM con Whisper funcionando junto al renderizado.
