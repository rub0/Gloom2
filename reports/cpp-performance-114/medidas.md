# Medidas y reproducción del hito 114

4 de octubre de 2026. Windows x64, Ryzen 7 3700X/GTX 1070, driver 581.29, VS 2022/MSVC 14.44. 1080p nativo, Immediate, 120 frames listos de calentamiento y 360 medidos. Hound v17 LOD0, siete TPS + FPS, audio null/SDL. Series seriales sin builds o pruebas simultáneos. Base 113 `8b96c71`; después workspace 114 todavía sin commit. Los diffs registrados pueden incluir documentación en preparación; no sustituyen los hashes del ejecutable.

## Juego sin diagnóstico

### Antes

Cache: `.cache/hito114/before/measurements.json`; terminado UTC `2026-10-04T11:42:56.269607+00:00`.

Exe SHA256: `023efa580503148f9ced483fd8a777ea48137e6373ec2e35ea73c18969a78046`. Diff SHA256: `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`. Diagnóstico OFF.

| Pasada | Media ms | FPS | p50 | p95 | p99 | Máximo | >5 ms | Visibilidad CPU ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| factory-1 | 2.350 | 425.56 | 2.347 | 2.381 | 2.441 | 2.490 | 0 | 0.028 |
| factory-2 | 2.357 | 424.27 | 2.355 | 2.388 | 2.446 | 2.527 | 0 | 0.027 |
| factory-3 | 2.354 | 424.75 | 2.350 | 2.393 | 2.460 | 2.571 | 0 | 0.027 |
| hound-null-1 | 3.290 | 303.93 | 3.286 | 3.517 | 3.552 | 3.731 | 0 | 0.039 |
| hound-null-2 | 3.289 | 304.06 | 3.281 | 3.514 | 3.591 | 4.801 | 0 | 0.039 |
| hound-audio-1 | 3.287 | 304.20 | 3.282 | 3.515 | 3.555 | 3.685 | 0 | 0.031 |
| hound-audio-2 | 3.339 | 299.46 | 3.286 | 3.521 | 3.688 | 6.246 | 1 | 0.031 |

| Pasada | Etapas CPU y GPU | Residencia, uploads y memoria |
| --- | --- | --- |
| factory-1 | CPU stages ms: update=0.148 begin=0.020 presentation=0.113 visibility=0.028 lighting=0.019 draw=0.466 end_present=1.556; CPU animation ms: poses=0.077 skin_bounds=0.003; GPU full render: samples=359 mean=2.052 p50=2.053 p95=2.067 p99=2.081 max=2.087 ms | Process memory: private=1072.21 MiB working_set=571.01 MiB peak_working_set=702.47 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13 |
| factory-2 | CPU stages ms: update=0.153 begin=0.021 presentation=0.110 visibility=0.027 lighting=0.019 draw=0.462 end_present=1.565; CPU animation ms: poses=0.076 skin_bounds=0.004; GPU full render: samples=359 mean=2.060 p50=2.061 p95=2.073 p99=2.089 max=2.189 ms | Process memory: private=1087.65 MiB working_set=571.27 MiB peak_working_set=674.36 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| factory-3 | CPU stages ms: update=0.150 begin=0.019 presentation=0.114 visibility=0.027 lighting=0.019 draw=0.466 end_present=1.560; CPU animation ms: poses=0.078 skin_bounds=0.003; GPU full render: samples=359 mean=2.057 p50=2.057 p95=2.071 p99=2.084 max=2.087 ms | Process memory: private=1094.82 MiB working_set=571.43 MiB peak_working_set=670.71 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| hound-null-1 | CPU stages ms: update=0.245 begin=0.022 presentation=0.633 visibility=0.039 lighting=0.020 draw=0.741 end_present=1.590; CPU animation ms: poses=0.542 skin_bounds=0.052; GPU full render: samples=359 mean=2.993 p50=2.986 p95=3.219 p99=3.237 max=3.258 ms | Process memory: private=1771.25 MiB working_set=994.45 MiB peak_working_set=1327.10 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-null-2 | CPU stages ms: update=0.246 begin=0.022 presentation=0.627 visibility=0.039 lighting=0.019 draw=0.734 end_present=1.602; CPU animation ms: poses=0.536 skin_bounds=0.052; GPU full render: samples=359 mean=2.990 p50=2.981 p95=3.211 p99=3.225 max=3.232 ms | Process memory: private=1773.94 MiB working_set=993.68 MiB peak_working_set=1327.34 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-audio-1 | CPU stages ms: update=0.250 begin=0.021 presentation=0.598 visibility=0.031 lighting=0.018 draw=0.703 end_present=1.666; CPU animation ms: poses=0.517 skin_bounds=0.051; GPU full render: samples=359 mean=2.987 p50=2.980 p95=3.212 p99=3.222 max=3.225 ms | Process memory: private=1776.59 MiB working_set=996.70 MiB peak_working_set=1327.97 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-audio-2 | CPU stages ms: update=0.244 begin=0.021 presentation=0.606 visibility=0.031 lighting=0.018 draw=0.710 end_present=1.710; CPU animation ms: poses=0.524 skin_bounds=0.051; GPU full render: samples=359 mean=3.026 p50=2.983 p95=3.217 p99=3.232 max=3.369 ms | Process memory: private=1774.77 MiB working_set=997.25 MiB peak_working_set=1330.18 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |

