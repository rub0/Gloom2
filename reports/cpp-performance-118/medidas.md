# Medidas del hito 118

7 de octubre de 2026. Ryzen 7 3700X / GTX 1070, driver 581.29, MSVC 14.44,
Release x64; base 934b73d. Librerías/headers/executable anteriores guardados antes
de editar. Series seriales, sin builds/tests simultáneos; runtime diagnóstico OFF.
Los hashes, cada pasada y el código del probe están en [datos](datos/manifest.md).

## Carga inicial, cancelación y recarga CPU

Cinco pares de procesos por caso, cuatro workers, mismo cooked del cierre 117
(4/67/33 blobs), caché del SO sin vaciar. El receptor consume y destruye uploads,
calcula checksums y declara residencia inmediata. Incluye lectura, envelope,
transcode BC5/BC7, jerarquía y rig; excluye waits/subidas GPU. QPC hasta ready y
drain de trabajos. Cancel mide última referencia desde ready; reload invalida
CPU y repite carga. No son tiempos de un frame de gameplay ni FPS.
Dispersión = (máximo − mínimo)/mediana, cinco pasadas normales.

| Caso | Etapa | Antes ms | Disp. % | Final ms | Disp. % |
| --- | --- | ---: | ---: | ---: | ---: |
| small | carga inicial | 15.074900 | 2.45 | 15.115000 | 6.63 |
| small | cancelación | 0.000700 | 42.86 | 0.000900 | 22.22 |
| small | recarga | 0.742100 | 43.55 | 0.727400 | 23.73 |
| factory | carga inicial | 1046.449700 | 3.63 | 1021.544600 | 2.48 |
| factory | cancelación | 0.015000 | 20.67 | 0.009600 | 17.71 |
| factory | recarga | 1049.273200 | 0.66 | 1002.636500 | 3.14 |
| hound | carga inicial | 1457.319200 | 4.03 | 1345.910200 | 2.60 |
| hound | cancelación | 0.306100 | 17.97 | 0.309100 | 6.02 |
| hound | recarga | 1487.594500 | 5.19 | 1339.193900 | 1.62 |

Carga inicial Hound −7,64 % y recarga Hound −9,98 %: superan la dispersión
de ambas series. Factory inicial −2,38 %, dentro del ruido (3,63 %). Su recarga
final baja 4,44 %, pero la serie previa de igual lógica registró dispersión
5,09 %: no se atribuye una ganancia estable Factory. Small queda en el ruido.
Cancel Hound +0,98 %, dentro del ruido (17,97 %/6,02 %).
Una retención inicial de ImportedScene hacía cancel 0,2774 → 0,9769 ms;
se corrigió al comprobar que todos los consumidores utilizan AnimationRig.
Esas cifras preliminares no se usan como resultado final.

## Asignaciones y memoria CPU

Contador separado, todos los threads, ordinary new/new[] y delete. Bytes pedidos
y pico vivo incremental respecto a la fixture, excluyendo aligned new, DLL,
malloc/realloc directo, cabecera de medida y overhead. No es RSS ni memoria GPU.
Lectura de contador y logs fuera de ventana. El receptor libera uploads al recibir;
el runtime real conserva staging según su backend. [Datos completos](datos/carga.md).

| Caso | Etapa | Versión | Calls | Bytes pedidos | Pico incremental bytes | Delta vivo |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| small | inicial | before | 165 | 19184 | 8468 | 6172 |
| small | inicial | after | 135 | 15080 | 6440 | 3224 |
| small | cancel | before | 0 | 0 | 0 | -392 |
| small | cancel | after | 0 | 0 | 0 | -232 |
| small | reload | before | 158 | 18544 | 2792 | 392 |
| small | reload | after | 122 | 14424 | 3656 | 312 |
| factory | inicial | before | 3761 | 390025175 | 238459721 | 157209958 |
| factory | inicial | after | 2574 | 236831639 | 161026310 | 75711701 |
| factory | cancel | before | 0 | 0 | 0 | -26079 |
| factory | cancel | after | 0 | 0 | 0 | -21112 |
| factory | reload | before | 3745 | 389999857 | 81277882 | 26079 |
| factory | reload | after | 2546 | 236821463 | 85342001 | 23576 |
| hound | inicial | before | 3860 | 943100025 | 578352035 | 386414974 |
| hound | inicial | after | 3225 | 571374373 | 386018707 | 187589776 |
| hound | cancel | before | 0 | 0 | 0 | -2381288 |
| hound | cancel | after | 0 | 0 | 0 | -2379208 |
| hound | reload | before | 3846 | 943086242 | 194318349 | 2381288 |
| hound | reload | after | 3200 | 571368917 | 200810747 | 2380248 |

Pico propio inicial Factory 238.459.721 → 161.026.310 bytes (−32,47 %),
Hound 578.352.035 → 386.018.707 (−33,25 %). Se retiran copias de payload,
copias de PreparedScene/futuro y staging retenido tras publicar. Bytes vivos
incrementales al quedar ready: Factory 157.209.958 → 75.711.701; Hound
386.414.974 → 187.589.776. No se promete cero heap durante carga inicial.

El pico incremental de reload sube (Factory +5,00 %, Hound +3,34 %): parte de una
base viva mucho menor en la versión final, que ya no conserva la preparación.
No interpretar ese delta como aumento del máximo total del proceso.
Los payloads de la caché del loader permanecen hasta invalidate/destrucción;
cancel retira instancia/rig y referencias GPU, sin vaciar esa caché compartida.

