# Validación de gráficos y conversación — 0.5.0

Entrega comprobada el 11 de octubre de 2026 con Unreal Engine 5.8.3, Windows, Ryzen 7 2700X, 24 GB RAM y GTX 1060 de 6 GB. Ejecutable: `Release/Windows/CeroEstrellas.exe`; inicio habitual: `Jugar.bat`.

## Acabado visual integrado

Hay 24 mallas originales, con 69 248 triángulos en sus fuentes, y 19 texturas propias de 512 × 512. Once materiales diferencian madera, piedra, tela, metal, piel, cerámica y otras superficies. Se corrigieron las UV y las normales, se integraron nariz y mejillas, se ajustaron ojos, orejas y cuello y se refinó el cabello. El estilo sigue siendo una caricatura tridimensional; no son personajes fotorrealistas.

La cafetería incorpora luz solar desde las ventanas, sombras, relleno frío, iluminación cálida y reflexión ambiental. Se añadieron molduras, madera estriada en el mostrador, tapizado, remates de latón, cuadros, reloj y edificios visibles en las ventanas. Las máquinas tienen paneles frontales; los portátiles muestran una interfaz financiera ficticia. La revisión de capturas permitió suavizar las bandas del metal y corregir la orientación de las pantallas. Los avisos se sitúan abajo para despejar las caras. El panel de conversación utiliza la misma paleta y oculta los nombres del backend y las emociones internas.

Fuentes reproducibles: [manifiesto de mallas](../SourceMeshes/model_manifest.json), [texturas](../SourceTextures/manifest.json), [generador de modelados](../Scripts/model_cafe.py), [generador de superficies](../Scripts/generate_material_textures.py). La fuente de Blender está en `SourceMeshes/CeroEstrellas_Modelos.blend`.

## Compilación y paquete comprobados

- [Modelados exportados: 24 mallas](../Evidence/entrega05_models_build.log).
- [Importación de contenido: cero errores](../Evidence/entrega05_content_build.log) y [reimportación de las texturas corregidas](../Evidence/entrega05_texture_refresh.log).
- [Compilación final del editor](../Evidence/entrega05_compact_editor_build.log): correcta, 56,55 segundos.
- [Empaquetado completo de contenido](../Evidence/entrega05_package_build.log), [actualización de texturas](../Evidence/entrega05_texture_package_build.log) y [configuración final con 60 FPS de límite](../Evidence/entrega05_final_package_build.log).
- [Paquete definitivo con historial compacto](../Evidence/entrega05_compact_final_package_build.log): correcto, 74,43 segundos. Reutiliza el contenido ya actualizado; este último cambio afecta únicamente al código de conversación.

La importación conserva una advertencia de prioridad de `r.Streaming.PoolSize`: permanece en el valor del proyecto, 1200. Los escenarios finales no detectan errores de compilación de materiales, sustituciones por falta de uso ni errores fatales de runtime.

## Pruebas del ejecutable Windows

| Escenario | Resultado | Evidencia |
|---|---|---|
| Preparación, servicio, físicas, desastres y recursos | 52/52 | [Informe](../Evidence/Entrega05FinalCafe/smoke_result.json), [runtime](../Evidence/Entrega05FinalCafe/packaged_runtime.log) |
| Conversación real, voz, entrada y cancelación | 33/33 | [Informe](../Evidence/Entrega05DialogoFinal/dialogue_result.json), [runtime](../Evidence/Entrega05DialogoFinal/runtime.log) |

Son **85 comprobaciones automatizadas** en dos escenarios separados. La prueba de cafetería comprueba recetas, molienda, prensado, extracción, jarra caliente, vertido, servicio, reposición, derrames, resbalones, lanzamiento, rotura, sillas, testigos, combos, objetivos y recuperación. También confirma que una huida llega a la puerta y activa a la jefa, que los incidentes pequeños permanecen en silencio y que el enfriamiento descarta llamadas acumuladas. Las llegadas esperan durante las salidas para evitar bloquear la puerta. Las 24 mallas, 19 texturas y once materiales están disponibles en el paquete.

La conversación genera **ocho respuestas reales** con Qwen3 4B Instruct y valida sus JSON. Comprueba el pedido, la identidad de cliente de Cero Estrellas y el papel de camarero del jugador, la corrección «No, pides tú», conversación sobre Francia, recuerdo del perro Croqueta y recepción del café derramado. La respuesta física final dice «Me diste café en la camisa»: no invierte al agresor y la víctima. La reacción se aplica únicamente tras validar la respuesta completa; el diálogo normal no llama a la jefa.

