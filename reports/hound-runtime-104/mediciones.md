# Hito 104 — mediciones finales

29/09/2026. Release, Ryzen 7 3700X, GTX 1070, 1920×1080 nativo, ventana, IMMEDIATE efectivo, VSync off, dos imágenes.
Cada fila: 120 frames de calentamiento después de cargar y 360 frames medidos. GPU: 359 consultas de intervalo completo, lectura diferida cuatro frames.
Audio real: SDL3 `Headphones (High Definition Audio Device)`. HUD y disparo en todos los casos. Casos ejecutados en serie, sin compilaciones ni otros tests simultáneos.

## Fotograma CPU completo (ms)

| Caso / salida / pasada | Media | FPS | p50 | p95 | p99 | Máximo | >5 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Dos originales (control) / nulo A | 2.393 | 417.93 | 2.353 | 2.573 | 2.593 | 2.742 | 0/360 |
| Ocho originales / nulo A | 2.556 | 391.19 | 2.515 | 2.742 | 2.752 | 2.879 | 0/360 |
| Un Hound + FPS / nulo A | 2.587 | 386.52 | 2.550 | 2.758 | 2.775 | 2.880 | 0/360 |
| Siete Hound + FPS / nulo A | 3.543 | 282.24 | 3.597 | 3.810 | 3.827 | 3.864 | 0/360 |
| Dos originales (control) / nulo B | 2.402 | 416.25 | 2.360 | 2.586 | 2.604 | 2.738 | 0/360 |
| Ocho originales / nulo B | 2.555 | 391.35 | 2.514 | 2.738 | 2.747 | 2.933 | 0/360 |
| Un Hound + FPS / nulo B | 2.591 | 386.02 | 2.551 | 2.760 | 2.773 | 2.875 | 0/360 |
| Siete Hound + FPS / nulo B | 3.553 | 281.43 | 3.600 | 3.819 | 3.831 | 3.844 | 0/360 |
| Dos originales (control) / audio real A | 2.399 | 416.80 | 2.359 | 2.583 | 2.592 | 2.611 | 0/360 |
| Ocho originales / audio real A | 2.548 | 392.40 | 2.528 | 2.749 | 2.762 | 2.789 | 0/360 |
| Un Hound + FPS / audio real A | 2.597 | 385.09 | 2.559 | 2.764 | 2.777 | 2.822 | 0/360 |
| Siete Hound + FPS / audio real A | 3.521 | 283.98 | 3.612 | 3.826 | 3.854 | 3.866 | 0/360 |
| Dos originales (control) / audio real B | 2.401 | 416.42 | 2.361 | 2.583 | 2.604 | 2.634 | 0/360 |
| Ocho originales / audio real B | 2.565 | 389.82 | 2.525 | 2.750 | 2.762 | 2.779 | 0/360 |
| Un Hound + FPS / audio real B | 2.596 | 385.23 | 2.558 | 2.765 | 2.772 | 2.823 | 0/360 |
| Siete Hound + FPS / audio real B | 3.555 | 281.29 | 3.609 | 3.828 | 3.844 | 3.974 | 0/360 |

## Intervalo GPU completo (ms)