### Después final

Cache: `.cache/hito114/normal-final/measurements.json`; terminado UTC `2026-10-04T13:47:45.799457+00:00`.

Exe SHA256: `8c254d1a3f25e57660055dd36d5e1c00b114ccce13814923e9cf0af6afb645f1`. Diff SHA256: `fb8f42bd1502b7c8a8e7dee13b362279968816b537d45c187dc61d81f93b708c`. Diagnóstico OFF.

| Pasada | Media ms | FPS | p50 | p95 | p99 | Máximo | >5 ms | Visibilidad CPU ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| factory-1 | 2.353 | 425.07 | 2.348 | 2.383 | 2.492 | 2.555 | 0 | 0.026 |
| factory-2 | 2.358 | 424.15 | 2.352 | 2.398 | 2.510 | 2.742 | 0 | 0.026 |
| factory-3 | 2.353 | 424.91 | 2.349 | 2.388 | 2.423 | 2.510 | 0 | 0.026 |
| hound-null-1 | 3.305 | 302.57 | 3.287 | 3.531 | 3.649 | 4.531 | 0 | 0.036 |
| hound-null-2 | 3.288 | 304.12 | 3.282 | 3.518 | 3.545 | 4.055 | 0 | 0.036 |
| hound-audio-1 | 3.360 | 297.65 | 3.293 | 3.535 | 3.788 | 7.011 | 3 | 0.032 |
| hound-audio-2 | 3.207 | 311.82 | 3.277 | 3.517 | 3.538 | 3.854 | 0 | 0.030 |

| Pasada | Etapas CPU y GPU | Residencia, uploads y memoria |
| --- | --- | --- |
| factory-1 | CPU stages ms: update=0.147 begin=0.020 presentation=0.112 visibility=0.026 lighting=0.019 draw=0.467 end_present=1.562; CPU animation ms: poses=0.077 skin_bounds=0.003; GPU full render: samples=359 mean=2.056 p50=2.055 p95=2.068 p99=2.085 max=2.205 ms | Process memory: private=1088.54 MiB working_set=571.01 MiB peak_working_set=680.10 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| factory-2 | CPU stages ms: update=0.149 begin=0.021 presentation=0.115 visibility=0.026 lighting=0.019 draw=0.474 end_present=1.554; CPU animation ms: poses=0.079 skin_bounds=0.004; GPU full render: samples=359 mean=2.057 p50=2.056 p95=2.068 p99=2.086 max=2.089 ms | Process memory: private=1089.98 MiB working_set=570.65 MiB peak_working_set=673.94 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| factory-3 | CPU stages ms: update=0.150 begin=0.020 presentation=0.111 visibility=0.026 lighting=0.019 draw=0.470 end_present=1.558; CPU animation ms: poses=0.076 skin_bounds=0.003; GPU full render: samples=359 mean=2.057 p50=2.056 p95=2.066 p99=2.084 max=2.192 ms | Process memory: private=1093.67 MiB working_set=570.96 MiB peak_working_set=677.24 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| hound-null-1 | CPU stages ms: update=0.265 begin=0.025 presentation=0.657 visibility=0.036 lighting=0.020 draw=0.767 end_present=1.536; CPU animation ms: poses=0.567 skin_bounds=0.052; GPU full render: samples=359 mean=2.985 p50=2.981 p95=3.215 p99=3.233 max=3.348 ms | Process memory: private=1775.16 MiB working_set=993.27 MiB peak_working_set=1326.68 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-null-2 | CPU stages ms: update=0.250 begin=0.022 presentation=0.633 visibility=0.036 lighting=0.020 draw=0.758 end_present=1.569; CPU animation ms: poses=0.543 skin_bounds=0.051; GPU full render: samples=359 mean=2.989 p50=2.983 p95=3.217 p99=3.236 max=3.247 ms | Process memory: private=1769.48 MiB working_set=993.54 MiB peak_working_set=1326.88 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-audio-1 | CPU stages ms: update=0.276 begin=0.023 presentation=0.637 visibility=0.032 lighting=0.019 draw=0.754 end_present=1.619; CPU animation ms: poses=0.552 skin_bounds=0.051; GPU full render: samples=359 mean=3.027 p50=2.984 p95=3.216 p99=3.226 max=3.237 ms | Process memory: private=1774.04 MiB working_set=995.52 MiB peak_working_set=1328.56 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-audio-2 | CPU stages ms: update=0.238 begin=0.023 presentation=0.586 visibility=0.030 lighting=0.018 draw=0.717 end_present=1.595; CPU animation ms: poses=0.505 skin_bounds=0.051; GPU full render: samples=359 mean=2.907 p50=2.977 p95=3.213 p99=3.231 max=3.237 ms | Process memory: private=1813.87 MiB working_set=999.71 MiB peak_working_set=1271.30 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |

