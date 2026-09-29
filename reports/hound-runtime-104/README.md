# Hito 104 — presentación y skinning de ocho combatientes

29 de septiembre de 2026 · **Ejecutado y validado. H06 sigue sin presupuesto de producción; H07 no se inicia.**

Siete Hound v16 TPS más FPS completan carga, calentamiento y medida. Cuatro
pasadas finales a 1080p, dos con audio nulo y dos con dispositivo real, dan
**281,29–283,98 FPS**, p99 máximo **3,854 ms** y máximo absoluto **3,974 ms**.
Los cuatro casos repetidos con ambas salidas suman **16 pasadas / 5.760 frames,
ninguno >5 ms**. Esto valida el escenario diagnóstico disponible, sin aprobar
contenido final inexistente. [Distribuciones completas y memoria](mediciones.md).

## Entrada, método y conservación

Base `d5eeecd`, preparación del 104, sobre `38477c3` (102) y `576eda4` (103).
Workspace limpio al comenzar. Objetivo conservado: **200 FPS / 5 ms, 1920×1080
nativo, ocho combatientes, Ryzen 7 3700X y GTX 1070**. Windows, controlador
32.0.15.8129, monitor 1920×1080 a 144 Hz. Release, ventana con drawable, render
y salida de 1920×1080, sin resolución dinámica, VSync off; modo efectivo registrado.

Cada proceso se ejecutó en serie, sin builds ni otros tests simultáneos durante
las medidas finales: carga, 120 frames de calentamiento y 360 medidos. Simulación
fija de 60 Hz; no confundirla con FPS. CPU desde poll/events hasta terminar
Present, incluidos simulación, HUD, audio y disparo. GPU desde el primer render
hasta terminar UI, excluido Present; 359 consultas recogidas con cuatro frames
de retraso, sin correspondencia exacta frame a frame con las 360 CPU.

Se conserva contenido y colocación del 102. El control antiguo de dos envía
dos skins, pero solo una pasa el frustum al final: no sirve para restar un coste
exacto por personaje. Ocho originales conservan ocho skins visibles y siete
Hound más brazos FPS v16 conservan 53 primitivas skinned en cada frame medido.
Máscara 127 confirma cambio de pose en los siete TPS. Hay oclusiones naturales
entre filas; no se omiten personajes, animación ni sombras.

Audio real confirmado: SDL3 **Headphones (High Definition Audio Device)**,
sin fallback nulo. Se repiten todos los casos dos veces con cada salida.
No se cambia autoridad/red, gameplay, escultura, mapas o clips de producción.

Los SHA-256 de fuente/glTF/BIN coinciden con el manifiesto v16:

| Archivo | SHA-256 |
| --- | --- |
| `art/characters/hound/v16/hound-mesh-v16.blend` | `e0692f36a7edd0d5546ce1f67b40857f9641d02f998fffae6de60d946c3a0911` |
| `assets/characters/hound_rig/v16/hound-rig.gltf` | `754d4b93a8141e05f8fccf67332fe1df1c4ac1ed76804547852b4c5f7668c1ab` |
| `assets/characters/hound_rig/v16/hound-rig.bin` | `d0f805dd6978bd07ff93b142780b729d2ee9a916e407ffa5f22e4712fc3eb4d7` |

## Skinning: causa compartida y vida de los datos

El ejecutable original instrumentado falla en **frame 4, mapeo 140**:
6.881.280 bytes reservados por skinning y 1.414.464 bytes de paletas contabilizados
incluyendo el intento fallido. Había **cero envíos anteriores pendientes** al
comenzar. Diligent confirma agotamiento del heap dinámico global de **8 MiB**,
incluso tras esperar 61 ms; pico global 7,94 MiB y 31 páginas en el contexto.
La espera no puede liberar datos aún grabados en ese mismo frame.

Cada draw reservaba `256 × 3 × 64 = 49.152` bytes, aunque Hound tiene 53 huesos.
Se repetía por primitiva/cascada/opacos/FPS, además de otras constantes que
comparten el heap. No era falta de memoria física ni un límite exclusivo de FPS.

La composición comparte la paleta por nodo de malla y actor. El backend cachea
la pareja **pose actual / pose anterior efectiva** durante el snapshot y la
reutiliza al enlazar sombras, opacos y FPS. DISCARD renombra cada nueva carga;
Diligent retira las asignaciones por fence. No se reescriben datos ya grabados.
Los cortes usan la pose actual como anterior; la caché no retiene punteros
después del draw. El drenaje previo a liberar recursos/dispositivo se conserva.

