# Medidas del cierre 117

7 de octubre de 2026. Ryzen 7 3700X, GTX 1070, driver 581.29, MSVC 14.44,
Release x64. Base previa a esta implementación: `54a51b7`; ejecutables y libs
guardados antes de editar. El catálogo/bind ya estaban en esa base. Sus mejoras
frente al 116 se conservan separadas en [medidas-parciales.md](medidas-parciales.md),
incluido el lookup más lento; no se vuelven a atribuir al cierre nuevo.

Ningún build/test simultáneo durante las series de rendimiento. Diagnóstico
normal OFF; el contador de asignaciones se ejecuta en otro proceso/probe.
Los [datos versionados](datos/manifest.md) fijan hashes de binarios, 19 archivos
propietarios y conteo de código. Los bloques JSON en Markdown mantienen cada pasada, sin descartar picos.
En runtime-antes el diff del runner describe el workspace del momento, no las
fuentes del ejecutable guardado: mandan su SHA y la base indicada aquí.

## Cocinado completo

Cinco pares antes/después por caso, seriales: 30 ejecuciones. Entrada pequeña
material_surface, Factory glTF y Hound v17 ya existente de 2048; no se regenera
arte. Incluye importación, mesh/LOD, compresión de todas las texturas y escritura.
GetProcessTimes suma CPU de todos los hilos; su quantum observado es 15,625 ms.
GetProcessMemoryInfo muestrea cada 2 ms: peak_working_set del SO y máximo privado
observado, que puede omitir un pico fugaz. Wall incluye arranque/cierre del proceso;
los archivos/caché del SO no se vacían entre pasadas. Valores: mediana de cinco;
dispersión = (máximo-mínimo)/mediana del wall.

| Caso | Versión | Wall s | CPU proceso s | Peak WS MiB | Privado muestreado MiB | Dispersión wall % |
| --- | --- | --- | --- | --- | --- | --- |
| small | before | 0,0378 | 0,0312 | 5,58 | 1,49 | 292,92 |
| small | after | 0,0375 | 0,0469 | 5,62 | 1,26 | 26,30 |
| factory | before | 8,6441 | 51,0781 | 170,92 | 168,50 | 3,43 |
| factory | after | 8,8752 | 50,5156 | 133,41 | 129,93 | 7,29 |
| hound | before | 9,8904 | 15,1562 | 170,17 | 167,85 | 2,28 |
| hound | after | 9,9497 | 15,2812 | 132,10 | 128,38 | 2,07 |

Factory WS 170,92 → 133,41 MiB (**−21,94 %**); Hound 170,17 → 132,10 MiB
(**−22,37 %**). Se libera RGBA/base/mips de staging antes de comprimir y se evita
duplicar el payload del cooker. Wall Factory +2,67 % y Hound +0,60 %, dentro de
la dispersión: **no se atribuye una ganancia de tiempo de cocinado**. El caso
pequeño dura decenas de milisegundos y CPU está cuantizada; no extrapolar su ratio.

Los 104 outputs (4 pequeños, 67 Factory, 33 Hound) son idénticos por SHA-256
en todos los pares. La serie pareada precede a los últimos ajustes de validación,
default identidad e inicialización; [cook-pareado.json](datos/cook-pareado.md)
conserva el SHA concreto de ese binario. Una pasada adicional con el binario final
reconfirma los 104 hashes: [cook-final.json](datos/cook-final.md); Factory
8,850 s /133,48 MiB WS, Hound 9,634 s /132,15 MiB. Es control, no sexta mediana.

## Carga y serialización de escena

Cinco pasadas normales por caso: 100 operaciones pequeñas, 20 Factory y cinco
Hound por pasada. Se leen los mismos blobs anteriores, con caché del SO sin
vaciar; cada iteración construye y destruye sus propios buffers. Stage 0 incluye
VFS read, envelope/fingerprint, decode de ImportedScene y destrucción. Stage 1
serializa una escena preparada fuera del intervalo, calcula fingerprint y
destruye la salida. No incluye decodificar/subir texturas ni cargar una escena
completa en GPU; esa frontera se comprueba con cooker/runtime. QPC por lote.

