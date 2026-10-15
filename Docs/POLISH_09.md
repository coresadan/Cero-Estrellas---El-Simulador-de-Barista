# Revisión 0.9: presentación, líquidos e historias

Corrección de la revisión visual, 14 de octubre: la existencia de las nuevas mallas no garantizaba su uso en la escena. El cargador buscaba los nombres `SM09_` en una ruta incorrecta, por lo que faltaban los tableros y otros detalles. También se detectó la orientación incorrecta de portátiles. [Revisión 0.10](POLISH_10.md) registra las correcciones y las pruebas nuevas; los resultados de 0.9 se conservan como evidencia histórica, con esta limitación.

Entrega 0.9 cerrada el 13 de octubre de 2026, Unreal Engine 5.8.3. [Manifiesto y ejecutable verificado](../Evidence/entrega09_delivery.json). Las pruebas 0.8 son históricas.

| Punto aprobado | Implementación de esta revisión |
|---|---|
| 1. Gráficos | Carcasa de cafetera con hueco frontal, mesas redondeadas, lámparas torneadas, bollería continua y madera de grano fino; ajuste de luz y contraste. |
| 2. Líquidos | Colisión a 30 Hz y representación interpolada; ocho chorros de ocho tramos, 192 paquetes de volumen y charcos acotados. Superficie nivelada, oscilación y espuma con dibujo propio. |
| 3. Personajes | Tres cuerpos originales regenerados con formas distintas de torso y cabeza, puentes de hombro, cuello de ropa y botones; nueve alturas y accesorios. Dos LOD por cuerpo: unos 35–36 mil vértices importados de cerca y 12–13 mil a distancia. |
| 4. Animación | Sorbo progresivo con alcance de brazo, gestos sociales y de llamada; taza liberada durante caída/salida y destrucción del cliente. Consulta del suelo a menor frecuencia. |
| 5. Preparación | Rueda del ratón mirando a la máquina: extracción de 30/60/90 ml o leche a 55/65/75 °C. Manómetro animado; no cambia el ajuste mientras trabaja. |
| 6. Vida social | Conversación tranquila entre mesas con visibilidad y pausa, identidades recíprocas y llegadas relacionadas. |
| 7. Historias | Presentación, incidente o llamada, preparación del desenlace y resolución. Cinco situaciones; el jugador puede escribir el verso del poema. |
| 8. Sabotaje | Superficie y chorro más legibles, conservación de cantidades e ingredientes, manchas y charcos, comida adherida y caídas en cadena conservadas. |
| 9. Conversación | Nuevos atajos de saludo, incidente, apoyo y promesas; memoria de la promesa y de acompañantes en contexto. Conversación libre estructurada y cancelable, con respuestas breves y validación de perspectiva. |
| 10. Jefa | Tres grabaciones adicionales, campañas vinculadas a los grandes desastres, memoria de víctima y tipo; pausa de 50 s conservada. Las campañas modifican parte de las siguientes llegadas. |
| 11. Sonido | Capas espaciales de calle y sala, llamada y ensayo musical; reducción suave del ambiente durante la radio. |
| 12. Estabilidad | Precarga de voces y cuerpos, caché de efectos, materiales estáticos compartidos y reutilización de gotas, chorros y vapor. Contadores y marcadores de CPU para medir el líquido. |

## Alcance de los líquidos

Los paquetes transportan volumen y composición; el dibujo continuo no decide impactos. Un paquete conserva sus cantidades hasta ser aceptado por el pool y una taza llena devuelve el excedente al suelo. No hay rejilla de fluidos ni física individual de gotas decorativas. Charcos y chorros tienen un presupuesto fijo. El renderizado usa mallas instanciadas y materiales opacos, con vapor translúcido limitado.

La comparación automatizada usa el mismo solucionador nuevo a 60 y 30 Hz, con 32 paquetes y 64 ml sobre el mismo suelo. Comprueba conservación y reducción de consultas; no equivale a comparar todos los costes de la versión 0.8 contra 0.9. Los contadores de la partida y el muestreo de recursos se informarán por separado.

## Historias y ritmo

Las historias avanzan desde los hechos del juego, no desde una orden del modelo. La entrevista tiene una llamada; la cita y las reservas pueden relacionar visitantes; el crítico toma notas; la ópera tiene ensayo; el poema acepta `verso: ...`. Servir y apoyar son necesarios para el desenlace positivo. Una agresión puede resolver el fracaso antes; cada historia puntúa una sola vez.

Las conversaciones autónomas respetan visibilidad, distancia, pausa y una conversación activa con el jugador. Los avisos de historia no pueden activar la radio por una puntuación de desastre anterior. Las llamadas de la jefa siguen siendo poco frecuentes y no forman una cola. Las campañas son consecuencias de una llamada por un desastre importante, no un comentario por cada gota.

## Validación de entrega

