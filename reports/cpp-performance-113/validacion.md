# Validación del hito 113

4 de octubre de 2026. Presets Windows x64 y contenido del 112, sin cambiar
referencias o tolerancias. Artefactos grandes y ejecutables aislados en
`.cache/hito113`, ignorados por Git; este informe conserva el protocolo y resultados.

## Builds, suites y controles

- Builds completos Release y Debug correctos; formato y diff revisados.
- Suite Release: **49/53**, mismos fallos `gloom.animation_network` (GNS 25),
  `gloom.ui_visual_review`, `gloom.factory_visual_review`, `gloom.character_visual_review`.
- Suite Debug: **50/53**, mismos tres visuales; animation_network pasa.
- Animation_vfx, skin_bounds, character_restoration, animation_dedicated,
  material_render, visibility, vulkan_sync y hound_runtime pasan en ambas.
- `gloom.pose_storage` nuevo pasa en ambas: dos actores concurrentes × 1.000,
  cero solicitudes de new en todas las fases; control de crecimiento positivo,
  estabilización posterior × 1.000, actor/generación/vida de rig, skin → rígido y
  nuevo enlace, anterior inmutable, wrap/clamp/step/slerp y mezcla in-place.
- `gloom.skin_bounds`: Hound v16, Archangel, Shadow y Hound v17; dos actores por
  rig × 1.000, TPS/FPS, cero new en poses. Conserva 427 + 244 comparaciones de
  primitive/pose con los vértices exactos, ocho influencias, escala negativa/no
  uniforme, sumas no unitarias y vértice de peso cero. Rechaza ciclo, NaN geométrico
  y escala singular sin destruir el rig válido anterior; nueva preparación cambia
  identidad aun con igual generación.

La prueba de asignaciones tiene allocator propio, incluso con diagnóstico del
juego OFF. El crecimiento se mide fuera de la ventana estable; no se excluyen
asignaciones dentro de esa ventana para fabricar cero. Scope de animador cubre
update completo, incluida preparación de skin en la evaluación; reserva solo en bind.

Las primeras suites completas preceden al ajuste final del enlace skin → rígido
y a adelantar la validación antes de encolar GPU. Ambos cambios posteriores tienen
builds y comprobaciones focalizadas finales. No se presenta la suite histórica
como ejecutada después de cada línea del ajuste final.
Las nueve comprobaciones focalizadas finales pasan en ambas configuraciones:
50,77 s Release y 104,48 s Debug, incluidos Vulkan/Hound con el engine final.

## Referencia numérica independiente

Se congelaron fuentes include/src/apps de `268db470` con git archive. Dos procesos
separados evaluaron rigs reales; no se mezclan sus tipos/ABI en un proceso. El
loader de referencia conserva sus dependencias legacy, sin introducir un puente
STL en la ruta de poses nueva.

Archangel: 45 nodos, 43 joints, cuatro clips. Shadow: 19/17, cero clips.
Hound v16: 55/53, un clip. Doce tiempos por clip:
`-1, 0, .01, .13, .5, 1, 1.13, 1.5, 2, 4, 9, 3600`.
Además 300 updates TPS + 300 FPS por personaje, a 30 Hz, con marcha atrás,
strafe, salto, escudo, daño, disparo, cambio de arma, corte, muerte y respawn.
Se comparan worlds, skin, normales, arma, seis anclajes y cortes: **3.495.720 floats**.

| Rig | Máximo worlds procedural | Máximo skin | Máximo normales | Máximo arma/anclajes | Cortes |
| --- | ---: | ---: | ---: | ---: | --- |
| Archangel | 0,000000477 | 0,000014306 | 0,000014306 | 0,000000447 | Idénticos |
| Shadow | 0,000004083 | 0,000152588 | 0,000164032 | 0,000004113 | Idénticos |
| Hound v16 | 0,000003308 | 0,000003278 | 0,000003636 | 0,000000387 | Idénticos |

Worlds/skin/normales de muestras directas de clips: diferencia **cero**.
La descomposición procedural utiliza hypot de C con intermediarios double;
no promete los mismos redondeos del antiguo hypot STL de tres floats.
Las tolerancias existentes de bind/bounds/FPS (.001/.002 según comprobación)
se conservan. Un tanteo inicial del harness con 1e-4 resultó demasiado estricto
para normales; no era un umbral del proyecto ni se ha modificado una prueba
existente para aprobar.

