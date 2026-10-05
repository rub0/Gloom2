# Medidas y reproducción del 115

5 de octubre de 2026. Base 114 `892463aaac62fec4de40fd48669b942af657c01d`.
Ryzen 7 3700X, GTX 1070, driver 581.29, Windows x64, MSVC 14.44 Release.
Mismos assets y cooker, sin cambios de renderer, shaders, referencias ni umbrales.
Ponytail full: Array/Span existentes, simdjson instalado, sin dependencia nueva.

## Serie integrada, diagnóstico OFF

Siete pasadas antes y siete después, seriales y sin builds/pruebas simultáneos.
1920×1080 nativo, Immediate, 120 frames calientes y 360 medidos por pasada.
Tres Factory, dos Hound con audio nulo y dos con dispositivo real. V17/LOD0
diagnóstico, no integración de ocho humanos. Tiempos ms salvo FPS.

| Versión / pasada | Media | FPS | p50 | p95 | p99 | Máximo | >5 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 114 factory-1 | 2.345 | 426.48 | 2.339 | 2.377 | 2.506 | 3.413 | 0/360 |
| 114 factory-2 | 2.345 | 426.44 | 2.344 | 2.368 | 2.381 | 2.470 | 0/360 |
| 114 factory-3 | 2.346 | 426.19 | 2.345 | 2.375 | 2.404 | 2.471 | 0/360 |
| 114 hound-null-1 | 3.296 | 303.37 | 3.277 | 3.512 | 3.545 | 3.835 | 0/360 |
| 114 hound-null-2 | 3.388 | 295.19 | 3.462 | 3.743 | 3.826 | 3.832 | 0/360 |
| 114 hound-audio-1 | 3.335 | 299.84 | 3.294 | 3.744 | 3.823 | 3.845 | 0/360 |
| 114 hound-audio-2 | 3.287 | 304.26 | 3.277 | 3.519 | 3.539 | 3.553 | 0/360 |
| 115 factory-1 | 2.342 | 426.93 | 2.341 | 2.360 | 2.417 | 2.484 | 0/360 |
| 115 factory-2 | 2.346 | 426.32 | 2.345 | 2.368 | 2.393 | 2.470 | 0/360 |
| 115 factory-3 | 2.353 | 425.05 | 2.352 | 2.377 | 2.387 | 2.467 | 0/360 |
| 115 hound-null-1 | 3.283 | 304.59 | 3.275 | 3.514 | 3.532 | 3.661 | 0/360 |
| 115 hound-null-2 | 3.282 | 304.69 | 3.275 | 3.506 | 3.523 | 3.528 | 0/360 |
| 115 hound-audio-1 | 3.279 | 304.94 | 3.275 | 3.514 | 3.532 | 3.571 | 0/360 |
| 115 hound-audio-2 | 3.280 | 304.86 | 3.279 | 3.509 | 3.523 | 3.534 | 0/360 |

No se atribuye una ganancia global de FPS al 115: Factory cambia dentro del ruido;
la variación Hound no aísla partículas de GPU/presentación/carga del sistema.
Ambas series observan 0/1.440 Hound >5 ms y p99<4 ms; no sustituyen la certificación
H06 ni eliminan los picos registrados en el 114. Residency Hound 345,26 MiB,
BC5/BC7, TPS mask 127, weapon mask 31, 53 skinned visibles, peak_maps=9,
reservados 442.368 y copiados 81.408 bytes/frame; sin missing ni evicción medidos.
Factory reserva/copia 147.456/16.512 bytes, peak_maps=3. Todos los controles
integrados de la herramienta pasan. Los budget_limited_frames durante carga se
conservan separados de los medidos (cero), igual que en 114.

Etapas y memoria de proceso de todas las pasadas (ms; private/working set MiB).
Son medias de frame de la serie normal, no tiempos con el contador de asignaciones.

