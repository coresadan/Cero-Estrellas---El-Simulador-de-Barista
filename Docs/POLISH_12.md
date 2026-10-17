# Revisión 0.12 · Más salón, menos pantallas de serie

Trabajo del 17 de octubre de 2026 sobre UE 5.8.3. Cambios solicitados: quitar el bigote del personaje con sombrero, retirar los pinchos rosas, ampliar la cafetería, renovar el HUD y dar a los clientes distintas pertenencias electrónicas.

## Local y circulación

El local pasa de 13,8 × 12 m a 20,5 × 12 m, con seis mesas utilizables frente a las tres anteriores. El salón lateral añade tres mesas, una separación baja con plantas y un aparador con bandeja, tazas y una jarra físicas. La barra conserva sus máquinas y dimensiones de interacción.

`CECafeLayout.h` centraliza posiciones de mesas, sillas, desayunos, dispositivos y pasillos. Los seis visitantes esperan el pedido en posiciones distintas junto a la barra. Tras tomar nota, rodean las mesas por los pasillos y se sientan en su silla física. La admisión espera cuando las seis plazas están ocupadas.

La geometría fija se regenera con `author_cafe12.py`, manteniendo fuera del mapa los actores jugables. Hay iluminación precalculada y una captura de reflejos adicional para el salón. Es necesario reconstruir Lightmass después de cambiar la distribución.

## Clientes y pertenencias

El hipster conserva el sombrero y pierde el bigote superpuesto. El personaje de pelo rosa conserva su cabello base y pendientes, sin los cinco conos de la cresta.

Los visitantes alternan portátil, móvil, tablet y ausencia de dispositivo. Los aparatos se crean con su dueño y entran con él; la mesa no recibe electrónica al anunciar una llegada. El cliente lleva el portátil cerrado y baja sus pertenencias a la mesa al sentarse. Una vez colocadas, son actores físicos independientes con identidad de propietario.

Puedes cogerlos, desplazarlos, lanzarlos y mojarlos. El daño apaga la pantalla y reacciona su propietario. Un objeto robado, lanzado o roto no vuelve mágicamente a la mano del cliente. Las pertenencias que conserva se recogen al salir; los restos del desmadre tienen limpieza acotada. Se mantienen cuadernos, flores y bolsos asociados a las historias.

Los impactos de líquido sobre electrónica conservan salpicaduras y daño de pantalla. Se evita la mancha genérica de 85 cm usada por otros objetos, que ocultaba por completo un móvil o una tablet.

## HUD

La interfaz principal utiliza Slate y fuentes de interfaz, con tarjetas redondeadas y una paleta de verde oscuro, crema, menta y ámbar. El reloj y el servicio quedan separados de la puntuación y el objetivo. Los datos del recipiente aparecen en una tarjeta propia con volumen y temperatura. Las acciones contextuales y los subtítulos ocupan la zona inferior sin una franja que cubra toda la pantalla.

Mantener **Tab** muestra las seis comandas en una cuadrícula; **F1** abre los controles y la preparación del café. La conversación mantiene su panel existente y oculta el HUD mientras se edita. La interfaz se adapta al área de la ventana sin recibir clics ni bloquear los controles del juego.