Resultado: **9 mapeos/frame, 442.368 bytes reservados y 81.408 copiados** con
siete Hound, incluida la reserva estática. El heap sigue en **8 MiB**. Buffers
creados al inicializar, sin nuevas asignaciones CPU por frame. Hay 64 parejas
en caché y streaming para el exceso, sin imponer otro límite de escena; ese
exceso vuelve a consumir reservas por draw y conserva el límite del heap.
El cambio no pretende resolver escenas arbitrariamente grandes.

## Present: localizar la espera antes de cambiar la política

La reproducción original de ocho dio 144,18 FPS, GPU 2,466 ms y
`end_frame/Present` 4,933 ms. Se separaron envío, `vkQueuePresentKHR`, adquisición
de imagen y espera del fence dentro del Diligent fijado por el proyecto.
No expone selección pública del modo; el pequeño parche versionado se aplica
y verifica desde CMake, también sobre dependencias ya descargadas.

Contenido idéntico, 1080p, dos imágenes; FIFO forzado solo como control:

| Contenido / modo efectivo | CPU media / FPS | CPU p95 / p99 / máximo | GPU media | QueuePresent | Acquire | Fence |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Ocho originales / MAILBOX | 6,939 / 144,12 | 7,000 / 7,065 / 7,130 | 2,260 | 0,031 | 5,396 | 0,001 |
| Ocho originales / IMMEDIATE | 2,530 / 395,30 | 2,750 / 2,763 / 2,989 | 2,239 | 0,026 | 0,002 | 1,050 |
| Ocho originales / FIFO | 6,940 / 144,09 | 7,034 / 7,154 / 7,352 | 2,270 | 0,024 | 0,003 | 5,332 |
| Siete Hound + FPS / MAILBOX | 6,937 / 144,15 | 6,983 / 7,054 / 7,167 | 3,341 | 0,032 | 5,401 | 0,001 |
| Siete Hound + FPS / IMMEDIATE, final A | 3,543 / 282,24 | 3,810 / 3,827 / 3,864 | 3,254 | 0,027 | 0,003 | 2,115 |
| Siete Hound + FPS / FIFO | 6,940 / 144,09 | 6,974 / 7,166 / 7,232 | 3,336 | 0,023 | 0,003 | 5,535 |

Tiempos en ms. MAILBOX/FIFO tienen 360/360 frames >5 ms en esos controles.
La regulación MAILBOX está en **adquirir la siguiente imagen**, no en el render
GPU ni en QueuePresent. No se atribuye a una causa interna concreta del
compositor/controlador sin una traza de sistema que la pruebe.

El ajuste permanente prioriza **IMMEDIATE con VSync desactivado**, con fallback
MAILBOX/FIFO si falta soporte. VSync activado conserva la política sincronizada.
IMMEDIATE permite tearing, acorde con VSync off. No se cambian ajustes globales
del equipo ni resolución. Los relojes de diagnóstico solo se activan en benchmark.

La cola se mide desde el envío hasta observar su fence completado al comenzar
otro frame: **cota superior CPU de finalización**, no tiempo exacto antes de
ejecutar ni latencia de pantalla. Máximo observado: un envío previo pendiente
al comenzar; swapchain de dos imágenes. [Distribuciones y límites](mediciones.md).

## Bounds: coste medido y comprobación conservadora

Tras arreglar mapeos, siete Hound en IMMEDIATE dieron media 4,698 ms, p95 5,272,
p99 6,523, máximo 7,948 y **28/360 >5 ms**. Poses: 0,145 ms; skin/bounds:
**3,203 ms**. Este perfil justificó sustituir el recorrido por vértice.

Se precalculan AABBs por hueso de cada primitiva inmutable. Cada frame transforma
centros/extensiones y obtiene una esfera que contiene su unión. Los pesos no
negativos producen una combinación convexa; los extremos de suma de pesos
cubren redondeo, sumas no unitarias y vértices sin peso. Caché por asset/generación,
64 entradas; el exceso conserva el cálculo de referencia. No se alteran poses,
mallas ni LODs. Los bounds pueden ser más amplios; no se estrechan para recortar
personajes/sombras. La implementación evita asignaciones durante el frame.