| Versión / pasada | update | presentation | visibility | lighting | draw | end/present | poses | bounds | GPU media | private | working set |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 114 factory-1 | 0.146 | 0.109 | 0.025 | 0.019 | 0.459 | 1.560 | 0.076 | 0.004 | 2.049 | 1091.60 | 570.06 |
| 114 factory-2 | 0.139 | 0.108 | 0.025 | 0.019 | 0.461 | 1.575 | 0.075 | 0.003 | 2.055 | 1093.77 | 570.82 |
| 114 factory-3 | 0.156 | 0.116 | 0.027 | 0.020 | 0.483 | 1.523 | 0.081 | 0.004 | 2.051 | 1092.18 | 570.98 |
| 114 hound-null-1 | 0.233 | 0.598 | 0.036 | 0.020 | 0.732 | 1.657 | 0.510 | 0.052 | 2.988 | 1775.80 | 995.21 |
| 114 hound-null-2 | 0.243 | 0.589 | 0.032 | 0.019 | 0.735 | 1.750 | 0.505 | 0.051 | 3.071 | 1773.98 | 993.84 |
| 114 hound-audio-1 | 0.229 | 0.571 | 0.029 | 0.018 | 0.705 | 1.762 | 0.491 | 0.051 | 3.027 | 1810.52 | 997.51 |
| 114 hound-audio-2 | 0.260 | 0.612 | 0.030 | 0.018 | 0.750 | 1.594 | 0.530 | 0.050 | 2.986 | 1774.53 | 994.76 |
| 115 factory-1 | 0.136 | 0.104 | 0.025 | 0.018 | 0.454 | 1.588 | 0.074 | 0.003 | 2.052 | 1089.48 | 571.59 |
| 115 factory-2 | 0.145 | 0.107 | 0.026 | 0.018 | 0.465 | 1.563 | 0.077 | 0.004 | 2.052 | 1092.52 | 570.40 |
| 115 factory-3 | 0.146 | 0.105 | 0.026 | 0.019 | 0.465 | 1.573 | 0.075 | 0.004 | 2.053 | 1093.82 | 571.27 |
| 115 hound-null-1 | 0.235 | 0.601 | 0.037 | 0.018 | 0.724 | 1.647 | 0.515 | 0.053 | 2.989 | 1773.29 | 993.82 |
| 115 hound-null-2 | 0.245 | 0.601 | 0.033 | 0.018 | 0.722 | 1.643 | 0.518 | 0.053 | 2.988 | 1773.47 | 993.82 |
| 115 hound-audio-1 | 0.226 | 0.568 | 0.029 | 0.017 | 0.690 | 1.731 | 0.491 | 0.051 | 2.985 | 1773.47 | 995.88 |
| 115 hound-audio-2 | 0.227 | 0.564 | 0.029 | 0.018 | 0.676 | 1.749 | 0.487 | 0.051 | 2.985 | 1775.59 | 995.00 |

## Control CPU del módulo, diagnóstico OFF

Mismo driver de prueba compila el código del 114 y el actual, mismas 17 recetas,
semillas y materiales. Cinco repeticiones alternadas antes/después; 1.000 pasos
update/render por repetición a capacidades 1, 5 y 2.048, después de reserva.
El vector antiguo se destruye después de tomar el tiempo; el actual incluye
vaciar la salida al empezar. La comparación es conservadora para el 115.
Resolución de impresión 0,1 µs. En cada celda: mediana / p95, en µs.

