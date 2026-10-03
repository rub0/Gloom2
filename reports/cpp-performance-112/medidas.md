# Medidas completas y reproducción

Fecha: 3 de octubre de 2026. Windows x64, Ryzen 7 3700X, GTX 1070, driver 581.29.
MSVC/Visual Studio 2022 y presets windows-release/windows-debug. Mismo Diligent/Vulkan
y contenido; ventana nativa 1920×1080, `--present=immediate`, modo efectivo 0,
dos imágenes de swapchain. Factory: 120 frames listos de calentamiento y 360 medidos.
Hound: probe H06/H07 v17 LOD0, siete TPS y FPS con HUD/fuego, audio null/SDL.
No certifica ocho jugadores humanos ni partida autoritativa completa.

Series seriales sin builds/pruebas simultáneos. `baseline`: binario del HEAD
517b16c antes de instrumentar. Las otras tres series son builds del workspace del
112 **sin commit todavía**, no binarios limpios del 111. SHA256 identifica el exe;
source_diff_sha256, donde existe, identifica el diff de archivos ya versionados,
no contiene las fuentes nuevas sin seguimiento. Estas se conservan en el commit
112. Los logs/JSON quedan en .cache/hito112, excluidos de Git; las tablas conservan
las medidas y hashes. No se descarta ninguna pasada.

## Identidad de cada serie

| Serie | Diagnóstico | HEAD de entrada | SHA256 del exe |
| --- | --- | --- | --- |
| baseline | False | 517b16cba8d0e3d312b422f292bec4af9e3b7f5d | `a0792eba982292e5305d4195d234e64be79dfa33489e10dfd9c10b43dd4e9700` |
| instrumented | True | 517b16cba8d0e3d312b422f292bec4af9e3b7f5d | `b38f30ea0088998f45bc0b064308f59094b622269ee44319719bb069b6e8695a` |
| instrumented-final | True | 517b16cba8d0e3d312b422f292bec4af9e3b7f5d | `ad8f72364e3feb71bb7570c4379da6f16130ebf5b21df40a3ef4b47d290671c8` |
| normal-final | False | 517b16cba8d0e3d312b422f292bec4af9e3b7f5d | `e70fa7235b6c1c4abebe8ec695baa6983ca16ea2cc33fe2740431a59c0b9e0c3` |

## Distribuciones de frame

Milisegundos; cada fila contiene 360 muestras. >5 ms es el conteo real del log.

| Serie / pasada | Media | FPS | p50 | p95 | p99 | Máximo | >5 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline/factory-1 | 2.366 | 422.71 | 2.346 | 2.461 | 2.555 | 2.577 | 0 |
| baseline/factory-2 | 2.549 | 392.31 | 2.458 | 3.099 | 3.186 | 3.397 | 0 |
| baseline/factory-3 | 2.655 | 376.63 | 2.476 | 3.080 | 9.286 | 9.386 | 9 |
| baseline/hound-null-1 | 3.653 | 273.73 | 3.518 | 4.003 | 8.723 | 10.229 | 9 |
| baseline/hound-null-2 | 3.346 | 298.82 | 3.366 | 3.672 | 3.722 | 3.815 | 0 |
| baseline/hound-audio-1 | 3.799 | 263.24 | 3.664 | 4.348 | 9.399 | 9.738 | 11 |
| baseline/hound-audio-2 | 3.417 | 292.64 | 3.431 | 3.824 | 4.008 | 4.211 | 0 |
| instrumented/factory-1 | 2.444 | 409.25 | 2.376 | 3.045 | 3.061 | 3.109 | 0 |
| instrumented/factory-2 | 2.549 | 392.29 | 2.467 | 3.016 | 3.117 | 4.219 | 0 |
| instrumented/factory-3 | 2.592 | 385.74 | 2.493 | 3.103 | 3.177 | 3.339 | 0 |
| instrumented/hound-null-1 | 3.314 | 301.71 | 3.324 | 3.736 | 3.840 | 3.874 | 0 |
| instrumented/hound-null-2 | 3.395 | 294.53 | 3.428 | 3.762 | 3.873 | 3.995 | 0 |
| instrumented/hound-audio-1 | 3.550 | 281.73 | 3.579 | 4.056 | 4.227 | 4.410 | 0 |
| instrumented/hound-audio-2 | 3.287 | 304.20 | 3.280 | 3.512 | 3.639 | 3.723 | 0 |
| instrumented-final/factory-1 | 2.400 | 416.61 | 2.354 | 2.548 | 2.648 | 2.720 | 0 |
| instrumented-final/factory-2 | 2.598 | 384.97 | 2.407 | 3.071 | 6.989 | 9.451 | 9 |
| instrumented-final/factory-3 | 2.404 | 415.96 | 2.358 | 2.549 | 2.643 | 2.713 | 0 |
| instrumented-final/hound-null-1 | 3.382 | 295.69 | 3.442 | 3.752 | 3.792 | 3.808 | 0 |
| instrumented-final/hound-null-2 | 3.382 | 295.70 | 3.452 | 3.755 | 3.800 | 3.910 | 0 |
| instrumented-final/hound-audio-1 | 3.385 | 295.44 | 3.450 | 3.763 | 3.798 | 3.964 | 0 |
| instrumented-final/hound-audio-2 | 3.383 | 295.58 | 3.442 | 3.755 | 3.814 | 3.987 | 0 |
| normal-final/factory-1 | 2.404 | 416.02 | 2.363 | 2.536 | 2.632 | 2.635 | 0 |
| normal-final/factory-2 | 2.407 | 415.53 | 2.365 | 2.534 | 2.604 | 2.639 | 0 |
| normal-final/factory-3 | 2.417 | 413.74 | 2.373 | 2.582 | 2.648 | 2.829 | 0 |
| normal-final/hound-null-1 | 3.390 | 295.02 | 3.458 | 3.739 | 3.813 | 3.973 | 0 |
| normal-final/hound-null-2 | 3.556 | 281.22 | 3.477 | 3.867 | 7.203 | 9.485 | 11 |
| normal-final/hound-audio-1 | 3.391 | 294.91 | 3.454 | 3.776 | 3.822 | 3.836 | 0 |
| normal-final/hound-audio-2 | 3.377 | 296.12 | 3.443 | 3.769 | 3.801 | 3.918 | 0 |