| Caso | Etapa | Antes ms | Dispersión antes % | Final ms | Dispersión final % |
| --- | --- | --- | --- | --- | --- |
| small | leer + decode + destruir | 0,173484 | 2,38 | 0,170179 | 9,33 |
| small | encode + fingerprint + destruir | 0,004642 | 3,38 | 0,002706 | 2,00 |
| factory | leer + decode + destruir | 22,296615 | 4,99 | 22,033680 | 3,52 |
| factory | encode + fingerprint + destruir | 16,943515 | 8,83 | 11,237140 | 6,94 |
| hound | leer + decode + destruir | 36,225680 | 5,98 | 35,578020 | 6,76 |
| hound | encode + fingerprint + destruir | 25,280420 | 1,21 | 16,652720 | 0,71 |

Encode Factory 16,943515 → 11,237140 ms y Hound 25,280420 → 16,652720 ms:
aproximadamente **34 % menos**, fuera del ruido de esas series. La lectura/decode
queda dentro de su dispersión; no se atribuye una mejora de carga. Preparar los
blobs/escena no forma parte del tiempo medido; los checksums antes/final coinciden.

Contador aparte, una operación completa por etapa; ordinary new/new[] y delete.
Bytes solicitados y máximo vivo incremental respecto a la fixture; excluye
aligned new, DLL, malloc/realloc directo, cabecera de medida y overhead del heap.
**No es RSS, memoria de GPU ni todo el heap del proceso.**

| Caso | Etapa | Versión | Llamadas | Bytes solicitados | Pico propio bytes | Delta vivo |
| --- | --- | --- | --- | --- | --- | --- |
| small | leer + decode + destruir | antes | 21 | 4328 | 3384 | 0 |
| small | leer + decode + destruir | final | 23 | 4368 | 3456 | 0 |
| small | encode + fingerprint + destruir | antes | 13 | 3051 | 1702 | 0 |
| small | encode + fingerprint + destruir | final | 1 | 976 | 976 | 0 |
| factory | leer + decode + destruir | antes | 295 | 12156947 | 12153267 | 0 |
| factory | leer + decode + destruir | final | 212 | 12159507 | 12158595 | 0 |
| factory | encode + fingerprint + destruir | antes | 34 | 15268841 | 8482378 | 0 |
| factory | encode + fingerprint + destruir | final | 1 | 4045999 | 4045999 | 0 |
| hound | leer + decode + destruir | antes | 9849 | 19996886 | 18812194 | 0 |
| hound | leer + decode + destruir | final | 1447 | 18729066 | 18724186 | 0 |
| hound | encode + fingerprint + destruir | antes | 42 | 22911758 | 12723528 | 0 |
| hound | encode + fingerprint + destruir | final | 1 | 6227136 | 6227136 | 0 |

Encode pasa a **una asignación** de capacidad exacta; el envelope externo conserva
su buffer propietario y su copia de payload al decodificar una entrada prestada.
Hound decode: 9.849 → 1.447 llamadas (−85,3 %); no se promete cero asignaciones
en carga. Small sube 21 → 23 llamadas y Factory suma 2.560 bytes solicitados/
5.328 bytes de pico: la validación de jerarquía ahora cubre también nodos sin skins.
Todas las ventanas terminan con delta vivo cero. Las cifras excluyen las
dependencias de texturas asíncronas y sus contextos propietarios del 118.

## Runtime normal integrado

Siete pasadas nuevas antes y siete finales, seriales: Factory ×3, Hound v17/LOD0
×2 con audio nulo y ×2 con audio real. 1920×1080, Immediate, 120 frames calientes
y 360 medidos. No se cambia arte ni renderer. Asignaciones del runtime OFF.