| Pasada | Cap 1 antes | Cap 1 después | Cap 5 antes | Cap 5 después | Cap 2.048 antes | Cap 2.048 después | Efectos antes | Efectos después |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 0.2 / 0.2 | 0.1 / 0.1 | 0.3 / 0.3 | 0.2 / 0.2 | 48.8 / 184.6 | 37.2 / 37.9 | 8.5 / 28.1 | 6.5 / 6.6 |
| 2 | 0.1 / 0.2 | 0.1 / 0.1 | 0.3 / 0.4 | 0.2 / 0.2 | 53.1 / 54.9 | 39.8 / 41.5 | 9.1 / 9.3 | 7.1 / 7.2 |
| 3 | 0.1 / 0.2 | 0.1 / 0.1 | 0.3 / 0.3 | 0.2 / 0.2 | 48.3 / 50 | 40.1 / 41.9 | 8.3 / 9.1 | 7 / 7.1 |
| 4 | 0.2 / 0.2 | 0.1 / 0.1 | 0.3 / 0.3 | 0.2 / 0.2 | 48.7 / 65.4 | 36.8 / 37.8 | 12.9 / 36 | 6.5 / 6.6 |
| 5 | 0.1 / 0.2 | 0.1 / 0.1 | 0.3 / 0.3 | 0.2 / 0.2 | 52.8 / 54.5 | 37.2 / 38.1 | 9.1 / 9.3 | 6.5 / 6.7 |

Cap 2.048: mediana de las medianas 48,8 → 37,2 µs (-23,8 %); mediana de p95
54,9 → 38,1 µs. Ruido relativo `(max-min)/mediana`: 9,8 % antes y 8,9 % después;
la mejora supera ambas dispersiones. La carga mantiene 2.048 spawned, 0 expired
y 24 dropped de los bursts de llenado, idénticos. Capacidades 1/5: 1/5 spawned,
27/23 dropped, cero expired; sus diferencias están cerca de resolución/ruido.
No se extrapolan a FPS ni se elimina el p95 de 184,6 de la primera pasada antigua.

Efectos: 1.000 eventos confirmados con refresh, update y render; mismo contador
1.000 events, 3.036 spawned, 2.718 expired, cero dropped en todas las pasadas.
Medianas agregadas 9,1 → 6,5 µs, p95 9,3 → 6,7 µs. Dispersión antigua 50,5 %
(incluye la pasada 12,9); tras cinco repeticiones no se atribuye una ganancia
causal porcentual a efectos ni al juego. El ahorro de asignaciones es inequívoco.

La primera versión nativa regresó en estrés porque Array::resize(0) reescribía
475.136 bytes cada frame. Se corrigió la limpieza de valores con destructor trivial;
propietarios no triviales siguen liberando al reducirse y regrowth restaura
defaults, comprobado por gloom.storage. Sin arena, SIMD o cambio GPU.

## Asignaciones, diagnóstico ON separado

Contador del ejecutable para ordinary new/new[] en todos los threads enlazados.
Excluye aligned new, asignaciones de DLL y llamadas C directas. Suma bytes
solicitados; no mide heap global, memoria viva ni pico de RSS. La ruta propia
calentada solo usa Array con new[]; no tiene malloc/realloc por frame. Lectura
C/simdjson, reservas y carga externa quedan fuera de estas ventanas.
El estrés antes usa el módulo 114 recompilado hoy; la referencia integrada de
asignaciones es la serie diagnóstica guardada del 114 (4 de octubre), no su CPU.

| Ventana | 114 llamadas / bytes | 115 llamadas / bytes |
| --- | ---: | ---: |
| 1.000 renders, cap 1 | 1.000 / 232.000 | 0 / 0 |
| 1.000 renders, cap 5 | 1.000 / 1.160.000 | 0 / 0 |
| 1.000 renders, cap 2.048 | 1.000 / 475.175.000 | 0 / 0 |
| 1.000 eventos+update/render | 1.000 / 69.003.125 | 0 / 0 |
| Factory, cada pasada de 360 frames | 360 / 8.269.992 | 0 / 0 |
| Hound, cada pasada de 360 frames | 360 / 6.967.080 | 0 / 0 |

Los 39 bytes de overhead por asignación grande del vector antiguo están incluidos
en los 475.175.000. Las siete pasadas actuales mantienen además poses, bounds y
jobs a cero llamadas/bytes, como en 113/114. Renderer y other aún asignan; no se
declara cero heap del juego. Partículas: peak_count 108 Factory/93 Hound,
capacidad 2.048, sizeof 56. Emisores peak_count 20/18, cap 64, sizeof 80;
complete_instances cap 2.578, sizeof 232, sin crecimiento en el intervalo.