## Etapas CPU y GPU por pasada

Medias de etapa en ms. GPU tiene 359 muestras por latencia de lectura de queries; CPU tiene 360.

| Serie / pasada | Update | Begin | Presentation | Visibilidad | Luz | Draw | End/Present | Poses | Bounds | GPU media/p50/p95/p99/máx |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| baseline/factory-1 | 0.140 | 0.018 | 0.171 | 0.026 | 0.019 | 0.452 | 1.541 | 0.114 | 0.018 | 2.078 / 2.045 / 2.169 / 2.180 / 2.187 |
| baseline/factory-2 | 0.170 | 0.024 | 0.194 | 0.028 | 0.019 | 0.484 | 1.629 | 0.133 | 0.018 | 2.131 / 2.165 / 2.261 / 2.413 / 2.800 |
| baseline/factory-3 | 0.203 | 0.031 | 0.194 | 0.031 | 0.020 | 0.526 | 1.650 | 0.130 | 0.016 | 2.133 / 2.149 / 2.278 / 2.430 / 2.565 |
| baseline/hound-null-1 | 0.308 | 0.030 | 0.743 | 0.040 | 0.019 | 0.804 | 1.709 | 0.585 | 0.097 | 3.161 / 3.152 / 3.516 / 3.674 / 3.937 |
| baseline/hound-null-2 | 0.237 | 0.021 | 0.705 | 0.039 | 0.019 | 0.721 | 1.604 | 0.529 | 0.118 | 3.043 / 2.987 / 3.344 / 3.460 / 3.470 |
| baseline/hound-audio-1 | 0.280 | 0.028 | 0.723 | 0.033 | 0.018 | 0.778 | 1.938 | 0.580 | 0.092 | 3.198 / 3.187 / 3.579 / 3.708 / 4.072 |
| baseline/hound-audio-2 | 0.240 | 0.022 | 0.669 | 0.031 | 0.017 | 0.719 | 1.718 | 0.531 | 0.093 | 3.073 / 2.999 / 3.436 / 3.542 / 3.567 |
| instrumented/factory-1 | 0.159 | 0.022 | 0.184 | 0.029 | 0.019 | 0.472 | 1.559 | 0.124 | 0.018 | 2.107 / 2.057 / 2.220 / 2.258 / 2.402 |
| instrumented/factory-2 | 0.169 | 0.025 | 0.193 | 0.031 | 0.020 | 0.502 | 1.609 | 0.128 | 0.018 | 2.147 / 2.164 / 2.279 / 2.413 / 2.458 |
| instrumented/factory-3 | 0.169 | 0.025 | 0.201 | 0.031 | 0.019 | 0.501 | 1.646 | 0.137 | 0.017 | 2.157 / 2.175 / 2.337 / 2.408 / 2.523 |
| instrumented/hound-null-1 | 0.247 | 0.022 | 0.722 | 0.041 | 0.020 | 0.751 | 1.510 | 0.545 | 0.116 | 2.992 / 2.983 / 3.372 / 3.502 / 3.588 |
| instrumented/hound-null-2 | 0.250 | 0.022 | 0.713 | 0.037 | 0.019 | 0.739 | 1.617 | 0.549 | 0.108 | 3.080 / 2.994 / 3.475 / 3.580 / 3.606 |
| instrumented/hound-audio-1 | 0.278 | 0.027 | 0.715 | 0.034 | 0.019 | 0.786 | 1.691 | 0.575 | 0.090 | 3.142 / 3.199 / 3.577 / 3.719 / 3.953 |
| instrumented/hound-audio-2 | 0.259 | 0.020 | 0.686 | 0.032 | 0.018 | 0.709 | 1.565 | 0.540 | 0.098 | 2.987 / 2.982 / 3.210 / 3.220 / 3.332 |
| instrumented-final/factory-1 | 0.147 | 0.018 | 0.180 | 0.028 | 0.019 | 0.457 | 1.552 | 0.118 | 0.020 | 2.106 / 2.051 / 2.248 / 2.258 / 2.269 |
| instrumented-final/factory-2 | 0.159 | 0.021 | 0.185 | 0.028 | 0.019 | 0.473 | 1.713 | 0.122 | 0.019 | 2.124 / 2.056 / 2.332 / 2.389 / 2.599 |
| instrumented-final/factory-3 | 0.149 | 0.019 | 0.184 | 0.028 | 0.019 | 0.464 | 1.541 | 0.121 | 0.020 | 2.110 / 2.054 / 2.252 / 2.264 / 2.267 |
| instrumented-final/hound-null-1 | 0.245 | 0.020 | 0.717 | 0.041 | 0.019 | 0.730 | 1.610 | 0.539 | 0.118 | 3.070 / 2.986 / 3.501 / 3.524 / 3.546 |
| instrumented-final/hound-null-2 | 0.252 | 0.021 | 0.797 | 0.034 | 0.018 | 0.719 | 1.542 | 0.639 | 0.103 | 3.079 / 2.988 / 3.501 / 3.533 / 3.543 |
| instrumented-final/hound-audio-1 | 0.232 | 0.019 | 0.669 | 0.031 | 0.017 | 0.690 | 1.727 | 0.527 | 0.095 | 3.071 / 2.986 / 3.501 / 3.527 / 3.534 |
| instrumented-final/hound-audio-2 | 0.241 | 0.020 | 0.676 | 0.032 | 0.017 | 0.699 | 1.697 | 0.534 | 0.095 | 3.082 / 2.991 / 3.505 / 3.536 / 3.548 |
| normal-final/factory-1 | 0.147 | 0.018 | 0.184 | 0.026 | 0.019 | 0.456 | 1.553 | 0.125 | 0.019 | 2.111 / 2.054 / 2.254 / 2.265 / 2.271 |
| normal-final/factory-2 | 0.145 | 0.019 | 0.172 | 0.027 | 0.019 | 0.461 | 1.565 | 0.115 | 0.018 | 2.117 / 2.058 / 2.258 / 2.269 / 2.281 |
| normal-final/factory-3 | 0.142 | 0.018 | 0.176 | 0.027 | 0.019 | 0.453 | 1.582 | 0.115 | 0.020 | 2.120 / 2.064 / 2.262 / 2.275 / 2.279 |
| normal-final/hound-null-1 | 0.243 | 0.022 | 0.724 | 0.039 | 0.019 | 0.736 | 1.607 | 0.546 | 0.118 | 3.076 / 3.002 / 3.434 / 3.542 / 3.559 |
| normal-final/hound-null-2 | 0.261 | 0.022 | 0.805 | 0.040 | 0.019 | 0.748 | 1.660 | 0.631 | 0.113 | 3.103 / 3.012 / 3.506 / 3.550 / 3.675 |
| normal-final/hound-audio-1 | 0.226 | 0.019 | 0.658 | 0.030 | 0.017 | 0.686 | 1.754 | 0.521 | 0.094 | 3.084 / 2.988 / 3.506 / 3.540 / 3.554 |
| normal-final/hound-audio-2 | 0.225 | 0.019 | 0.655 | 0.030 | 0.017 | 0.690 | 1.740 | 0.517 | 0.094 | 3.075 / 2.983 / 3.498 / 3.532 / 3.541 |