Factory: mediana del frame 2,354 → 2,353 ms; visibilidad 0,027 → 0,026 ms. Hound: mediana del frame 3,2895 → 3,2965 ms; visibilidad 0,035 → 0,034 ms. Las diferencias del juego se solapan con la dispersión y la resolución de impresión (1 µs); no se atribuye una mejora global de FPS ni una regresión causal al scheduler. El cierre se apoya en la eliminación medida de asignaciones y los contratos, no en una ganancia incierta del frame.

**El presupuesto H06 no pasa en todas las pasadas de esta sesión:** antes audio-2 registra 1/360 >5 ms, máximo 6,246 ms; después audio-1 registra 3/360 >5 ms, máximo 7,011 ms. Se conservan ambos picos; no se certifica de nuevo el presupuesto de ocho ni se atribuyen a un origen sin evidencia. Los cuatro p99 de Hound están por debajo de 4 ms y residencia 345,26 MiB, sin ausencias/evicciones/frames limitados durante la medida.

Contenido en las siete pasadas antes/después: Factory 249 instancias, 96 visibles, 36 batches, 340 draws, dos skinned; Hound 288/155/51/619, 53 skinned visibles, máscaras TPS/arma 127/31, nueve mapas y 81.408 bytes copiados/frame. BC5/BC7, cero recursos ausentes y cero VUID en logs. Los frames limitados mostrados al arrancar Factory se distinguen de cero durante la medida.

## Lotes aislados sin diagnóstico

`tools/perf/measure_job_batches.py` construye el scheduler de 113 desde sus blobs Git y el actual, usa el mismo control de 1.000 grupos nuevos × tres trabajos y alterna siete pasadas por versión. Cada una completa exactamente 3.003 trabajos incluyendo calentamiento. Grupo local incluido en schedule/wait; QPC, granularidad observada 0,1 µs. Pico de cola 3 en todas. Los valores siguientes son medianas de las siete estadísticas por pasada.

| Workers | Versión | Mediana lote µs | p95 lote µs | Asistidos | Espera acumulada ms |
| ---: | --- | ---: | ---: | ---: | ---: |
| 1 | before | 0.7 | 1.6 | 2756 | 0.587 |
| 1 | after | 0.4 | 0.9 | 2874 | 0.308 |
| 4 | before | 4.7 | 9.8 | 1253 | 1.481 |
| 4 | after | 4 | 7.7 | 1307 | 2.048 |

La espera suma tiempo dentro de wait, incluida ejecución asistida: no equivale a tiempo dormido. Con cuatro workers aumenta 1,481 → 2,048 ms mientras mediana/p95 del lote bajan; no se afirma mejora en cada métrica. El primer diseño con una sola condición despertaba innecesariamente todos los workers; dos condiciones sobre el mismo lock corrigen esa dispersión sin colas múltiples ni spin.