Prueba permanente gloom.particle_storage: 1.000 update/render a capacidad 2.048
y 1.000 eventos separados suman cero llamadas/bytes en todas las fases. Conserva
puntero y prefijo de salida. Control positivo: salida sin reserva sí asigna y
el contador lo detecta. En Release y Debug pasa con el diagnóstico de la app OFF;
la prueba enlaza su contador independiente. No se mantiene un framework nuevo.

## Equivalencia y capturas

Control numérico con 114: 49.080 registros, 583.108 valores comparados,
delta máximo absoluto 4,800000001914384e-7 (<1e-5 del control). Identidades,
orden, flags, eventos y métricas exactos. Incluye 30/60/144 Hz × capacidades
1/5/2.048, emisores con empate temporal, bursts, refresh ausente, movimiento,
translate/cancel/clear y 300 ticks de combate con replay/cut/cambio de vida.
La diferencia pequeña procede de la longitud con intermediarios double;
no se cambian semillas, generador, interpolación o material.

Captura normal antes/después: 1.200 frames a 30 Hz; falla en grupos iniciales
por fase de TAA/historia de carga. Se conserva esa evidencia, no se modifican
las referencias del proyecto. Como en 113/114, se usa un ejecutable de revisión
con calentamiento de 32 frames listos y primer índice divisible por 8. El binario
histórico del 114 se conserva y su main se contrasta contra Git: solo ese ajuste
de revisión. El 115 aplica exactamente la misma alineación en copia de caché.

**Las 1.200 capturas alineadas pasan** el comparador C++ original de miniaturas
160×90, mean≤4/255, changed≤1,5 % (>32/255), worst_tile≤12/255. La salida completa
es 640×360, resolución propia del modo animation-review. Todos sus píxeles se
comparan también: máximo por canal 1/255, máximo de medias RGB 0,00160735/255,
máximo 0,479601 % píxeles con cualquier cambio. Trece pares revisados visualmente,
incluidos muzzle, trails, sangre, explosión, muerte/respawn y ambos personajes.
41 eventos, spawned=5.410, expired=4.746, dropped=0 en ambos. Sin regresión nueva
detectada en esta ruta; pasarla no declara arreglados los comparadores heredados.

## Builds, pruebas y fronteras

Builds completos Release/Debug correctos; gloom_format_check correcto. Cambio
de Array/públicos recompila consumidores; sin ajustes de recursos/shaders.
Release 50/54, 110,50 s. Debug 51/54, 256,63 s. Pasan storage, particle_storage,
animation_vfx, combat, material_render, vertical_slice_smoke, vulkan_sync,
hound_runtime y los demás tests salvo los fallos heredados:

- ui_visual_review: fase/historia visual de la referencia previa.
- factory_visual_review: spawn-1 worst tile ~31,3/255 (>12), misma causa previa.
- character_visual_review: fase de animación/TAA, misma causa previa.
- animation_network solo Release: GNS send result 25, como 114.

No se desactivan pruebas, cambian umbrales ni actualizan referencias. La prueba
nativa añade 0/1/2.048/exceso, cámara paralela, vida, missing material/refresh,
cancel/clear/translate, 65 emisores, reutilización ID con nueva vida tras cancel,
archivo/JSON/esquema/versión/receta inválidos, duplicados y máximo 64 recetas.
Debug ejecuta en hijos los asserts por dt NaN, identidad cambiada y solapamiento
de materiales con salida; no usa excepciones ni abort manual.