## Regeneración y comprobaciones

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/build.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/build_graphics12.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/package.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/smoke_test.ps1 -Packaged -ResX 1920 -ResY 1080 -EvidenceName Entrega12Closure
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/smoke_test.ps1 -Packaged -Quick -Lumen -ResX 1920 -ResY 1080 -EvidenceName Entrega12Lumen
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/start_ai.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/dialogue_test.ps1 -Packaged -ResX 1920 -ResY 1080 -EvidenceName Entrega12Dialogue
```

La copia anterior del código y el mapa se conserva en `Saved/Polish12Baseline`. Los informes 0.11 son históricos y no validan estos cambios.

El mapa guardado contiene 671 actores fijos, 22 materiales de decorado y 11 luces. La reconstrucción de producción completó 630 asignaciones de Lightmass y guardó 27.174.640 bytes de datos de iluminación. La auditoría confirma que los actores interactivos se crean en la partida, sin duplicarlos en el mapa. El primer arranque de Swarm falló al conectar con el agente local; la repetición con el agente preparado terminó con cero errores. `build_graphics12.ps1` espera ahora a que su puerto local esté disponible.

Los pases iniciales de editor se conservan como diagnósticos en `Entrega12EditorPreview` y `Entrega12EditorReview`. El segundo aprobó 99 de 100 comprobaciones: varias texturas recién cargadas devolvían el tamaño provisional de 32 × 32 durante su compilación asíncrona. La comprobación espera ahora a `FTextureCompilingManager::FinishCompilation`, conservando el requisito de 512 × 512. El pase final `Entrega12EditorFinal` aprueba **101/101**.

`Entrega12StainDiagnostic` se interrumpió al detectar visualmente la mancha desproporcionada de la tablet. `Entrega12ServingDiagnostic` permitió comprobar su corrección y detectó una colocación inestable de la taza en el escenario automático: tras teletransportar al jugador, el agarre físico arrastraba el espresso desde la cafetera a varios metros y podía derramarlo antes de F. La prueba coloca ahora la taza ya preparada en el punto de agarre, conserva su cantidad y temperatura, comprueba que sigue siendo una receta válida y entrega con el control simulado real. Estos pases interrumpidos no cuentan como validación de entrega.

## Entrega verificada

El paquete final se ha probado en ventanas reales a **1920 × 1080**, sobre una **GTX 1060 de 6 GB**. Los tres escenarios usan el mismo ejecutable, verificado por SHA-256 contra el binario compilado. No se han registrado errores de materiales ni de RHI en los pases finales.

| Escenario | Comprobaciones | Capturas | p50 / p95 de fotograma |
|---|---:|---:|---:|
| DX11 precalculado, turno de ocho minutos reales | 172/172 | 30 | 16,67 / 20,44 ms |
| Lumen por software, regresión rápida | 167/167 | 28 | 19,93 / 22,38 ms |
| Diálogo local sobre DX11 | 43/43 | 5 | 24,54 / 41,24 ms |

Las comprobaciones nuevas cubren seis recorridos y asientos físicos, cuatro variantes de dispositivos, propietario y colocación correctos, admisión de un séptimo visitante, servicio en las seis mesas, panel Tab, agarre y devolución del móvil, rechazo de un dueño incorrecto, daño real por agua sobre la tablet y recogida del aparato al salir. Las capturas se han revisado para comprobar el salón, el bigote retirado, el pelo rosa, las comandas y los aparatos en pantalla.

El diálogo valida ocho respuestas generadas y reproduce ocho voces Piper; también comprueba transcripción con un WAV de prueba, interrupciones y regreso al juego. La observación del backend confirma Qwen3 4B en CPU, contexto 2048, cero VRAM del modelo y servicio de voz listo. Se conserva en [entrega12_backend.json](../Evidence/entrega12_backend.json).

Los tiempos miden reloj real entre ticks, incluyendo paradas, cargas y capturas. Los máximos fueron 348,66 / 374,87 / 342,00 ms respectivamente. Los escenarios tienen distinta duración y actividad; estos valores no son una comparativa controlada entre perfiles. **No se garantizan 60 FPS**, especialmente durante inferencia de diálogo.

El muestreo cada dos segundos registró picos de GPU del escritorio completo de 4705 / 5668 / 4191 MiB, y conjuntos de trabajo del juego de 703,7 / 1280,8 / 681,8 MiB. No son mediciones de VRAM exclusiva del juego ni aseguran capturar cada pico transitorio. No se observó otra instancia de juego durante estos pases. Para repetir el registro, iniciar `Scripts/profile_playable.ps1 -OnlyEvidence -EvidenceName <nombre del escenario>` en otra consola antes de cada prueba.

Los resultados se reúnen en [entrega12_delivery.json](../Evidence/entrega12_delivery.json); la [comparativa visual](../Evidence/Comparativa12/Comparativa.html) incorpora trece capturas reales de antes y después. Los tres pases finales suman **63 capturas de paquete verificadas a 1080p**. Para regenerar la galería y auditar la entrega: `py -3 Scripts/compare_polish12.py` y `py -3 Scripts/verify_delivery12.py`.

Ejecutable: `Release/Windows/CeroEstrellas.exe`, mediante **Jugar.bat**. SHA-256 del ejecutable real en `Release/Windows/CeroEstrellas/Binaries/Win64/CeroEstrellas.exe`: `682820de02cec8fb30079c73cc5d2bf3f0f0831a10fbb39e1ed13275993f025e`. El primer empaquetado cocinó el mapa y su iluminación; los siguientes reutilizaron ese contenido sin cambios y recompilaron el código. Logs: `entrega12_package_build.log`, `entrega12_package_build_final.log` y `entrega12_package_build_validation_fix.log`.

No se ha realizado una partida humana completa ni una conversación humana con micrófono. Quedan sin validar la diversión, el equilibrio y la ergonomía mediante juego humano. Las pruebas combinan preparaciones directas de escenarios, recorridos reales y controles simulados; no sustituyen esa valoración.
