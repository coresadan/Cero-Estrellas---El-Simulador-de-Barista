# Revisión gráfica 0.11

Trabajo iniciado el 16 de octubre de 2026 con Unreal Engine 5.8.3. Objetivo: cafetería estilizada, cálida y ligeramente decadente, con objetos legibles y clientela expresiva.

## Estructura e iluminación

`BuildCafe(true)` genera únicamente el decorado fijo para autoría. `author_cafe11.py` lo guarda en `/Game/Maps/Cafeteria` con instancias de material persistentes, UV de iluminación y capturas locales de reflejos. En la partida se recupera ese decorado mediante identificadores, mientras máquinas, puerta, sillas, clientes, pertenencias y líquidos siguen siendo actores jugables. Las comandas conservan su texto actualizable.

La base utiliza DX11, iluminación precalculada con Lightmass, luces de ventana amplias, lámparas cálidas y sol estacionario para sombras de objetos móviles. La iluminación indirecta de los objetos dinámicos procede del volumen de iluminación precalculado. El techo es más claro y la oclusión ambiental se reduce para evitar negros excesivos.

El perfil experimental arranca con DX12/SM6 y Lumen por software; convierte las luces del mapa en móviles y activa iluminación y reflejos Lumen. Se prepara el campo de distancia de las mallas. No utiliza trazado de rayos por hardware.

## Materiales, geometría y animación

- Metal con rugosidad estable y sin normal microscópica; nuevas capturas del propio local para sus reflejos. Cerámica, madera, pintura y piel con respuestas diferenciadas. Se conserva la mancha de tejido anclada al cuerpo y controlada por `Wetness`.
- Nueve cuerpos regenerados con mejillas más contenidas, hombros menos esféricos y dedos de perfil progresivo; se mantienen esqueleto, regiones de pesos y dimensiones de interacción. Fuente: `author_characters.py`.
- Cejas curvas originales, parpadeo gradual, movimiento bucal suavizado, respiración y pequeños cambios de postura. Alcance de teléfono y protección del desayuno con mezcla gradual. Las poses continúan siendo procedurales y no constituyen captura de movimiento.
- Orejas ajustadas al ancho de cada cabeza. El recipiente agarrado presenta el asa a la derecha y la mano se coloca por su exterior; se corrige el antebrazo que atravesaba la taza al verter. El HUD recibe una sombra discreta y paneles más legibles.
- Charcos con contorno irregular y borde redondeado, material húmedo propio y vapor que aparece y desaparece gradualmente. Permanecen los presupuestos de 192 paquetes, ocho chorros, 32 charcos de cinco lóbulos, 48 salpicaduras y 24 volutas. Los efectos visuales no deciden cantidades ni impactos.
- `Jugar.bat` solicita 1080p en DX11; `Jugar_Lumen_Experimental.bat` solicita el perfil alternativo. La resolución efectiva depende de la ventana y se audita en las capturas de prueba.

## Regeneración

Compilar el editor antes de importar. Ejecutar los comandos de forma secuencial desde la raíz:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --python Scripts/author_characters.py
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --python Scripts/author_details11.py
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/build.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/build_graphics11.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/package.ps1
```

La regeneración del mapa reemplaza los actores etiquetados `CEArchitecture`. Los cambios manuales en esos actores deben trasladarse a la fuente antes de regenerar. Si cambia el orden o la cantidad de elementos de `BuildCafe`, hay que regenerar el mapa y reconstruir su iluminación antes de empaquetar. Se conserva una copia del mapa anterior en `Saved/Polish11Baseline` para diagnóstico.

Para repetir la validación del paquete, ejecutar cada escenario por separado. En otra consola, `profile_playable.ps1 -EvidenceName NOMBRE -OnlyEvidence` recoge recursos mientras se ejecuta el escenario con ese nombre:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/smoke_test.ps1 -Packaged -Quick -ResX 1920 -ResY 1080 -EvidenceName Entrega11Closure
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/smoke_test.ps1 -Packaged -Quick -Lumen -ResX 1920 -ResY 1080 -EvidenceName Entrega11Lumen
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/start_ai.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/dialogue_test.ps1 -Packaged -ResX 1920 -ResY 1080 -EvidenceName Entrega11Dialogue
py -3 Scripts/compare_graphics11.py
py -3 Scripts/verify_delivery11.py
```

Las pruebas fuerzan 1920 × 1080 y renderizan en ventana; los PNG se auditan por sus dimensiones reales. `-Quick` omite la espera del turno completo. Para probar también sus ocho minutos, omitir ese modificador y utilizar otra carpeta de evidencia.

Desde esta revisión, los tiempos se miden con reloj real entre ticks, después de diez segundos de simulación. Incluyen cargas, preparación de escenas y capturas del recorrido automático; el máximo ya no queda recortado por el límite del paso de simulación. No son una medición aislada de GPU ni se deben comparar directamente con los máximos históricos basados en `DeltaSeconds`. La VRAM procede de `nvidia-smi`, incluye todo el escritorio y se muestrea cada dos segundos.

## Incidencias corregidas durante el cierre