| Caso / salida / pasada | Media | p50 | p95 | p99 | Máximo |
| --- | ---: | ---: | ---: | ---: | ---: |
| Dos originales (control) / nulo A | 2.100 | 2.058 | 2.280 | 2.288 | 2.372 |
| Ocho originales / nulo A | 2.265 | 2.224 | 2.447 | 2.451 | 2.459 |
| Un Hound + FPS / nulo A | 2.297 | 2.257 | 2.462 | 2.467 | 2.609 |
| Siete Hound + FPS / nulo A | 3.254 | 3.304 | 3.518 | 3.532 | 3.534 |
| Dos originales (control) / nulo B | 2.111 | 2.071 | 2.290 | 2.296 | 2.297 |
| Ocho originales / nulo B | 2.266 | 2.224 | 2.446 | 2.450 | 2.460 |
| Un Hound + FPS / nulo B | 2.297 | 2.257 | 2.461 | 2.465 | 2.468 |
| Siete Hound + FPS / nulo B | 3.263 | 3.305 | 3.519 | 3.533 | 3.537 |
| Dos originales (control) / audio real A | 2.107 | 2.065 | 2.287 | 2.292 | 2.296 |
| Ocho originales / audio real A | 2.257 | 2.234 | 2.458 | 2.463 | 2.468 |
| Un Hound + FPS / audio real A | 2.302 | 2.263 | 2.470 | 2.475 | 2.480 |
| Siete Hound + FPS / audio real A | 3.226 | 3.315 | 3.529 | 3.545 | 3.551 |
| Dos originales (control) / audio real B | 2.109 | 2.066 | 2.288 | 2.291 | 2.294 |
| Ocho originales / audio real B | 2.271 | 2.231 | 2.454 | 2.459 | 2.465 |
| Un Hound + FPS / audio real B | 2.301 | 2.262 | 2.468 | 2.473 | 2.476 |
| Siete Hound + FPS / audio real B | 3.262 | 3.313 | 3.528 | 3.541 | 3.556 |

## CPU y cola (ms)

La finalización de envío es una **cota superior observada en CPU**: desde el envío hasta detectar su fence completado al comenzar otro frame. Incluye trabajo GPU y demora hasta la observación; no es latencia de escaneo ni medida aislada del tiempo antes de empezar la GPU. Máximo observado: un envío previo pendiente al comenzar el frame; swapchain de dos imágenes.

| Caso / salida / pasada | Poses | Skin/bounds | Draw CPU | End/Present | Acquire | Espera fence | Finalización p50 / p95 / p99 / máx. |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Dos originales (control) / nulo A | 0.116 | 0.019 | 0.457 | 1.418 | 0.002 | 1.316 | 3.990 / 4.431 / 4.578 / 4.621 |
| Ocho originales / nulo A | 0.405 | 0.059 | 0.508 | 1.215 | 0.002 | 1.122 | 3.974 / 4.460 / 4.593 / 4.897 |
| Un Hound + FPS / nulo A | 0.078 | 0.030 | 0.429 | 1.679 | 0.002 | 1.585 | 4.448 / 4.905 / 5.045 / 5.106 |
| Siete Hound + FPS / nulo A | 0.146 | 0.124 | 0.611 | 2.221 | 0.003 | 2.115 | 6.122 / 6.633 / 6.795 / 6.846 |
| Dos originales (control) / nulo B | 0.117 | 0.018 | 0.457 | 1.426 | 0.002 | 1.329 | 4.019 / 4.426 / 4.626 / 4.672 |
| Ocho originales / nulo B | 0.404 | 0.060 | 0.501 | 1.242 | 0.002 | 1.151 | 3.989 / 4.418 / 4.623 / 4.852 |
| Un Hound + FPS / nulo B | 0.077 | 0.032 | 0.438 | 1.648 | 0.002 | 1.549 | 4.444 / 4.837 / 4.997 / 5.088 |
| Siete Hound + FPS / nulo B | 0.144 | 0.118 | 0.591 | 2.282 | 0.002 | 2.186 | 6.160 / 6.639 / 6.806 / 6.848 |
| Dos originales (control) / audio real A | 0.123 | 0.015 | 0.440 | 1.478 | 0.002 | 1.387 | 4.054 / 4.481 / 4.663 / 4.757 |
| Ocho originales / audio real A | 0.400 | 0.052 | 0.492 | 1.259 | 0.002 | 1.170 | 4.004 / 4.458 / 4.592 / 4.681 |
| Un Hound + FPS / audio real A | 0.074 | 0.028 | 0.423 | 1.702 | 0.002 | 1.610 | 4.495 / 4.894 / 5.114 / 5.187 |
| Siete Hound + FPS / audio real A | 0.140 | 0.106 | 0.577 | 2.303 | 0.002 | 2.209 | 6.195 / 6.669 / 6.853 / 6.949 |
| Dos originales (control) / audio real B | 0.115 | 0.015 | 0.438 | 1.472 | 0.002 | 1.382 | 4.068 / 4.492 / 4.662 / 4.750 |
| Ocho originales / audio real B | 0.396 | 0.056 | 0.488 | 1.286 | 0.002 | 1.200 | 4.031 / 4.470 / 4.647 / 4.722 |
| Un Hound + FPS / audio real B | 0.075 | 0.028 | 0.422 | 1.711 | 0.002 | 1.619 | 4.491 / 4.907 / 5.103 / 5.152 |
| Siete Hound + FPS / audio real B | 0.143 | 0.110 | 0.585 | 2.316 | 0.002 | 2.222 | 6.219 / 6.698 / 6.856 / 7.015 |

