# Hito 68 — audio original

Fecha de cierre técnico: 7 de septiembre de 2026.

## Resultado

Se incorporó audio original con backend SDL3 y fallback nulo, mezclador estéreo,
buses maestro/música/efectos, límite de voces, loops y lifecycle por escena. El
listener usa la cámara local; la primera persona permanece estéreo y las fuentes
remotas, impactos, recogibles y nueve emisores de Factory se espacializan.

Los eventos se originan en la autoridad y se replican como identificadores
semánticos con secuencia. Un diario de 128 eventos cubre el peor caso de los 73
recogibles en un tick; cada snapshot transporta solo la ventana variable del último
segundo. Esta redundancia tolera pérdidas sin saturar el enlace adverso de 32 KB/s.
Primer snapshot, snapshots repetidos/reordenados, cambio de escena y reconexión
establecen línea base y no repiten one-shots. El contrato incompatible eleva el
protocolo de 18 a 19.

## Recursos

`tools/legacy/audit_audio.py` auditó los 89 archivos Legacy y produjo
`assets/audio/inventory.json`: 51 recursos referenciados, 35 huérfanos, 3 de
`troll`, 3 referencias ausentes (`Gun.wav`, `plasma.wav`, `shootFireball3.wav`)
y un grupo duplicado (`fisico.wav`/`supermalla_escenario1.wav`). Se seleccionaron
37 recursos: los usados por el gameplay restaurado, la música y ambientes de
Factory, más explosión e ignición justificadas explícitamente en el inventario.
`troll` quedó excluido.

Todos los WAV/OGG/MP3 se decodificaron para validar formato, canales, frecuencia,
duración, tamaño y muestras finitas. Los seleccionados se conservaron bajo
`assets/audio/original` y se cocinaron como PCM flotante GAU1 bajo
`assets/audio/cooked` (21.362.236 bytes). CMake los copia a `content/audio`; el
runtime solo lee `game:/`/`cache:/` y no depende de Python, FFmpeg o FMOD.

## Validación

- Auditoría reproducible `--check`: 89 auditados, 37 importados, 3 referencias
  ausentes conocidas, 1 grupo duplicado.
- Cooker focalizado: WAV, OGG y MP3 válidos; corrupción de los tres formatos falla
  con nombre y diagnóstico.
- 11/11 pruebas focalizadas: mezclador/carga/corrupción, buses, voz/loop/stop,
  panning/atenuación, salida nula, eventos de personaje, cinco armas, recogibles,
  red de dos clientes, snapshot tardío, pérdida/reordenación/deduplicación,
  reconexión, simulación, transporte, protocolo, arsenal, movimiento y servidor.
- Smoke real de SDL3: abrió `Headphones (High Definition Audio Device)` y completó
  la secuencia de 58 s. `sequence.txt` registra 29 tramos; `review.wav` conserva
  la misma secuencia para escucha posterior. Ambos son artefactos regenerables y
  están excluidos por `.gitignore`.
- Smoke Vulkan/Jolt final completado: 2 bajas, 720 lotes de entrada, 4 disparos
  autorizados, 0 rechazos y 120 reconciliaciones. El servidor dedicado completó
  tres ticks incluso forzando un driver SDL de audio inexistente.

El smoke demuestra apertura, alimentación y cierre del dispositivo. La confirmación
subjetiva de volumen, timbre y espacialización por una persona queda pendiente;
las pruebas y la inspección numérica no pueden sustituirla.

## Comandos

```powershell
.cache/legacy-tools/Scripts/python.exe tools/legacy/audit_audio.py D:/Projects/Gloom-Legacy --check
.cache/legacy-tools/Scripts/python.exe tools/legacy/test_audio_cooker.py D:/Projects/Gloom-Legacy
D:/Dev/CMake/bin/ctest.exe --preset windows-debug -R "^gloom\.(audio|audio_network|audio_no_device|vertical_slice|vertical_slice_network|vertical_slice_transport|slice_server_smoke|legacy_arsenal|legacy_pickups|legacy_movement|network_protocol)$"
build/windows-vs/Debug/gloom.exe --audio-review reports/audio-2026-09-06 --device
```
