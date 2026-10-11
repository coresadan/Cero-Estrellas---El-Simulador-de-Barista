# Validación del hito 0.3.0

11 de octubre de 2026. Unreal Engine **5.8.3**, Win64 Development, DX11. Equipo del desarrollador: GTX 1060 de 6 GB y 24 GB de RAM.

## Resultado

La compilación C++ final del editor y la generación de contenido terminan correctamente. La ejecución con Unreal, previa al último ajuste visual de la cintura, pasa **50 comprobaciones**: [informe](../Evidence/RealismoEditor/smoke_result.json) y [registro](../Evidence/RealismoEditor/editor_runtime.log).

El **paquete final de Windows también pasa las 50 comprobaciones** después de ese ajuste: [informe final](../Evidence/RealismoPlayable/smoke_result.json) y [registro del ejecutable](../Evidence/RealismoPlayable/packaged_runtime.log). Se revisaron las capturas reales, incluida la continuidad entre torso y piernas. El ejecutable es `Release/Windows/CeroEstrellas.exe`; `Jugar.bat` lo abre.

Se realizó un cook completo del contenido nuevo. La actualización final, exclusivamente de C++, reutiliza ese cook y vuelve a compilar y empaquetar el ejecutable.

## Cobertura

| Área | Evidencia funcional |
|---|---|
| Máquina | Molienda, rechazo de dosis sin prensar, F para prensar, acoplar, colocar taza y extraer espresso caliente |
| Leche | Jarra física con 120 ml fríos; la lanza calienta el volumen existente; clic derecho vierte por trayectorias con colisión y produce un latte caliente |
| Servicio | Entrada por puerta animada, toma de pedido, receta correcta, rechazo de receta incorrecta, desayuno que exige cruasán y americano |
| Física | Movimiento y carrera simulados, salto, resbalón, proyección de cruasán, rotura de taza y fragmentos, transferencia entre recipientes y límite de capacidad |
| Clientes | Asiento sobre silla física, agarre y lanzamiento de esa silla, caída del cliente, reacción de testigo y recuperación posterior |
| Progresión | Objetivos opcionales, combos de incidentes diferentes y prevención de puntos duplicados por repetir un incidente en un intervalo corto |
| Jefa | Silencio inicial y ante incidentes pequeños; llamada tras salida confirmada; intervalo real de 50 segundos; descarte de llamadas pendientes; siguiente frase diferente; 18 grabaciones válidas |
| Renderizado | Pool fijo de 192 segmentos y 160 instancias de charco; cinco materiales, siluetas originales de cabeza y torso, efectos propios y revisión del registro de shaders |

El escenario de prueba configura directamente ciertos recipientes, recetas y clientes para aislar fallos. Simula pulsaciones reales en el controlador para prensado, servicio, lanzamiento, vertido, carrera, agarre de silla y salto. No es una partida humana.

## Archivos de evidencia

- [Compilación final del editor](../Evidence/RealismoPlayable/editor_build.log).
- [Generación de contenido](../Evidence/RealismoPlayable/content_build.log): cero errores, una advertencia de prioridad del pool de texturas.
- [Cook y empaquetado completo](../Evidence/RealismoPlayable/full_package_build.log).
- [Empaquetado final tras el ajuste visual](../Evidence/RealismoPlayable/package_build.log).
- Capturas del ejecutable: [máquina](../Evidence/RealismoPlayable/02_maquina_completa.png), [jarra](../Evidence/RealismoPlayable/02b_jarra_verter_a_mano.png), [cliente sentado y servicio](../Evidence/RealismoPlayable/03_pedido_y_clientes.png), [chorro continuo](../Evidence/RealismoPlayable/04_liquido_y_charcos.png), [charco](../Evidence/RealismoPlayable/04b_charco_persistente.png), [caída y testigo](../Evidence/RealismoPlayable/07_silla_caida_y_testigo.png).

La revisión de los registros de cook y de la última ejecución no encuentra errores de materiales, compilaciones de shaders fallidas ni errores fatales. La prueba espera realmente el intervalo entre llamadas: la segunda frase es diferente y los incidentes descartados durante el intervalo no reaparecen.

## Correcciones encontradas durante la validación

- Se separó la pulsación del prensador del cambio posterior de objetivo en la prueba: Unreal procesa esos controles en el siguiente frame.
- La nueva detección de colisiones reveló una pared bajo la puerta que impedía salir. La fachada tiene ahora un hueco y una pequeña entrada con suelo y límites físicos.
- El cliente se coloca sobre el asiento, en lugar de quedar delante del borde de la silla.
- Se añadió continuidad en la cintura para cerrar la separación visible entre torso y piernas al estar de pie o sentado.
- Los testigos trazan visión desde la altura de los ojos y reintentan brevemente cuando una silla u otro cuerpo tapa el incidente. El cliente situado detrás de una pared sigue sin verlo.

## Límites de esta entrega

- `human_playthrough: false`: falta valorar diversión, ergonomía, dificultad de preparar latte y equilibrio con una partida humana.
- Los personajes tienen geometría y animación procedural de prototipo. Las caídas animadas empujan cuerpos Chaos cercanos; no son ragdolls esqueléticos, personajes fotorrealistas ni navegación general.
- Los líquidos usan gotas con volumen y colisión, y charcos de contorno compuesto. No hay simulación volumétrica, olas, espuma física ni química de extracción.
- Los efectos, madera procedural y reflejos de pantalla mejoran la lectura de los objetos; no constituyen una pasada de arte final.
- Los clientes todavía responden mediante texto local. La jefa usa audios generados previamente. Micrófono, Whisper, Ollama y TTS en tiempo real siguen pendientes.
- No se ha medido rendimiento o consumo de VRAM con Whisper funcionando a la vez.

Los informes de [0.2.0](VALIDATION_LOOP.md) y del [primer prototipo](VALIDATION.md) se conservan como resultados históricos.