## Residencia y visibilidad

Conteos del último frame. En los casos ocho/original y Hound se comprueba la visibilidad en **cada frame medido**, y máscara 127 confirma cambios de pose en los siete TPS. El control antiguo de dos mantiene su colocación original: dos skins enviadas, una en el frustum al final; no sirve para restar un coste exacto por personaje. Todos los intervalos medidos registran cero evicciones y cero frames limitados por residencia; Hound acumula 30 evicciones durante carga/calentamiento.

| Caso / salida / pasada | Skins enviadas / frustum | Draws / sombras | Mapeos / bytes reservados | Residencia assets MiB | Privada / working set MiB |
| --- | ---: | ---: | ---: | ---: | ---: |
| Dos originales (control) / nulo A | 2 / 1 | 340 / 244 | 3 / 147456 | 504.29 | 2269.42 / 1132.61 |
| Ocho originales / nulo A | 8 / 8 | 406 / 296 | 9 / 442368 | 504.29 | 2290.08 / 1149.52 |
| Un Hound + FPS / nulo A | 11 / 11 | 341 / 262 | 3 / 147456 | 511.73 | 2278.51 / 1170.29 |
| Siete Hound + FPS / nulo A | 53 / 53 | 527 / 406 | 9 / 442368 | 511.73 | 2278.73 / 1171.34 |
| Dos originales (control) / nulo B | 2 / 1 | 340 / 244 | 3 / 147456 | 504.29 | 2273.49 / 1132.05 |
| Ocho originales / nulo B | 8 / 8 | 406 / 296 | 9 / 442368 | 504.29 | 2287.88 / 1147.66 |
| Un Hound + FPS / nulo B | 11 / 11 | 341 / 262 | 3 / 147456 | 511.73 | 2281.34 / 1171.55 |
| Siete Hound + FPS / nulo B | 53 / 53 | 527 / 406 | 9 / 442368 | 511.73 | 2282.82 / 1170.25 |
| Dos originales (control) / audio real A | 2 / 1 | 340 / 244 | 3 / 147456 | 504.29 | 2264.27 / 1133.56 |
| Ocho originales / audio real A | 8 / 8 | 406 / 296 | 9 / 442368 | 504.29 | 2286.63 / 1148.47 |
| Un Hound + FPS / audio real A | 11 / 11 | 341 / 262 | 3 / 147456 | 511.73 | 2286.95 / 1172.36 |
| Siete Hound + FPS / audio real A | 53 / 53 | 527 / 406 | 9 / 442368 | 511.73 | 2284.86 / 1173.46 |
| Dos originales (control) / audio real B | 2 / 1 | 340 / 244 | 3 / 147456 | 504.29 | 2273.64 / 1132.63 |
| Ocho originales / audio real B | 8 / 8 | 406 / 296 | 9 / 442368 | 504.29 | 2290.55 / 1149.55 |
| Un Hound + FPS / audio real B | 11 / 11 | 341 / 262 | 3 / 147456 | 511.73 | 2280.00 / 1172.08 |
| Siete Hound + FPS / audio real B | 53 / 53 | 527 / 406 | 9 / 442368 | 511.73 | 2282.36 / 1172.08 |

Los 511,73 MiB son contabilidad de recursos del motor frente a su límite de 512 MiB, no VRAM total del controlador. No incluyen todos los targets, heaps y objetos Vulkan. Logs y capturas regenerables en `.cache/hito104-final-{a,b,audio-a,audio-b}-<caso>-immediate.*`; no se versionan.