Medida adicional del SO en un proceso normal por variante/caso, consulta cada
2 ms: peak_working_set acumulado, privado máximo muestreado. Puede omitir picos
fugaces del privado. Son procesos adicionales, separados de las cinco series CPU.

| Caso | Antes peak WS MiB | Final peak WS MiB | Antes privado MiB | Final privado MiB |
| --- | ---: | ---: | ---: | ---: |
| small | 4.07 | 4.35 | 1.04 | 1.00 |
| factory | 232.22 | 157.95 | 229.77 | 152.55 |
| hound | 557.05 | 373.17 | 555.24 | 371.17 |

Factory WS −31,98 %, Hound −33,01 % en este control. Una pasada de memoria del
SO complementa el contador; no sustituye las cinco pasadas de tiempo ni aísla
todo el ahorro del allocator. [Contadores del SO](datos/memoria-proceso.md).

## Mil updates dentro de capacidad

64 solicitudes en queued, dispatch budget cero, una pasada de calentamiento;
después 1.000 updates idénticos. Antes 12.000 new, 4.544.000 bytes pedidos,
2.512 bytes de pico temporal; final **0 new /0 bytes /0 pico incremental** en
los tres procesos diagnóstico. El scratch final retiene capacidad (64×16 bytes),
reservada al crear tickets: cero incremental no significa almacenamiento cero.
Con una escena ready, ambas versiones ya daban 0; no atribuir ese cero al 118.
La nueva prueba mide también 1.000 updates con dos escenas compartidas ready y
un crecimiento positivo fuera de capacidad. Se cuentan todos los threads de la
ventana; malloc/aligned de bibliotecas quedan fuera del contador declarado.

## Runtime integrado

Tres Factory y cuatro Hound por versión, 1920×1080 nativo, Immediate,
120 frames calientes/360 medidos, Hound v17 LOD0, dos pasadas audio nulo y dos
dispositivo real. Base nueva medida con ejecutable 934b73d; no se reutiliza la
serie anterior del 117. Después se repitió la serie final tras la revisión de
nombres contados; hashes concretos de cada ejecutable en [manifest](datos/manifest.md).

| Versión | Caso | Media ms | p50 | p95 | p99 | Máximo | >5 ms |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| antes | factory-1 | 2.347 | 2.346 | 2.365 | 2.480 | 3.296 | 0/360 |
| antes | factory-2 | 2.344 | 2.342 | 2.362 | 2.388 | 2.510 | 0/360 |
| antes | factory-3 | 2.350 | 2.346 | 2.381 | 2.486 | 2.585 | 0/360 |
| antes | hound-null-1 | 3.283 | 3.278 | 3.514 | 3.538 | 3.558 | 0/360 |
| antes | hound-null-2 | 3.278 | 3.275 | 3.508 | 3.526 | 3.578 | 0/360 |
| antes | hound-audio-1 | 3.284 | 3.278 | 3.513 | 3.528 | 3.537 | 0/360 |
| antes | hound-audio-2 | 3.281 | 3.279 | 3.517 | 3.543 | 3.609 | 0/360 |
| final | factory-1 | 2.294 | 2.294 | 2.308 | 2.315 | 2.318 | 0/360 |
| final | factory-2 | 2.295 | 2.294 | 2.311 | 2.320 | 2.450 | 0/360 |
| final | factory-3 | 2.296 | 2.296 | 2.314 | 2.351 | 2.359 | 0/360 |
| final | hound-null-1 | 3.237 | 3.219 | 3.449 | 3.460 | 3.466 | 0/360 |
| final | hound-null-2 | 3.236 | 3.221 | 3.452 | 3.473 | 3.586 | 0/360 |
| final | hound-audio-1 | 3.238 | 3.221 | 3.455 | 3.476 | 3.582 | 0/360 |
| final | hound-audio-2 | 3.239 | 3.226 | 3.455 | 3.472 | 3.555 | 0/360 |

Controles adicionales de runtime y carga de la misma lógica quedan conservados
en [runtime-control](datos/runtime-control.md), [control previo al ajuste de
nombres](datos/runtime-pre-review.md) y [carga previa](datos/carga-pre-review.md).
Variación entre ejecuciones y hashes distintos impiden asignar el cambio global
de FPS al ahorro frío de memoria. El helper de nombres se ejecuta durante carga,
no por frame; conteos/render/contenido y shaders permanecen iguales.
Una repetición posterior del mismo ejecutable anterior confirma la variación:
Factory media 2,291–2,297 ms y Hound 3,219–3,241 ms, también 0/360 >5 ms
en cada pasada. [Control de la base](datos/runtime-antes-control.md).

No se atribuye una mejora global de FPS; el ahorro demostrado corresponde a carga
y memoria. Ambas series: Factory 0/1.080 y Hound **0/1.440 >5 ms**, Hound p99<4 ms,
máximo final 3,586 ms. GPU Hound **345,26 MiB**, Factory 130,24 MiB; missing y
evictions cero, budget_limited_frames medidos cero. El contador acumulado de
budget_limited_frames incluye carga inicial (hasta 30 en Hound), no se oculta.
Presupuesto configurable 512 MiB /sobre Hound ≤384 MiB intactos. Paletas Hound
peak_maps=9, 53 skinned visibles, TPS mask=127 y weapon_mask=31 conservados.
No recertifica H06 ni integra ocho jugadores humanos.