## Memoria y contenido conservado

Private bytes, working set y pico WS pertenecen al proceso entero; no son bytes vivos de los contadores new. Residency es el contador de assets de Gloom, no todo el heap Vulkan.

| Serie / pasada | Private MiB | WS MiB | Pico WS MiB | Residency MiB | Pico cola jobs |
| --- | ---: | ---: | ---: | ---: | ---: |
| baseline/factory-1 | 1088.18 | 569.41 | 679.66 | 130.24 | 103 |
| baseline/factory-2 | 1092.27 | 568.96 | 671.55 | 130.24 | 102 |
| baseline/factory-3 | 1076.41 | 569.93 | 699.57 | 130.24 | 103 |
| baseline/hound-null-1 | 1770.29 | 989.60 | 1322.90 | 345.26 | 127 |
| baseline/hound-null-2 | 1785.71 | 989.66 | 1324.72 | 345.26 | 126 |
| baseline/hound-audio-1 | 1770.33 | 994.05 | 1326.77 | 345.26 | 126 |
| baseline/hound-audio-2 | 1805.51 | 1014.92 | 1325.55 | 345.26 | 125 |
| instrumented/factory-1 | 1092.50 | 569.55 | 677.16 | 130.24 | 103 |
| instrumented/factory-2 | 1075.53 | 569.01 | 699.66 | 130.24 | 102 |
| instrumented/factory-3 | 1092.40 | 568.95 | 674.53 | 130.24 | 103 |
| instrumented/hound-null-1 | 1808.61 | 994.56 | 1271.71 | 345.26 | 126 |
| instrumented/hound-null-2 | 1772.25 | 991.38 | 1324.03 | 345.26 | 124 |
| instrumented/hound-audio-1 | 1782.51 | 990.43 | 1324.65 | 345.26 | 126 |
| instrumented/hound-audio-2 | 1771.55 | 994.17 | 1326.22 | 345.26 | 125 |
| instrumented-final/factory-1 | 1077.46 | 570.60 | 704.77 | 130.24 | 102 |
| instrumented-final/factory-2 | 1077.41 | 570.46 | 706.79 | 130.24 | 102 |
| instrumented-final/factory-3 | 1078.11 | 571.70 | 700.82 | 130.24 | 103 |
| instrumented-final/hound-null-1 | 1775.92 | 993.21 | 1326.83 | 345.26 | 126 |
| instrumented-final/hound-null-2 | 1771.38 | 990.89 | 1324.30 | 345.26 | 125 |
| instrumented-final/hound-audio-1 | 1785.48 | 993.09 | 1325.93 | 345.26 | 125 |
| instrumented-final/hound-audio-2 | 1784.79 | 991.62 | 1325.71 | 345.26 | 125 |
| normal-final/factory-1 | 1091.75 | 569.52 | 675.92 | 130.24 | 102 |
| normal-final/factory-2 | 1091.53 | 569.83 | 671.93 | 130.24 | 102 |
| normal-final/factory-3 | 1092.63 | 570.61 | 672.89 | 130.24 | 104 |
| normal-final/hound-null-1 | 1783.64 | 990.85 | 1324.55 | 345.26 | 125 |
| normal-final/hound-null-2 | 1772.34 | 990.48 | 1324.17 | 345.26 | 125 |
| normal-final/hound-audio-1 | 1772.68 | 993.07 | 1326.12 | 345.26 | 125 |
| normal-final/hound-audio-2 | 1789.75 | 993.79 | 1326.08 | 345.26 | 126 |

