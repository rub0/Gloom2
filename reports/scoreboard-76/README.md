# Hito 76 — marcador de partida

## Alcance

Se migró el marcador que Gloom Legacy mostraba al mantener Tab. La referencia
funcional fue `ScoreDM.swf` y su integración en `Game.cpp`: presentación temporal
durante la partida y estadísticas por jugador. Como el SWF de duelo no contiene
bitmaps reutilizables, el panel se reconstruyó con el sistema visual actual.

## Comportamiento

- Tab muestra el panel mientras permanece pulsado y soltarlo lo oculta.
- El juego no se pausa ni se bloquean movimiento, habilidades o disparo.
- Cada fila muestra nombre, clase, bajas y muertes.
- Las filas se ordenan por bajas descendentes y, en empate, por menos muertes.
- El jugador local queda resaltado y los nombres online se resuelven por entidad.
- La partida local usa `JUGADOR` y `RIVAL` si no existe estado de sala.

Los contadores proceden directamente de los dos `CombatantView` del snapshot.
No se añadió estado duplicado ni cambió el protocolo de gameplay, que permanece
en la versión 22.

## Validación

La galería `--ui-review` tiene ahora 17 estados. La página 16 fija el marcador
con puntuaciones distintas, orden y resaltado local. Se inspeccionaron las
capturas completas a 1280×720, 1920×1080 y 2560×1080 y se añadieron sus tres
referencias reducidas a la regresión visual.

```text
cmake --build build/windows-vs --config Debug --target gloom gloom_visual_capture_tests
gloom.exe --ui-review .cache/scoreboard-76
gloom_visual_capture_tests.exe .cache/scoreboard-76 tests/ui-visual-references --ui
ctest --test-dir build/windows-vs -C Debug -R "gloom.ui_(visual_review|flow)" --output-on-failure
```
