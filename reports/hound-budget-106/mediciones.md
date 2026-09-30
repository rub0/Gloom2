# Hito 106 — distribuciones del presupuesto

30 de septiembre de 2026 · [Decisión, método y límites](README.md).

Release, Ryzen 7 3700X / GTX 1070, 1920×1080 nativo, IMMEDIATE, VSync off, ventana. Dos repeticiones con audio nulo y dos con SDL3 Headphones (High Definition Audio Device).
Cada pasada carga, espera 120 frames y mide 360 CPU / 359 consultas GPU completas. Ejecuciones en serie sin builds/tests simultáneos. Los nombres LOD0/1/2 son índices forzados de la v16 diagnóstica, no entregas de H07.

## CPU completa (ms)

| Caso | Media | FPS | p50 | p95 | p99 | Máximo | >5 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| v16-null-a | 3.776 | 264.86 | 3.662 | 4.128 | 4.346 | 9.715 | 1/360 |
| v16-null-b | 3.577 | 279.57 | 3.637 | 4.002 | 4.297 | 4.430 | 0/360 |
| v16-audio-a | 3.646 | 274.31 | 3.650 | 4.075 | 4.251 | 4.420 | 0/360 |
| v16-audio-b | 3.648 | 274.16 | 3.649 | 4.029 | 4.201 | 4.382 | 0/360 |
| budget-lod0-null-a | 3.781 | 264.49 | 3.759 | 4.155 | 4.356 | 6.521 | 1/360 |
| budget-lod0-null-b | 3.883 | 257.52 | 3.951 | 4.293 | 4.511 | 4.700 | 0/360 |
| budget-lod0-audio-a | 3.900 | 256.38 | 3.955 | 4.312 | 4.556 | 4.692 | 0/360 |
| budget-lod0-audio-b | 3.778 | 264.69 | 3.772 | 4.181 | 4.330 | 4.534 | 0/360 |
| budget-lod1-null-a | 3.347 | 298.76 | 3.305 | 3.692 | 3.903 | 4.503 | 0/360 |
| budget-lod1-null-b | 3.348 | 298.69 | 3.306 | 3.692 | 3.834 | 3.881 | 0/360 |
| budget-lod1-audio-a | 3.365 | 297.20 | 3.316 | 3.724 | 3.911 | 3.927 | 0/360 |
| budget-lod1-audio-b | 3.370 | 296.77 | 3.326 | 3.704 | 3.914 | 3.940 | 0/360 |
| budget-lod2-null-a | 3.058 | 327.01 | 2.971 | 3.344 | 3.532 | 3.624 | 0/360 |
| budget-lod2-null-b | 3.053 | 327.60 | 2.960 | 3.382 | 3.604 | 3.915 | 0/360 |
| budget-lod2-audio-a | 3.065 | 326.31 | 2.978 | 3.363 | 3.514 | 3.623 | 0/360 |
| budget-lod2-audio-b | 3.061 | 326.71 | 2.978 | 3.346 | 3.489 | 3.540 | 0/360 |

## GPU completa (ms)

| Caso | Media | p50 | p95 | p99 | Máximo |
| --- | ---: | ---: | ---: | ---: | ---: |
| v16-null-a | 3.443 | 3.353 | 3.795 | 3.961 | 4.113 |
| v16-null-b | 3.259 | 3.339 | 3.677 | 3.959 | 4.106 |
| v16-audio-a | 3.331 | 3.347 | 3.762 | 3.957 | 4.124 |
| v16-audio-b | 3.327 | 3.348 | 3.721 | 3.890 | 4.112 |
| budget-lod0-null-a | 3.454 | 3.459 | 3.829 | 4.002 | 4.221 |
| budget-lod0-null-b | 3.546 | 3.608 | 3.999 | 4.045 | 4.300 |
| budget-lod0-audio-a | 3.567 | 3.612 | 4.026 | 4.070 | 4.302 |
| budget-lod0-audio-b | 3.456 | 3.465 | 3.854 | 4.013 | 4.220 |
| budget-lod1-null-a | 3.023 | 3.000 | 3.363 | 3.583 | 3.611 |
| budget-lod1-null-b | 3.031 | 3.002 | 3.377 | 3.541 | 3.592 |
| budget-lod1-audio-a | 3.044 | 3.011 | 3.391 | 3.619 | 3.630 |
| budget-lod1-audio-b | 3.043 | 3.011 | 3.371 | 3.598 | 3.636 |
| budget-lod2-null-a | 2.743 | 2.664 | 2.981 | 3.210 | 3.310 |
| budget-lod2-null-b | 2.738 | 2.664 | 3.032 | 3.213 | 3.285 |
| budget-lod2-audio-a | 2.744 | 2.670 | 3.054 | 3.166 | 3.288 |
| budget-lod2-audio-b | 2.734 | 2.666 | 2.951 | 3.152 | 3.250 |