| Versión | Workers | Serie de medianas µs | Serie de p95 µs | Asistidos | Espera acumulada ms |
| --- | ---: | --- | --- | --- | --- |
| before | 1 | 0.9, 1.4, 0.7, 0.8, 0.7, 0.6, 0.6 | 3.8, 3.9, 1.4, 1.6, 1, 3.4, 1.4 | 2809, 2756, 2694, 2463, 2470, 2886, 2827 | 0.643, 0.807, 0.53, 0.587, 0.45, 0.61, 0.476 |
| after | 1 | 0.4, 0.4, 0.4, 0.4, 0.6, 0.5, 0.4 | 2.1, 0.9, 0.9, 0.6, 2.5, 1, 0.9 | 2873, 2907, 2878, 2998, 2817, 2874, 2870 | 0.49, 0.296, 0.299, 0.281, 0.579, 0.351, 0.308 |
| before | 4 | 4.9, 4.9, 2.1, 4.7, 4.8, 4.2, 4.2 | 9.8, 10.3, 6.4, 9.5, 9.9, 10.3, 9.8 | 1253, 1315, 1520, 1215, 1116, 1093, 1288 | 1.695, 1.516, 1.183, 1.481, 1.491, 1.333, 1.389 |
| after | 4 | 4, 4.4, 3.6, 3.4, 4.2, 3.6, 4.3 | 7.5, 8.3, 6.7, 8, 7.8, 7.3, 7.7 | 1461, 1307, 1459, 1203, 1284, 1351, 1264 | 2.032, 2.302, 1.897, 2.027, 2.097, 2.048, 2.186 |

Diff SHA256 de la serie: `fb8f42bd1502b7c8a8e7dee13b362279968816b537d45c187dc61d81f93b708c`. Cache `.cache/hito114/batches-normal-final`.

Exe before: `ae31937a665f898f07a45d4845308f57d5c8ff2d9c826eafec4ac3f10d6fdb0a`.
Exe after: `51ea3ed4b87bafe47703482b71693abed48ac1799021227fb71370ea357dc03f`.

Hashes finales de fuentes del scheduler:
- `src/core/job_system.cpp`: `a390f42495524de0b3500026ab3be48f1d339b597a2601cb1be31686a87d7a3b`.
- `include/gloom/core/job_system.hpp`: `c39b2708c0bf5ea165a4f0b66a27e9209479c61f1beaea14bc68bb9be0088ec6`.
- `include/gloom/core/fixed_function.hpp`: `090cd218b1f949bf5e4c3fce3b533e17395eeb67576fbbf904c4ecb8de7b78ee`.

## Asignaciones: series separadas de los tiempos

| Control 1.000 grupos/3.000 jobs | Workers | new/new[] | Bytes solicitados |
| --- | ---: | ---: | ---: |
| before | 1 | 1000 | 200000 |
| before | 4 | 1000 | 200000 |
| after | 1 | 0 | 0 |
| after | 4 | 0 | 0 |

Control con código final: `.cache/hito114/batches-profile-final/measurements.json`. La prueba permanente gloom.jobs además combina grupo nuevo y reutilizado durante 1.000 rondas/6.000 jobs, con uno y cuatro workers: **0 llamadas y 0 bytes en todas las fases**. El pico 16 de su log corresponde a la saturación anterior de la misma instancia, no al lote de tres.

| Juego, 360 frames/pasada | Antes new/frame | Después new/frame | Antes bytes/frame | Después bytes/frame |
| --- | ---: | ---: | ---: | ---: |
| Factory, tres pasadas | 3 | 0 | 344 | 0 |
| Hound, cuatro pasadas | 4 | 0 | 416 | 0 |

Base diagnóstica 113: `.cache/hito113/profile-final/measurements.json`; después: `.cache/hito114/profile-final/measurements.json`. Este diagnóstico de juego precede al cambio de tag struct→enum/uint8 del almacenamiento y a la documentación del contrato de productores; no cambió su layout/propiedad. El código final se comprueba con el control y CTest instrumentados. No se mezclan tiempos del diagnóstico con los normales. Son ordinary new/new[] del ejecutable enlazado; no cuentan aligned new, DLL ni malloc/realloc directos; no son bytes vivos ni cero heap global. Scheduler, grupos y capturas inline no usan malloc/realloc; Array reserva antes de la ventana. Los contextos grandes se asignan explícitamente en carga, fuera del frame calentado. Los buffers reutilizados de visibilidad del 110 ya existen en ambos lados: su ahorro no se vuelve a atribuir al 114.

## Validación y límites

