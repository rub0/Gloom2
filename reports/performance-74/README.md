# Hito 74: Factory, medición comparable y sombras

9 de septiembre de 2026. Windows, Ryzen 7 3700X, NVIDIA GTX 1070, Vulkan.

## Método y correcciones

Los hitos 71/73 comparaban escenas distintas. `--vertical-slice-smoke` excluye
Factory original y acaba con unas 31 instancias; el perfil original incluía
Factory y unas 227. Sus 108 FPS tampoco medían simulación ni el loop completo.
No se puede atribuir su diferencia a resolución, culling o streaming.

El benchmark nuevo espera Factory, personajes solicitados, armas, habilidades
y efectos; calienta 120 frames y mide 360. Simulación fija a 60 Hz, input vacío,
cámara inicial de Factory y AI local. Cuenta desde después de poll_events hasta
end_frame/Present, incluyendo actualización, carga residual, animación, listas,
visibilidad, iluminación y envío. Excluye arranque, calentamiento, HUD, audio,
capturas y el propio informe. Poll_events tampoco entra en el intervalo.
Resolución nativa fija, VSync desactivado, sin resolución dinámica. No representa
todo combate, cámara, carga de efectos o sesión multijugador.

La muestra incompleta (120 s de límite o cierre) devuelve código 1. Las antiguas
exenciones del smoke ya no afectan este modo, que tiene sus propias condiciones
de carga; el smoke habitual conserva su validación.

## Cambios

- Eliminada la pausa de 16 ms por frame jugable. Ese límite artificial impedía
  100 FPS incluso con CPU/GPU sin carga. VSync opcional: `GLOOM_VSYNC=1`.
- Descarte conservador por esfera contra los seis planos de cada cascada en
  espacio de luz, antes de binds/draws. Matriz local-to-clip incorpora escala
  no uniforme y reflejos. Reutiliza la matriz para el draw que sobrevive.
- Lista completa persistente y dos listas animadas intercambiadas entre frames;
  se conserva la capacidad en vez de copiar/reservar desde cero cada frame.
- Perfil con arrays fijos, reloj monotónico, percentiles y seis etapas CPU;
  tipos propios en `include/gloom/core/types.hpp`. Sin nueva dependencia ni
  nuevas asignaciones de memoria para almacenar muestras. Memoria de proceso
  consultada solo al finalizar en Windows; fallback POSIX de reloj no probado aquí.
- Timeout de carga de revisión visual en segundos en lugar de frames: el
  descarte aceleró los frames de carga y expuso el límite de 1800 frames.

La base del proyecto aún usa STL/excepciones/RTTI en módulos anteriores. Este
hito no migra toda esa arquitectura; los nuevos helpers usan C, tipos propios
y almacenamiento fijo. Las listas existentes conservan sus tipos mediante
`decltype` y reutilizan capacidad. No se ha hecho profiling por pila de cada
asignación ni se ha demostrado una reducción sustancial de memoria residente.

## Resultados

| Configuración | Media ms | FPS del loop | p95 ms | p99 ms | Máximo ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Debug 1080p, benchmark corregido antes de descarte/reuso | 41,273 | 24,23 | 43,883 | 47,356 | 54,627 |
| Debug 1080p, después | 32,934 | 30,36 | 34,712 | 36,150 | 37,535 |
| Release 1080p, final | 6,936 | 144,17 | 6,954 | 6,964 | 7,111 |
| Release 720p, final | 1,829 | 546,76 | 2,957 | 2,992 | 3,036 |

Las cuatro pasadas terminan con 224 instancias, 85 visibles y 35 batches.
Debug reduce 20,2 % el tiempo medio: draw/Present baja de 33,526 a 25,298 ms;
presentación pasa de 4,020 a 3,983 ms. No se midió el efecto individual de cada
reserva. Visibilidad caliente cuesta 0,232 ms: los 6,85 ms anteriores estaban
contaminados por carga/esperas. `JobSystem::wait` puede ejecutar trabajos de
carga ajenos al grupo esperado.

Release 1080p: update 0,144 ms, begin 0,030, presentación 0,273, visibilidad
0,033, iluminación 0,039 y draw/Present 6,419. Últimas consultas disponibles de
pasadas GPU suman 2,328 ms; no son una media ni cubren necesariamente UI/Present.
Los frames emitidos en modo mailbox no equivalen al número de imágenes que
el monitor muestra. La cadencia ~144 Hz puede incluir regulación de presentación.

Memoria Release 1080p: privados 2085,09 MiB, working set 1132,79 MiB, pico de
working set 1414,21 MiB. La memoria global Diligent incluye picos de carga y
reservas de páginas; no debe confundirse con RAM privada del proceso.
El objetivo de 10 ms se cumple en esta muestra Release, no en Debug.

## Reproducción y validación

```powershell
D:/Dev/CMake/bin/cmake.exe --build build/windows-vs --config Release --target gloom --parallel 6
build/windows-vs/Release/gloom.exe --vertical-slice-performance-1080p
build/windows-vs/Release/gloom.exe --vertical-slice-performance-720p
```

Builds finales Debug/Release correctos; ambas muestras finales devuelven 0.
Siete pruebas focalizadas únicas correctas: render_scene (incluye nuevos casos
de seis planos, tangencia y escala reflejada), visibility, factory_visual_review,
character_visual_review, vulkan_sync, ui_flow y vertical_slice_smoke.
No se ejecutó una nueva suite completa.

Las seis imágenes Factory y siete de personajes son idénticas píxel a píxel a
las referencias. Hubo un fallo inicial de timeout de carga de Factory y pasó
tras corregir el límite temporal; no se modificaron referencias.
Logs regenerables ignorados: `build/profile74-*.log`; comparaciones visuales en
`build/windows-vs/factory-review/Debug/comparison.txt` y
`build/windows-vs/character-review/Debug/comparison.txt`.

Siguiente frente: sesión Release con HUD/audio y combate; estudiar retención de
escenas decodificadas, texturas y buffers CPU (~2 GiB privados), perfil por pilas
de asignación y coste de skinning con más combatientes. No se ha reproducido
exactamente la sesión humana que reportaba 13 FPS.