- [Turno completo de ocho minutos](../Evidence/Entrega09Delivery/smoke_result.json): **142/142**. Se ejecutó antes de ajustar la longitud/grosor visual del chorro y los guardas de diálogo; el código de fases y cierre no cambió después.
- [Regresión sobre el ejecutable definitivo](../Evidence/Entrega09ReleaseChecked/smoke_result.json): **137/137**, incluidos preparación, volcado, transferencia, cuerpo, contacto en cadena, historias, promesas, selector, recursos y radio. Coincidió con cinco consultas narrativas al backend; no se usa como medición aislada de rendimiento.
- [Conversación definitiva](../Evidence/Entrega09DialogoReady/dialogue_result.json): **42/42**, ocho respuestas generadas reales, memoria, roles, incidente, Whisper, Piper, caché y cancelación. Dos comprobaciones nuevas rechazan inversiones explícitas de cliente/camarero y disculpas de la víctima por el café recibido. Se abrieron el dispositivo y el flujo de captura; la transcripción comprobada procede del WAV de prueba, no de una conversación humana.
- [Deformación](../Evidence/entrega09_deformation.json): tres cuerpos aprobados, ningún vértice de pantalón sigue los brazos. [LOD](../Evidence/entrega09_lods.json): aproximadamente un 64 % menos de vértices a distancia.
- [Contexto de las cinco historias](../Evidence/Entrega09Stories/story_dialogue_result.json): cinco consultas reales aprobadas directamente por el backend. Comprueban contrato y palabras de contexto; no son cinco partidas narrativas ni una revisión semántica humana.

### Líquidos y recursos

La [comparación controlada](../Evidence/Entrega09FinalQuick/liquid_comparison.json) conserva **64 ml** en ambos modos: **1120 → 544 consultas**, reducción **51,4 %**. Tiempo del solucionador de esa escena: **0,936 → 0,483 ms** acumulados durante un segundo simulado. No se interpreta como una reducción del 51 % de todo el coste del juego. La predicción del extremo visual usa hasta ocho consultas cada 0,25 s por chorro activo y se incluye en el contador de consultas de la partida; no consume volumen ni activa impactos.

El [turno sin inferencia](../Evidence/Entrega09Delivery/resources_summary.json) tomó 224 muestras: GPU del escritorio **3491/6144 MiB**, juego **576 MiB**; p95 **16,67 ms**, máximo **152,05 ms**. La [prueba rápida aislada de los chorros finales](../Evidence/Entrega09FinalQuick/resources_summary.json) registró **3453/6144 MiB** y **574,5 MiB** de juego. Sus cambios posteriores fueron de validación/reintento del diálogo.

Con [conversación real](../Evidence/Entrega09DialogoReady/resources_summary.json): GPU del escritorio **3597/6144 MiB**, juego **558 MiB**, p95 **22,94 ms**, máximo **137,76 ms**. Las cifras de GPU incluyen otras aplicaciones y se muestrean cada dos segundos. Todas estas pruebas usan DX11 a 1280 × 720, renderizado real sin ventana visible y límite de 60 FPS. No garantizan el mismo resultado con una ventana ni eliminan pausas puntuales.

### Conversación y diagnóstico

[Latencias](../Evidence/entrega09_dialogue_latency.json): texto visible **4,99–10,44 s**, respuesta validada **13,54–23,46 s** en turnos posteriores. Mediana de cinco turnos comparables **15,55 s**, frente a **18,34 s** en 0.8, un **15,2 % menor** en esta medición. No prueba una ganancia universal: las respuestas y carga de CPU varían. Los atajos de servicio son inmediatos y se cuentan por separado.

La variante de prompt de 64 caracteres produjo una inversión de papeles y se descartó; se restauraron 80 caracteres. El reintento añade los hechos del cliente y conserva el tiempo total. Se guardan trazas `accepted_*.json` separadas de respuestas rechazadas, para que el resumen no mezcle intentos. La conversación final no necesitó rechazos; la protección de perspectiva se comprueba también con dos regresiones directas. Los recorridos fallidos/interrumpidos se conservan como diagnóstico y no cuentan como validación de entrega.

### Capturas revisadas

- [Máquina y manómetro](../Evidence/Entrega09FinalQuick/20_cafetera_y_materiales.png).
- [Chorro](../Evidence/Entrega09FinalQuick/04_liquido_y_charcos.png) y [latte con extras](../Evidence/Entrega09FinalQuick/13_latte_espuma_y_extras.png).
- [Clientela](../Evidence/Entrega09Delivery/18_clientela_extravagante.png) y [caída](../Evidence/Entrega09FinalQuick/22_caida_y_campana.png).
- [Cierre del turno](../Evidence/Entrega09Delivery/11_despido_denegado.png).
- [Conversación](../Evidence/Entrega09DialogoReady/01_dialogar_respuesta_real.png) y [reacción al café recibido](../Evidence/Entrega09DialogoReady/02_memoria_y_contexto_fisico.png).

### Reproducir

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/smoke_test.ps1 -Packaged -Offscreen -EvidenceName OtraPrueba09
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/start_ai.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/dialogue_test.ps1 -Packaged -Offscreen -EvidenceName OtroDialogo09
```

Ejecuta una sola instancia del juego. `-Quick` omite las esperas de las fases finales; `-PolishOnly` concentra los bloques de revisión. `Scripts/author_professional09.py`, `generate_professional09.py`, `import_professional09.py`, `refresh_characters09.py` y `configure_professional_lods.py` conservan las fuentes y regeneración de esta revisión. Abre **Jugar.bat** para jugar.

La animación continúa siendo de poses y claves propias, con alcance y mezcla; no se presenta como captura de movimiento. Los nueve perfiles comparten tres cuerpos base. El aspecto es estilizado, y las voces comparten un modelo con ajustes. La latencia libre debe medirse: los atajos inmediatos no demuestran que el modelo genere más rápido. Las pruebas automatizadas no valoran por sí solas diversión ni sustituyen una partida humana.