| Versión | Pasada | Media ms | p50 | p95 | p99 | Máximo | >5 ms |
| --- | --- | --- | --- | --- | --- | --- | --- |
| antes | factory-1 | 2.349 | 2.345 | 2.373 | 2.442 | 2.696 | 0/360 |
| antes | factory-2 | 2.346 | 2.343 | 2.364 | 2.390 | 2.701 | 0/360 |
| antes | factory-3 | 2.341 | 2.339 | 2.360 | 2.377 | 3.103 | 0/360 |
| antes | hound-null-1 | 3.284 | 3.274 | 3.509 | 3.540 | 3.723 | 0/360 |
| antes | hound-null-2 | 3.274 | 3.274 | 3.512 | 3.545 | 3.619 | 0/360 |
| antes | hound-audio-1 | 3.279 | 3.274 | 3.513 | 3.531 | 3.794 | 0/360 |
| antes | hound-audio-2 | 3.291 | 3.284 | 3.523 | 3.700 | 3.873 | 0/360 |
| final | factory-1 | 2.342 | 2.342 | 2.361 | 2.375 | 2.403 | 0/360 |
| final | factory-2 | 2.342 | 2.341 | 2.360 | 2.379 | 2.497 | 0/360 |
| final | factory-3 | 2.358 | 2.343 | 2.363 | 2.380 | 7.870 | 1/360 |
| final | hound-null-1 | 3.281 | 3.272 | 3.509 | 3.526 | 3.530 | 0/360 |
| final | hound-null-2 | 3.284 | 3.274 | 3.508 | 3.518 | 3.531 | 0/360 |
| final | hound-audio-1 | 3.280 | 3.278 | 3.510 | 3.524 | 3.543 | 0/360 |
| final | hound-audio-2 | 3.280 | 3.279 | 3.515 | 3.527 | 3.534 | 0/360 |
| final control | factory | 2.346 | 2.343 | 2.386 | 2.476 | 2.525 | 0/360 |

Tiempos habituales conservados, sin ganancia global de FPS atribuida. Hound final
**0/1.440 >5 ms**, p99 <4 ms; antes también 0/1.440. Factory final tiene **1/1.080
>5 ms**, pico 7,870 ms, que no se descarta. Control adicional Factory sin carga
simultánea: máximo 2,525 ms, 0/360 >5; no se reproduce y no se le asigna una causa
sin evidencia. No recertifica la partida humana de ocho ni sustituye H06.

Residencia GPU idéntica: Factory 130,24 MiB /Hound 345,26 MiB; BC5/BC7, Hound
155 visibles/619 draws/53 skinned, mask127, peak_maps9. Missing/evictions y
budget_limited medidos cero; el contador total conserva la limitación inicial.
Process private/WS se conserva en los datos y es una métrica distinta de residencia GPU.

## Control visual y verificación de herramientas

67 capturas completas antes/después y seis adicionales del ejecutable anterior.
Comparador nativo y umbrales intactos; thumbnails generados únicamente en caché.
UI idéntica, personajes máximo 1/255, Factory dentro de la variación del control
anterior; no se declara igualdad de todos sus píxeles. Detalle en el informe y
[datos visuales](datos/visual.md)/[control](datos/visual-control.md).

Una segunda serie separada comprueba tools/perf/measure_asset_load.py tal como
se entrega (tipos propios y hashes de fixtures/exes): mismas libs, checksums y
contadores exactos. [Datos de esa verificación](datos/load-herramienta.md), sin
mezclarlos con la serie final principal ni seleccionar los mejores tiempos.

## Reproducir

Con las dependencias/build existentes; CMake/CTest del entorno en PATH:

```powershell
rtk proxy cmake --build --preset windows-release --parallel 6
rtk proxy cmake --build --preset windows-debug --parallel 6
rtk proxy cmake --build --preset windows-release --target gloom_format_check
rtk proxy ctest --test-dir build/windows-vs -C Release --output-on-failure
rtk proxy ctest --test-dir build/windows-vs -C Debug --output-on-failure
rtk proxy python -X utf8 tools/perf/check_asset_paths.py
rtk proxy python -X utf8 tools/perf/measure_asset_cooking.py --before-cooker .cache/hito117/closure/before-gloom_asset_cooker.exe --output .cache/hito117/repeat-cook
rtk proxy python -X utf8 tools/perf/measure_asset_load.py --files .cache/hito117/closure/before --output .cache/hito117/repeat-load
rtk proxy python -X utf8 tools/perf/measure_cpp_baseline.py --output .cache/hito117/repeat-runtime
```

Para un nuevo antes/después, guardar binarios/libs/headers de la base antes de
editar; --libraries/--headers del probe aceptan esos directorios, con los nombres
gloom_engine.lib/gloom_asset_pipeline.lib. --files exige small/factory/hound con
scene.gasset, preparados por el cooker. Hound v17 procede de la caché existente:
no se recrea si falta. Las comparaciones requieren mismas entradas/dependencias.
Los probes se construyen en caché, sin añadir targets a producción. Herramientas
de medida Windows; logs/builds/blobs quedan en .cache, solo Markdown con datos/tablas versionado.