Todas las 28 pasadas terminan correctamente sin VUID, errores Diligent ni mapping
failed. Factory conserva 249 instances/96 visibles/36 batches/340 draws, dos
skinned (cero visibles). Hound conserva 288 instances/155 visibles/51 batches/619
draws, **53 skinned y 53 visibles**, TPS mask 127/weapon mask 31, BC5/BC7 y 345,26 MiB
residency. Las máscaras, conteos y ausencia de recursos se comprueban en el script.
Evictions y budget_limited_frames **medidos** son cero; el total desde arranque
contiene frames limitados durante carga (Factory 12–13, Hound alrededor de 30),
y no debe confundirse con el delta del intervalo calentado. Cero missing meshes/textures
medidos en todas las pasadas. Hound: nueve mapas de skin por frame, peak_maps=9,
442.368 bytes reservados/81.408 copiados y 464 shadow draws. No se reducen estos trabajos.

El contrato H06 sigue p99≤4 ms/máximo≤5 ms/0 frames >5 ms. **La serie baseline ya
contiene incumplimientos**, incluso antes de los cambios del 112. Las tablas
anteriores conservan todos los picos. Las pasadas posteriores que pasan no
certifican estabilidad ni convierten esa variación en una optimización. No se
han relajado límites, ni se ha cambiado resolución, LOD, audio o contenido.