SHA256 payload antes: `9e2065378cb84f4d8dbc84893313309c11c39b25e542bb1a3dd0713ffe904d27`.
Después: `ef3e2ca217bbe54eae9e3dfa11584e02ff33e28f8830d605b6c66b3787c8cde5`.
Harness/registro: `.cache/hito113/reference/capture.cpp`, `numeric-results.json`.
El arreglo final de enlaces no altera estos rigs durante la evaluación; su
comprobación específica está en pose_storage.

## Capturas y origen de la diferencia temporal

Captura animada: `--animation-review <directorio> 30`, 40 segundos simulados,
1.200 PPM 640×360. Eventos/partículas antes y después: **41 eventos, 5.410
spawned, 4.746 expired, cero dropped**. Vistas estáticas: `--character-review`.
Comparator oficial `gloom_visual_capture_tests.exe`, thumbnails 160×90, tiles
8×6, límites existentes mean ≤ 4/255, changed ≤ 1,5 %, worst_tile ≤ 12/255.

| Comparación animada | Máximo mean | Máximo changed | Máximo tile | Frames fallidos |
| --- | ---: | ---: | ---: | ---: |
| Primer antes/después, sin alinear | 4,35412 | 0,333333 % | 7,51111 | 3/1.200 |
| Antes frente a otra ejecución aislada del mismo 112 | 2,92914 | 0,229167 % | 5,79222 | 0/1.200, pero no idénticas |
| Otra captura después frente al control 112, sin alinear | 5,64583 | 0,951389 % | 9,99667 | 72/1.200 |
| Antes/después con fase y calentamiento iguales | 0,00488426 | 0 % | 0,115556 | **0/1.200** |

La diferencia extendida por el suelo no procede de huesos. `temporal_jitter`
usa `frame_index % 8`, incluyendo frames mientras se cargan assets. La preparación
y el tiempo de carga cambian ese inicio. Dos ejecuciones nuevas inicialmente
idénticas no garantizan que la fase coincida con una ejecución anterior.

Se compilaron aplicaciones de validación aisladas usando sources del 112 y del
workspace, con sus engines separados y las mismas bibliotecas externas ya
compiladas. Único ajuste de harness aplicado igual a ambos: antes del primer
frame de review, esperar al menos 32 frames de contenido cargado y comenzar
cuando `(renderer_view->frame_metrics().frame_index + 1) % 8 == 0`.
Antes comenzó en renderer frame **928**, después en **760**: misma fase cero.
No cambia dt, número de frames, snapshot, animación o técnica gráfica.
La lógica de captura estable permanece en `.cache/hito113/make_aligned_capture.py`;
no se incorpora al runtime ni reemplaza las referencias versionadas.

Cada bloque de siete frames se enlazó en un directorio temporal bajo los siete
nombres aceptados por el comparador; se escribió solo una referencia temporal del
antes y se comparó el después. Los cuatro padding del último bloque repiten el
frame 1199 y no se cuentan como frames distintos. Los resultados originales y
alineados se conservan separados, sin descartar los primeros fallos.

Siete capturas estáticas alineadas: **7/7**, máximo mean 0,000393519/255,
changed cero, tile 0,00222222/255. El ajuste final de enlace se comprueba también
con el engine final y una nueva captura alineada antes del cierre.

## Reproducción de las comprobaciones

```text
rtk proxy cmake --preset windows -DGLOOM_ALLOCATION_PROFILE=OFF
rtk proxy cmake --build --preset windows-release --parallel 4
rtk proxy cmake --build --preset windows-debug --parallel 4
rtk proxy ctest --preset windows-release
rtk proxy ctest --preset windows-debug
rtk proxy ctest --preset windows-release -R "gloom.(pose_storage|skin_bounds|animation_vfx|character_restoration|animation_dedicated|material_render|visibility|vulkan_sync|hound_runtime)$"
rtk proxy cmake --build --preset windows-release --target gloom_format_check
rtk git diff --check
```

El binario final normal se conserva con diagnóstico OFF. Auditar firmas Span
por valor, flags efectivos /GR- /EHs-c- en fuentes migradas y residuos de otros
módulos antes de interpretar este hito como cumplimiento global; el 128 sigue
siendo el cierre transversal.
