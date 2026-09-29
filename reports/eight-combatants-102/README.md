# Hito 102 — carga y rendimiento de ocho combatientes

29 de septiembre de 2026 · Investigación ejecutada. **El objetivo de 200 FPS/5 ms a 1080p no queda validado.**
Este hito termina con una medida reproducible y dos bloqueos concretos; no aprueba el presupuesto Hound H06 ni inicia H07.

## Escenario y método

El benchmark de Factory ahora tiene una variante local con siete Archangel TPS animados y brazos FPS de la presentación original (selección jugable Hound), sin ampliar la autoridad de dos jugadores ni el protocolo. Cada cuerpo TPS tiene su propio animador, skinning y bounds; se mueve lateralmente con fase fija, porta Soul Reaper y proyecta sombras. El jugador local dispara; se procesan simulación, efectos de combate, HUD y audio con salida nula. La salida nula ejecuta la presentación de audio sin medir el dispositivo físico. La captura de calentamiento muestra siete cuerpos distinguibles y brazos FPS. En los 360 frames medidos hay ocho instancias skinned y ocho pasan el frustum; el benchmark falla si deja de cumplirse.

Cámara FPS fija en Factory: jugador en torno a `(-35,49; 0,91; 9,00)` en la captura, orientación inicial del juego. Los siete TPS ocupan dos filas a 4,5 y 7,0 m, separados lateralmente 0,8 m con oscilación de 0,25 m. Se usan los assets jugables actuales, sin mapas ni clips finales de Hound v16. No hay siete actores de gameplay autónomos ni una partida de ocho clientes. El combate se ejercita con el disparo local, no con una simulación autoritativa de ocho participantes.

Windows, Ryzen 7 3700X, GTX 1070, 1920×1080 de render y salida nativos, sin resolución dinámica. VSync está desactivado; Diligent registra `VK_PRESENT_MODE_MAILBOX_KHR` como modo efectivo tras inicializar. Simulación fija a 60 Hz, 120 frames de calentamiento y 360 de medida, tras carga de recursos. El reloj CPU cubre desde `poll_events` hasta `end_frame/Present`, incluido HUD/audio; el reloj GPU cubre de la primera orden de render hasta la última orden de UI, excluido Present. Las consultas GPU se leen después de cuatro frames: 359 muestras válidas. Los resultados son tiempos del equipo en esta sesión, no un límite de producción universal.

## Resultados Release

| Caso | Media/FPS | p95 | p99 | Máximo | GPU media/p99 | CPU `end_frame/Present` medio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Dos, HUD/audio/disparo A | 6,937 ms / 144,16 | 6,990 | 7,164 | 7,252 | 3,683 / 4,457 ms | 5,577 ms |
| Ocho A | 3,525 ms / 283,65 | 5,259 | 5,441 | 5,460 | 2,339 / 2,782 ms | 1,571 ms |
| Dos, HUD/audio/disparo B | 2,981 ms / 335,49 | 5,011 | 5,105 | 5,124 | 2,160 / 2,626 ms | 1,837 ms |
| Ocho B | 6,937 ms / 144,15 | 6,993 | 7,111 | 7,195 | 2,266 / 2,452 ms | 4,976 ms |
| Ocho, control 1280×720 | 1,975 ms / 506,21 | 2,282 | 2,816 | 3,548 | 1,384 / 1,604 ms | 0,150 ms |

Los dos regímenes aparecen con el mismo ejecutable, VSync desactivado y modo MAILBOX. La cadencia de ~144 FPS se reproduce también con dos combatientes: no demuestra saturación GPU de los ocho. En esos pases, `end_frame/Present` absorbe ~5 ms y el render GPU de ocho promedia 2,266 ms. En el régimen rápido, la media supera 200 FPS, pero p95/p99 de ocho exceden 5 ms. **El objetivo de 5 ms por fotograma completo no pasa**, tampoco por usar solo la media de los pases rápidos. La variación de presentación impide atribuir una diferencia precisa por combatiente entre ejecuciones; no hay ajuste de motor justificado aún. El siguiente experimento debe controlar el modo/cadencia de presentación y medir la latencia de la cola junto con tiempos GPU, manteniendo 1080p.