| Comprobación final | Resultado | Evidencia local ignorada |
| --- | --- | --- |
| Build Debug completo | Correcto | `.cache/hito114/debug-complete.log` |
| Build Release completo | Correcto | `.cache/hito114/release-final-build.log` |
| CTest Debug completo | 50/53, 257,86 s | `.cache/hito114/ctest-debug.log` |
| CTest Release completo | 49/53, 107,30 s | `.cache/hito114/ctest-release.log` |
| Formato completo | Correcto | `.cache/hito114/format-check.log` |
| Arranque parcial y recuperación | Error 1450 en segundo CreateThread; primero unido; reinicio 64/64 | `.cache/hito114/start-failure.log` |
| Captura máxima/negativos de compilación | 80/8 acepta; cinco usos no soportados rechazan | `.cache/hito114/negative/results.json` |
| Siete vistas estáticas alineadas | Comparador original pasa | `.cache/hito114/character-comparison.log` |

Jobs, storage, visibility, assets, gpu_assets, vertical_slice_smoke, vulkan_sync y hound_runtime pasan en ambos builds. La prueba siempre instrumentada cubre ejecución/destrucción exacta, 10.000 elementos, wait anidado con uno/cuatro workers, cuatro productores/4.096 índices, saturación forzada de cola 16, errores explícitos sin perder finalización, descendientes durante stop, reinicio y grupos vacíos/reutilizados. Debug comprueba asserts foreign/stale/same en procesos hijos. Assets fuerza capacidad 1, cancel/reload de preparación encolada y cien cargas con asistencia; no bloquea al publicar bajo el lock de caché.

Fallos heredados, comparados por causa con 113: animation_network solo Release termina con GameNetworkingSockets send result 25; ui_visual_review, factory_visual_review y character_visual_review fallan en ambos ante las referencias existentes. UI no usa JobSystem; Factory conserva el fallo de tile en spawn-1 y personajes diferencias de fase/calentamiento del TAA. No se borran tests ni se cambian referencias/tolerancias.

Control separado antes/después de personajes: mismas siete vistas, 32 frames listos y primera fase TAA múltiplo de ocho, con aplicación aislada de captura bajo cache. Base `.cache/hito113/character-aligned-after`; después `.cache/hito114/character-aligned`. No altera el runtime normal. Miniaturas: máximo mean 0,000416667/255, changed 0 %, worst tile 0,00444444/255. También se comparan los 1280×720 completos y se inspeccionan los pares: máximo por canal 1/255, media RGB máxima 0,000328053/255, píxeles con cualquier cambio máximo 0,098416 %. Estadísticas `.cache/hito114/character-full-stats.json`, pares `.cache/hito114/character-pairs.png`. No se repite en 114 el control animado de 1.200 frames de 113 ni se extiende este resultado a todas las vistas UI/Factory.

Fuentes propias del scheduler/prueba, loader y residencia compilan efectivamente con /GR- /EHs-c- en ambos builds. Sus formatos/resultados legacy de 117/118 mantienen STL: cierre de capturas/scheduler, no cumplimiento global. Se revisan parámetros Span/ByteSpan/GpuRange por valor, IDs pequeños, captura/lifetime, sincronización y rendimiento. Renderer, shaders, fuentes/exportaciones y referencias no figuran en el diff.

## Reproducción

Desde la raíz, en serie:

```powershell
rtk proxy cmake --preset windows -DGLOOM_ALLOCATION_PROFILE=OFF
rtk proxy cmake --build --preset windows-debug --parallel 6
rtk proxy cmake --build --preset windows-release --parallel 6
rtk proxy ctest --preset windows-debug --output-on-failure
rtk proxy ctest --preset windows-release --output-on-failure
rtk proxy cmake --build --preset windows-release --target gloom_format_check
rtk proxy python tools/perf/measure_cpp_baseline.py --output .cache/hito114/repeat-normal
rtk proxy python tools/perf/measure_job_batches.py --output .cache/hito114/repeat-batches --before 8b96c71
rtk proxy python tools/perf/measure_job_batches.py --output .cache/hito114/repeat-batches-profile --before 8b96c71 --profile
rtk proxy git diff --check
```

Para diagnóstico del juego: configurar GLOOM_ALLOCATION_PROFILE=ON, compilar gloom Release, ejecutar measure_cpp_baseline.py con --profile en otro output; restaurar OFF y recompilar después. Las cachés conservan JSON/logs, probes de fallo y capturas; no se versionan binarios, builds ni imágenes bajo reports. Los controles temporales de inicio/compilación/capturas están documentados por contrato y resultados; no añaden hooks al producto. Cierre local; 115 no iniciado. Los bloqueos Jolt/fastgltf de 117/120/128 permanecen.