/GR- /EHs-c- verificados en CL.command.1.tlog Debug/Release para particles.cpp,
combat_effects.cpp y particle_storage_tests.cpp. Sin std::, auto ordinario,
lambdas, shared owners, excepciones o RTTI directos en los cuatro archivos
del módulo y su prueba. Span/ByteSpan/GpuRange por valor en todos los cambios.
/wd4530 limitado a esos consumidores por headers legacy Scene/animación/simdjson;
no acredita cumplimiento global. Scene/RenderInstance siguen bajo 119, assets
bajo 117 y bloques mixtos de tests bajo 117/122. Los conflictos fastgltf/Jolt
117/120/128 siguen sin excepción autorizada. 198 archivos propios /54 CTest.

## Identificación y reproducción

Logs/JSON/ejecutables/capturas completos en `.cache/hito115`, no versionados.
Solo este informe Markdown se versiona; la prueba permanente fija los contratos.
Series normales before-normal/after-normal, after-profile separado; control/
measurements.json contiene las cinco pasadas y registros numéricos. Control
temporal `.cache/hito115/control.cpp` / measure_control.py compara Git 114 con
el módulo actual; sus números/tablas arriba quedan registrados sin depender
de la conservación de esa caché. Control positivo/equivalencia básica se
reproducen desde particle_storage y animation_vfx versionados.

```powershell
rtk proxy cmake --preset windows -DGLOOM_ALLOCATION_PROFILE=OFF
rtk proxy cmake --build --preset windows-release --parallel 6
rtk proxy cmake --build --preset windows-debug --parallel 6
rtk proxy cmake --build --preset windows-release --target gloom_format_check
rtk proxy ctest --preset windows-release --output-on-failure
rtk proxy ctest --preset windows-debug --output-on-failure
rtk proxy build/windows-vs/Release/gloom_particle_storage_tests.exe
rtk proxy build/windows-vs/Debug/gloom_particle_storage_tests.exe
rtk proxy python -X utf8 tools/perf/measure_cpp_baseline.py --output .cache/hito115/repeat-normal
rtk proxy cmake --preset windows -DGLOOM_ALLOCATION_PROFILE=ON
rtk proxy cmake --build --preset windows-release --target gloom
rtk proxy python -X utf8 tools/perf/measure_cpp_baseline.py --profile --output .cache/hito115/repeat-profile
rtk proxy cmake --preset windows -DGLOOM_ALLOCATION_PROFILE=OFF
rtk proxy cmake --build --preset windows-release --target gloom
```

Ejecutar serialmente; no comparar los tiempos perfilados con la serie normal.
El diagnóstico de runtime se restaura OFF y el juego se recompila así al terminar.
El 116 no se inicia; cierre con commit local sin push, resolver mediante
`rtk git log -1 --oneline --grep='^hito 115:'`.

SHA-256 normales y fuentes medidas (comprobadas otra vez antes del cierre):

| Elemento | SHA-256 |
| --- | --- |
| gloom.exe 114 normal | `8c254d1a3f25e57660055dd36d5e1c00b114ccce13814923e9cf0af6afb645f1` |
| gloom.exe 115 normal | `70a2960d220d18537f36d18fe7d596d4aa1369ed1abea8d71e058a4377669d4c` |
| Diff 114 al medir | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` (vacío) |
| Diff 115 al medir | `bde17170d185dae2f207f5958e192903267347f2104d6627a722d8ed087f685c` |
| include/gloom/core/array.hpp | `4234f3b527109de4ebd8e8aff8ac82c6b60b88192f18c077d90cbfda13d84e64` |
| include/gloom/render/particles.hpp | `989d68ed22c26d09d64e3315e22a4c7cd56e758ccf23ea6f5d5277a11f80c09f` |
| src/render/particles.cpp | `a100fcf70273ab5b7e774945c7e0f8de6340250ea269feedfad0ec74360de439` |
| include/gloom/gameplay/combat_effects.hpp | `35d52434a8364949c75644b35119fafb130ffdf0f0d540d771a61bbb30c3fff5` |
| src/gameplay/combat_effects.cpp | `49483938da67beaaced44ad53ccf93ba93512e186b07645010a260b9909ab3c2` |