## Comparación normal antes/después

Mediana de las tres Factory normales. Ruido = mayor rango relativo de cada serie.

| Métrica ms | Entrada mediana | Final mediana | Cambio relativo | Ruido relativo |
| --- | ---: | ---: | ---: | ---: |
| mean | 2.549 | 2.407 | -5.57% | 11.34% |
| update | 0.170 | 0.145 | -14.71% | 37.06% |
| presentation | 0.194 | 0.176 | -9.28% | 11.86% |
| visibility | 0.028 | 0.027 | -3.57% | 17.86% |
| lighting | 0.019 | 0.019 | +0.00% | 5.26% |
| draw | 0.484 | 0.456 | -5.79% | 15.29% |
| end_present | 1.629 | 1.565 | -3.93% | 6.69% |

No se atribuye ganancia de FPS al 112. Instrumented usa interceptación, atomics,
QPC y metadata locks: sus tiempos son diagnóstico y nunca sustituyen la serie
normal. No hay un cambio de algoritmo para aceptar por rendimiento en este hito.
Las diferencias cercanas a resolución/ruido no justifican optimizaciones nuevas;
los próximos hitos harán antes/después con su propio intervalo y repeticiones.

## Comandos reproducibles

Preparación Hound ya existente usada por estas series, sin regenerar fuentes:

```powershell
rtk proxy python tools/art/prepare_hound_budget.py --v17 --size 2048
rtk proxy cmake --preset windows -DGLOOM_ALLOCATION_PROFILE=ON
rtk proxy cmake --build --preset windows-release
rtk proxy ctest --preset windows-release -R "gloom\.(allocation_profile|dependency_contract|storage|jobs|visibility|lighting)$" --output-on-failure
rtk proxy build/windows-vs/Release/gloom_job_tests.exe
rtk proxy python tools/perf/measure_cpp_baseline.py --profile --output .cache/hito112/repeat-profile
rtk proxy cmake --preset windows -DGLOOM_ALLOCATION_PROFILE=OFF
rtk proxy cmake --build --preset windows-release
rtk proxy cmake --build --preset windows-debug
rtk proxy python tools/perf/measure_cpp_baseline.py --output .cache/hito112/repeat-normal
rtk proxy ctest --preset windows-release --output-on-failure
rtk proxy ctest --preset windows-debug --output-on-failure
rtk proxy cmake --build --preset windows-release --target gloom_format_check
```

El script nuevo lanza tres Factory, dos Hound null y dos Hound SDL **serialmente**,
siempre con --present=immediate. Los flags Hound son
`--vertical-slice-performance-hound-eight-1080p --hound-production-v17 --hound-budget-lod=0`,
y añade `--audio-device` para SDL. Timeout 120 s por proceso; falla ante exit no
cero, error gráfico, contenido ausente o máscaras/conteos diferentes. Conserva
los fallos de presupuesto como datos: no los borra ni cambia el contrato.

Para las 1.000 llamadas dentro de capacidad y el control de crecimiento, ejecutar
`gloom.allocation_profile`; la prueba propia de lighting también conserva su control
del 110. Para tamaños de wrappers, `gloom.jobs` con diagnóstico ON (report frames=0,
ventana cerrada: solo metadata, no medida de asignaciones del scheduler).

Los escenarios futuros de poses/partículas/entities/snapshots se definen en
113/115/116/122/126 y [contratos](contratos.md). No se han ejecutado artificialmente
para declarar cero asignaciones de esas rutas antes de migrarlas.