En el último frame de ocho a 1080p: 243 instancias enviadas, 110 visibles, 37 batches, 406 draws en todas las pasadas, 8 instancias skinned visibles. Frente a dos con HUD/audio/disparo: 249/96, 36 batches, 340 draws, 2 skinned. Los conteos de instancias incluyen Factory, recogibles y efectos; no representan solo personajes. Los siete TPS comparten malla/texturas. El coste CPU medio de poses fue 0,439–0,440 ms y el de skin/bounds 0,400–0,413 ms; con dos fue 0,122–0,149 y 0,106–0,115 ms. La residencia GPU del caso jugable fue 504,29 MiB dentro del límite de 512 MiB, sin evicciones acumuladas; los 39 frames limitados por presupuesto son un contador de toda la sesión, no una distribución de la medida. La memoria privada de proceso osciló entre 2199 y 2272 MiB en las pasadas de dos y entre 2149 y 2283 MiB en las de ocho; el working set rondó 1133 y 1148 MiB, respectivamente. No es una medida directa de VRAM total.

## Hound v16: transporte GPU y límite detectado

Un modo separado carga el glTF/BIN v16 cocinado y reproduce `Hound16_joint_check` en GPU en un cuerpo TPS, con brazos FPS Hound. La captura de calentamiento y otra 30 frames después muestran cambio de pose. Su último frame tuvo 11 primitivas skinned visibles, 46 batches y 341 draws; skin/bounds CPU promedió 0,982 ms, memoria de proceso 2187 MiB privada/1172 MiB working set. La residencia GPU llegó a 511,73 MiB y registró 30 evicciones durante la sesión. Este clip es diagnóstico, no una locomoción jugable ni una validación de siete Hound de producción.

El modo reproducible de **siete Hound v16 TPS + FPS** sale con código 1 antes de completar el calentamiento: `Skin constant mapping failed`. Siete cuerpos de siete primitivas multiplican las cargas de paleta de skinning en sombras y pasada opaca; el backend mapea el mismo buffer dinámico por draw. El error indica que esa ruta no admite aún la carga ensayada; no demuestra por sí solo qué asignación interna agota Diligent. Además, `skinned_bounds` transforma todos los vértices CPU de cada primitiva; los 0,982 ms medidos con un Hound son un riesgo para ocho, sin extrapolar linealmente una cifra de aceptación. Antes de presupuestar siete Hound, hay que instrumentar el buffer, reducir/reorganizar sus mapeos, medir y optimizar bounds si sigue dominando, y repetir con mapas y clips de producción. No se modificó la escultura v16 ni se inventaron límites para LOD0, materiales o texturas.

## Reproducción y validación

Desde `D:/Projects/Gloom`, compilar Release de `gloom` y `gloom_asset_cooker` y cocinar v16 con:

```powershell
rtk proxy build/windows-vs/Release/gloom_asset_cooker.exe D:/Projects/Gloom/assets D:/Projects/Gloom/build/windows-vs/content game:/characters/hound_rig/v16/hound-rig.gltf cache:/characters/hound_rig/v16/hound-rig.gasset
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-two-full-1080p
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-eight-1080p
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-eight-720p
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-hound-1080p
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-hound-eight-1080p
```

Ejecutar cada proceso **en serie**, sin otros benchmarks GPU. El último comando reproduce el fallo esperado. Capturas locales regenerables en `.cache/hito102-eight.ppm` y `.cache/hito102-hound-{a,b}.ppm`; inspeccionadas. Logs de las dos pasadas por caso, control 720p, Hound y fallo en `.cache/hito102-*.log`, ignorados por Git. Release compilado; `gloom.render_scene`, `gloom.visibility`, `gloom.vertical_slice_smoke` y `gloom.vulkan_sync` pasan 4/4. `git diff --check` sin errores.

## Estado entregado a H06

La carga jugable local de ocho y la reproducción GPU de un Hound v16 están comprobadas. **No hay presupuesto de producción aprobado**: el fotograma completo incumple 5 ms en las colas medidas; siete Hound v16 fallan al mapear constantes; el coste CPU de bounds es significativo; faltan mapas y clips jugables finales, residencia VRAM validada y combate de ocho participantes. H06 permanece bloqueada y H07 no se inicia. El trabajo siguiente concreto es aislar la regulación MAILBOX/Present y resolver el límite del buffer de skinning, después repetir y perfilar bounds con contenido Hound de producción.