Primera comparación: skin/bounds **0,124 ms**, p99 3,849 y máximo 4,082 ms,
cero frames >5 ms. En las cuatro pasadas finales de siete Hound: skin/bounds
**0,106–0,124 ms**, poses **0,140–0,146 ms**; 53 skins visibles, 46 batches,
527 draws y 406 draws de sombras en el último frame. Capturas equivalentes
antes/después: cero píxeles distintos debajo del HUD superior, inspeccionadas
en dos poses. Las capturas anteriores de un Hound y las nuevas conservan su
apariencia; el caso original de siete fallaba antes de poder capturarse.

## Validación y reproducción

**Release 7/7:** render_scene, visibility, animation_vfx, skin_bounds,
vertical_slice_smoke, vulkan_sync y hound_runtime. **Debug 4/4:** animation_vfx,
skin_bounds, vulkan_sync y hound_runtime, con validación Vulkan sin errores.
Objetivos afectados compilados antes de probar; revisión de rendimiento,
parámetros Span por valor en el cambio y `git diff --check -- . ':!cmake/diligent-present.patch'`
comprobados. El parche conserva los espacios de contexto del formato unified diff;
su aplicación inversa se verifica con `git apply --reverse --check` sobre Diligent.

`skin_bounds` contrasta cada vértice contra el cálculo de referencia: 427
combinaciones primitiva/pose de v16, 244 del Archangel, y ocho influencias con
rotación, escala negativa/no uniforme, pesos no unitarios y cero.
`hound_runtime` cocina v16 y exige 120+360 frames, 1080p, 53 skins visibles,
animación de los siete TPS, sombras, nueve mapeos y ausencia de errores. Esa misma
regresión **falla con el ejecutable anterior por Skin constant mapping failed**
(frame 4/mapeo 140) y pasa con el corregido. No exige 200 FPS en Debug/otras GPUs.

Desde `D:/Projects/Gloom`, después de compilar Release:

```powershell
rtk proxy D:/Dev/CMake/bin/cmake.exe --build build/windows-vs --config Release --target gloom gloom_skin_bounds_tests --parallel 8
rtk proxy build/windows-vs/Release/gloom_asset_cooker.exe D:/Projects/Gloom/assets D:/Projects/Gloom/build/windows-vs/content game:/characters/hound_rig/v16/hound-rig.gltf cache:/characters/hound_rig/v16/hound-rig.gasset
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-two-full-1080p --present=immediate
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-eight-1080p --present=immediate
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-hound-1080p --present=immediate
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-hound-eight-1080p --present=immediate --audio-device
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Release -j 1 --output-on-failure -R 'gloom\.(skin_bounds|hound_runtime|vulkan_sync)$'
```

Repetir cada caso dos veces sin `--audio-device` y dos con él, siempre en serie
y sin builds simultáneos. Para los controles, cambiar a `--present=mailbox` o
`--present=fifo`. Sin override, VSync off ya usa IMMEDIATE. Se imprime el modo
efectivo. Control 720p original: 612,59 FPS, p99 1,894 ms; no sustituye 1080p.
Logs y capturas regenerables en `.cache/hito104-*`, ignorados por Git.

## Entrega a H06

El escenario diagnóstico alcanza 200 FPS por distribución y repeticiones,
**no solo por la media**. El presupuesto de producción sigue pendiente:

- V16 tiene 80.152 triángulos de autoría, siete materiales y cero imágenes
  propias. El clip diagnóstico no cubre locomoción, habilidades ni las cinco armas.
- Residencia Hound **511,73/512 MiB**: 30 evicciones en carga/calentamiento,
  cero durante las medidas. Es contabilidad de assets del motor, no VRAM total.
  No hay margen validado para mapas finales/skins distintas; memoria de proceso
  y cola están desglosadas en el anexo.
- No se amplía autoridad/red a ocho ni se sustituye una partida humana por
  este benchmark. No se inventan límites LOD/materiales/mapas a partir de la media.

Siguiente entrada, solo por nuevo encargo: conservar v16 y estas pruebas como
control, medir mapas/clips/armas de producción cuando existan y resolver
residencia antes de fijar límites definitivos H06. Su ausencia no impide dar
por comprobado este arreglo del motor. **H06 bloqueada, H07 no iniciada.**
Estado, roadmap, contrato e índice actualizados. Commit local de ejecución;
resolver con `git log -1 --oneline --grep='^hito 104:'`. Sin push ni otro hito.