También se comprueban entradas vacías o largas, campos incompletos, emociones desconocidas, cambios excesivos de enfado, bloqueo de turnos concurrentes, texto progresivo, memoria al reabrir, Escape tras perder foco y cierre durante una petición pendiente. La prueba aísla teclado y botones físicos externos para impedir envíos accidentales; esa protección solo se activa con `-CeroDialogueTest`.

Whisper transcribe un WAV español sintético al campo editable, sin enviarlo automáticamente. Tras confirmarlo llega al mismo pipeline de conversación. Piper produce cinco voces que alcanzan la reproducción en Unreal. El dispositivo real de micrófono se abre y se cierra; también se prueba la detención de una grabación demasiado corta. Esto verifica integración y reproducción, no una conversación hablada por una persona ni una evaluación auditiva de la voz.

## Tiempos y recursos medidos

[Mediciones por turno](../Evidence/entrega05_dialogue_latency.json), tomadas dentro del ejecutable con el modelo precargado:

| Muestra | Primeras palabras | Respuesta completa validada |
|---|---:|---:|
| Primer turno, sin prefijo conversacional en caché | 12,09 s | 19,94 s |
| Siete turnos siguientes | 4,41–9,07 s | 11,14–20,69 s |

El historial del prompt contiene palabras y roles; los hechos actuales y metadatos no se repiten en todos los intercambios. Conserva hasta doce turnos y 2500 caracteres de texto combinado. En el octavo turno, la [prueba anterior con historial redundante](../Evidence/Entrega05DialogoVerificado/runtime.log) tardó 28,36 s en mostrar palabras y 38,99 s en completar la respuesta; la versión final tarda 9,07 y 20,69 s, respectivamente. Alcanzar el límite de historial todavía puede requerir reevaluar contexto.

Esto mejora la presentación progresiva y esa espera larga; no demuestra una reducción general de inferencia frente a la versión 0.4. La mediana de cinco casos comparables fue 12,05 s con el modelo anterior de 3B y es 18,55 s con 4B. Se priorizó coherencia: una [comprobación adicional con 3B](../Evidence/compact_model_benchmark_qwen2.5_3b.json) negó un derrame recibido aunque mencionaba la camisa. La revisión semántica rechaza ese resultado; una coincidencia de palabras por sí sola no basta. Los ensayos exploratorios se conservan separados de los dos informes finales.

Ollama se verificó en CPU con `size_vram: 0` y contexto 2048: [recursos](../Evidence/entrega05_cpu_resources.json). Whisper tiny int8 y Piper también usan CPU. Una [instantánea durante el diálogo](../Evidence/entrega05_gpu_snapshot.csv) registró 2649 MiB usados de 6144 MiB en la GPU: incluye el escritorio y no representa un máximo medido. DX11, texturas con mipmaps y líquidos acotados mantienen un presupuesto contenido. El suavizado limita la partida a 60 FPS; no se ha demostrado que todas las escenas sostengan esa tasa.

## Capturas revisadas dentro del juego

- [Cafetera y cerámica](../Evidence/Entrega05FinalCafe/02_maquina_completa.png).
- [Jarra y vertido manual](../Evidence/Entrega05FinalCafe/02b_jarra_verter_a_mano.png).
- [Cliente, acabados y portátil](../Evidence/Entrega05FinalCafe/03_pedido_y_clientes.png).
- [Caída, personajes y testigos](../Evidence/Entrega05FinalCafe/07_silla_caida_y_testigo.png).
- [Conversación real sin nombres técnicos](../Evidence/Entrega05DialogoFinal/01_dialogar_respuesta_real.png).
- [Contexto físico y respuesta](../Evidence/Entrega05DialogoFinal/02_memoria_y_contexto_fisico.png).
- [Transcripción revisable](../Evidence/Entrega05DialogoFinal/03_transcripcion_whisper.png).

## Límites y repetición

No se ha completado una partida humana para valorar diversión, ergonomía, calidad auditiva o conversación con una voz humana por el micrófono. Los informes mantienen `human_playthrough: false` y `human_microphone_conversation_verified: false`. Los personajes usan articulación procedural y caídas animadas; los líquidos usan gotas y charcos limitados, sin simulación volumétrica. Los tres arquetipos comparten por ahora la voz Davefx.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\start_ai.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\dialogue_test.ps1 -Packaged -EvidenceName RepetirDialogo05
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\smoke_test.ps1 -Packaged -EvidenceName RepetirCafe05
```

Ejecutar los escenarios por separado evita dos partidas compitiendo por CPU. La arquitectura y los contratos están en [DIALOGUE_ARCHITECTURE.md](DIALOGUE_ARCHITECTURE.md). Los informes `DialogoPlayable` y `ModelosPlayable` conservan la validación histórica de 0.4 y no sustituyen las pruebas finales de esta entrega.