## CPU por trabajo y residencia

| Caso | Poses ms | Skin/bounds ms | Draws / sombras | Assets MiB | Privada / working set MiB |
| --- | ---: | ---: | --- | ---: | --- |
| v16-null-a | 0.148 | 0.113 | 584 / 436 | 183.54 | 1240.08 / 673.13 |
| v16-null-b | 0.143 | 0.111 | 584 / 436 | 183.54 | 1242.10 / 673.50 |
| v16-audio-a | 0.140 | 0.100 | 584 / 436 | 183.54 | 1242.14 / 673.88 |
| v16-audio-b | 0.143 | 0.103 | 584 / 436 | 183.54 | 1242.37 / 676.44 |
| budget-lod0-null-a | 0.551 | 0.113 | 619 / 464 | 355.43 | 1798.45 / 1008.58 |
| budget-lod0-null-b | 0.557 | 0.108 | 619 / 464 | 355.43 | 1803.58 / 1008.92 |
| budget-lod0-audio-a | 0.539 | 0.091 | 619 / 464 | 355.43 | 1779.47 / 1004.58 |
| budget-lod0-audio-b | 0.534 | 0.091 | 619 / 464 | 355.43 | 1794.46 / 1006.79 |
| budget-lod1-null-a | 0.566 | 0.108 | 619 / 464 | 355.43 | 1798.85 / 1005.98 |
| budget-lod1-null-b | 0.542 | 0.103 | 619 / 464 | 355.43 | 1800.85 / 1007.42 |
| budget-lod1-audio-a | 0.533 | 0.090 | 619 / 464 | 355.43 | 1804.66 / 1010.55 |
| budget-lod1-audio-b | 0.558 | 0.091 | 619 / 464 | 355.43 | 1800.03 / 1010.14 |
| budget-lod2-null-a | 0.520 | 0.112 | 619 / 464 | 355.43 | 1820.62 / 1036.11 |
| budget-lod2-null-b | 0.532 | 0.110 | 619 / 464 | 355.43 | 1799.66 / 1005.28 |
| budget-lod2-audio-a | 0.523 | 0.092 | 619 / 464 | 355.43 | 1817.73 / 1028.52 |
| budget-lod2-audio-b | 0.528 | 0.089 | 619 / 464 | 355.43 | 1785.85 / 1006.94 |

Todas las pasadas: 53/53 primitivas skinned, máscara TPS 127, nueve mapeos, cero mallas/texturas ausentes y cero evicciones durante la medida. Carga completa sin evicciones también. Las pruebas budget usan las cinco armas (máscara 31).

Se conservan los dos picos: v16-null-a 9,715 ms y budget-lod0-null-a 6,521 ms. Sus intervalos GPU máximos son menores de 5 ms; no se atribuye una causa de sistema sin una traza. No se certifica la v16 de 80.152 triángulos por su media.

La referencia elegida para el presupuesto es budget-lod1: 40.073 triángulos TPS más 18.108 FPS, cuatro pasadas / 1.440 frames, ninguno >5 ms; p99 máximo 3,914 ms y máximo 4,503 ms. H07 recibe un límite redondeado hacia abajo de 40.000 TPS / 18.000 FPS.

Assets es contabilidad del motor, no VRAM total. Logs/capturas/JSON regenerables: `.cache/hound-budget-106/`. La octava colección de cuatro mapas ocupa memoria; siete colecciones se muestrean en los siete materiales de todos los cuerpos. No se simulan ocho geometrías distintas.