- Las nuevas mallas de cejas y charcos recibieron UV explícitas antes de su importación definitiva.
- Lightmass activaba en memoria el uso de iluminación estática de algunos materiales. `finalize_cafe11.py` persiste esas banderas antes del cocinado y comprueba que el mapa guardado no contiene máquinas ni clientes de prueba.
- La revisión de capturas detectó orejas separadas de cabezas estrechas y el antebrazo atravesando la taza. Se ajustaron proporciones y agarre antes de repetir las pruebas.
- `Entrega11ChainDiagnostic` y `Entrega11DepartedWitnessDiagnostic` conservan recorridos con fallos en la cadena de impactos. La escena reutilizaba posiciones de una caída anterior y un testigo que podía haberse marchado ya. Ahora crea dos clientes nuevos en posiciones conocidas, sin contadores de impactos previos, y mantiene la colisión real y la huida como condiciones de éxito.
- `Entrega11LumenPrecacheDiagnostic` conserva el primer recorrido DX12, con errores al precargar variantes de shaders y fallos funcionales. El lanzador experimental desactiva únicamente la precarga global de PSO mediante `r.PSOPrecache.GlobalShaders=0` al arrancar. Los shaders usados por el renderizado siguen siendo obligatorios: la prueba rechaza errores RHI y de materiales. `Entrega11LumenStartupDiagnostic` aprobó 86 comprobaciones de la parte visual sin esos errores; es una prueba parcial, no el recorrido completo de entrega. Esta opción puede aumentar los tirones al usar un efecto por primera vez.
- `Entrega11GraphicsReview` contiene capturas provisionales anteriores a los ajustes finales de manos y orejas. `Entrega11EditorPreview` se interrumpió durante la compilación de shaders. Ninguna de esas carpetas valida el paquete final.
- `Entrega11ProfilerRaceDiagnostic` aprobó las 151 comprobaciones, pero el muestreador señaló un posible segundo juego al mezclar dos inventarios de procesos tomados en instantes distintos. Se cambió a un único inventario, se guardan los identificadores observados y se repitió la medición: únicamente aparecen el lanzador y el proceso de ese escenario.

## Validación

Validación cerrada el 17 de octubre de 2026. El editor y el juego compilan con UE 5.8.3 y el empaquetado termina correctamente. Lightmass terminó con **0 errores**. `Cafeteria_BuiltData.uasset` ocupa 19.669.460 bytes. La auditoría en partida encuentra **454 mallas fijas con mapas de luz reales**, dos capturas de reflejos e identificadores únicos; el mapa guardado contiene 483 actores de decorado y ningún actor jugable precargado. La deformación de los nueve cuerpos está aprobada.

Ambos perfiles han aprobado **151/151 comprobaciones** sobre el mismo ejecutable empaquetado. Cada recorrido produce 23 capturas a 1920 × 1080. Los registros verifican DX11/SM5 en el perfil predeterminado y DX12/SM6 con iluminación y reflejos Lumen en el experimental. No aparecen errores RHI, de compilación de materiales ni materiales ausentes en los recorridos finales.

Mediciones de estos recorridos en la **GTX 1060 de 6 GB**, ejecutados por separado y sin otro juego observado. Las dos primeras filas no tienen inferencia simultánea:

| Perfil | Mediana | p95 | Máximo | VRAM total del escritorio, pico | Memoria residente del juego, pico |
|---|---:|---:|---:|---:|---:|
| DX11 precalculado | 16,67 ms | 20,35 ms | 361,93 ms | 3.757 MiB | 683,5 MiB |
| Lumen experimental | 18,06 ms | 20,17 ms | 399,86 ms | 4.823 MiB | 1.262,7 MiB |
| DX11 con diálogo local | 25,99 ms | 42,27 ms | 327,89 ms | 3.747 MiB | 637,4 MiB |

El p95 es parecido en estos dos recorridos; no demuestra una ventaja general de rendimiento. Lumen usa unos 1.066 MiB adicionales de VRAM del escritorio y aumenta la memoria del juego. DX11 queda como opción predeterminada por su margen de recursos y la iluminación más uniforme del local. Persisten picos durante el recorrido automático; el límite de 60 FPS no garantiza 60 FPS constantes.

La regresión de diálogo aprueba **43/43 comprobaciones**, con ocho respuestas validadas de Ollama, ocho voces preparadas anticipadamente y seis voces Piper reproducidas, además de una respuesta de servicio con voz. Whisper procesa el WAV de prueba y el dispositivo de micrófono se abre; eso no valida una conversación humana. `entrega11_backend.json` confirma Whisper y Piper en CPU y el modelo Qwen3 4B con `size_vram=0`. La inferencia compite por CPU: el p95 de 42,27 ms demuestra que todavía falta margen para mantener 60 FPS durante conversación. Los escenarios de diálogo y jugabilidad tienen cargas distintas; sus cifras describen cada recorrido, no una comparación controlada de una sola variable.

Se han revisado capturas reales de máquinas, clientela, agarres, clientes sentados y charcos. El perfil precalculado hace más legibles techo y paredes y separa mejor los materiales; Lumen presenta más contraste y ruido visible en algunas superficies. La comparación utiliza escenarios automatizados equivalentes de 0.10 y 0.11, con variaciones posibles en animación, visitantes y objetos desplazados por la física. Los resultados de 0.10 son históricos y no validan esta revisión.

Entrega: [comparativa visual](../Evidence/Comparativa11/Comparativa.html), [manifiesto y comprobación del ejecutable](../Evidence/entrega11_delivery.json), [jugabilidad DX11](../Evidence/Entrega11Closure/smoke_result.json), [Lumen](../Evidence/Entrega11Lumen/smoke_result.json) y [diálogo](../Evidence/Entrega11Dialogue/dialogue_result.json). Las 51 capturas de los tres recorridos son nuevas y tienen 1920 × 1080 píxeles. El manifiesto verifica que sus tres identidades SHA-256 corresponden al ejecutable empaquetado y a la compilación actual.

La validación automática no sustituye una partida humana. Esta entrega usa `-Quick`: no repite la espera del turno completo de ocho minutos. Tampoco valida una conversación humana por micrófono. Los informes conservan expresamente esas limitaciones.
