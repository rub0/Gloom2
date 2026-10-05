# Medidas del avance parcial 117

5 de octubre de 2026. Base `f38a4a8` (116 cerrado). Ryzen 7 3700X /GTX 1070,
driver 581.29, MSVC 14.44, Release x64. No builds, tests ni aplicaciones gráficas
simultáneos durante las series. El 117 sigue abierto; esto mide catálogo y
evaluación de bind, no el tiempo completo de cook/load de assets.

## Control CPU, contador OFF

Cinco pasadas alternadas antes/después. Mismos registros y cadena de dependencias
a 8/64/1.024; mismas matrices identidad y joints invertidos a 8/53/256. Fixtures
preparadas fuera del intervalo. Find: 1.000 lotes, cada uno de 32 consultas por
registro; ns por consulta. Order: 100 recorridos con salida nueva por pasada.
Lifecycle: 100 creaciones y destrucciones del catálogo, incluidos metadatos.
Rig: 1.000 validaciones de bind, sin vértices ni mallas en esta carga sintética.
Los tres últimos tiempos son ns por operación completa, no por nodo.
QPC de 100 ns por lote; resolución de find con ocho entradas: 0,391 ns/consulta.
Checksum común (suma uint64, módulo 2^64): **15405493449735111304**.

| Etapa | Entradas/joints | Pasada | Antes mediana /p95 ns | Después mediana /p95 ns |
| --- | ---: | ---: | ---: | ---: |
| find | 8 | 1 | 2.344 /2.344 | 4.688 /4.688 |
| find | 8 | 2 | 2.344 /2.344 | 7.031 /7.812 |
| find | 8 | 3 | 2.344 /2.344 | 4.688 /5.078 |
| find | 8 | 4 | 2.344 /2.344 | 4.688 /5.078 |
| find | 8 | 5 | 2.344 /2.344 | 7.031 /7.812 |
| find | 64 | 1 | 2.490 /3.174 | 9.033 /9.424 |
| find | 64 | 2 | 2.490 /3.223 | 9.033 /9.570 |
| find | 64 | 3 | 2.490 /3.223 | 8.984 /9.814 |
| find | 64 | 4 | 2.393 /3.174 | 8.984 /9.082 |
| find | 64 | 5 | 2.490 /2.539 | 9.033 /9.717 |
| find | 1024 | 1 | 3.183 /3.531 | 16.275 /18.054 |
| find | 1024 | 2 | 3.119 /3.403 | 16.333 /18.127 |
| find | 1024 | 3 | 3.091 /3.296 | 16.443 /18.097 |
| find | 1024 | 4 | 3.116 /3.363 | 16.296 /18.073 |
| find | 1024 | 5 | 3.082 /3.278 | 16.660 /18.521 |
| lifecycle | 8 | 1 | 3200.000 /3700.000 | 1900.000 /2500.000 |
| lifecycle | 8 | 2 | 3000.000 /3500.000 | 3000.000 /3500.000 |
| lifecycle | 8 | 3 | 3100.000 /3600.000 | 1900.000 /2500.000 |
| lifecycle | 8 | 4 | 3000.000 /3500.000 | 1900.000 /2400.000 |
| lifecycle | 8 | 5 | 3100.000 /3600.000 | 3000.000 /3600.000 |
| lifecycle | 64 | 1 | 22000.000 /23800.000 | 21400.000 /22500.000 |
| lifecycle | 64 | 2 | 21400.000 /23400.000 | 21500.000 /27400.000 |
| lifecycle | 64 | 3 | 21700.000 /23200.000 | 21700.000 /23500.000 |
| lifecycle | 64 | 4 | 21700.000 /23200.000 | 21800.000 /22600.000 |
| lifecycle | 64 | 5 | 21800.000 /23200.000 | 21600.000 /24400.000 |
| lifecycle | 1024 | 1 | 398100.000 /433500.000 | 394900.000 /441800.000 |
| lifecycle | 1024 | 2 | 381300.000 /418100.000 | 401400.000 /447500.000 |
| lifecycle | 1024 | 3 | 389000.000 /416200.000 | 400400.000 /442900.000 |
| lifecycle | 1024 | 4 | 396800.000 /442900.000 | 402800.000 /451400.000 |
| lifecycle | 1024 | 5 | 395800.000 /436100.000 | 395000.000 /456100.000 |
| order | 8 | 1 | 800.000 /1300.000 | 300.000 /500.000 |
| order | 8 | 2 | 1800.000 /2200.000 | 400.000 /500.000 |
| order | 8 | 3 | 800.000 /1300.000 | 300.000 /700.000 |
| order | 8 | 4 | 1900.000 /3500.000 | 300.000 /400.000 |
| order | 8 | 5 | 800.000 /1200.000 | 400.000 /500.000 |
| order | 64 | 1 | 5800.000 /6700.000 | 1900.000 /2000.000 |
| order | 64 | 2 | 7100.000 /7800.000 | 1900.000 /2000.000 |
| order | 64 | 3 | 5800.000 /6700.000 | 2000.000 /2100.000 |
| order | 64 | 4 | 5800.000 /6600.000 | 2000.000 /2100.000 |
| order | 64 | 5 | 5800.000 /6700.000 | 1900.000 /2000.000 |
| order | 1024 | 1 | 118300.000 /128000.000 | 39300.000 /41200.000 |
| order | 1024 | 2 | 85500.000 /92800.000 | 39000.000 /41400.000 |
| order | 1024 | 3 | 116200.000 /146300.000 | 38900.000 /41200.000 |
| order | 1024 | 4 | 84200.000 /98600.000 | 39600.000 /42100.000 |
| order | 1024 | 5 | 120600.000 /149500.000 | 39100.000 /40000.000 |
| rig | 8 | 1 | 4100.000 /4500.000 | 1600.000 /1600.000 |
| rig | 8 | 2 | 1800.000 /2000.000 | 1500.000 /1600.000 |
| rig | 8 | 3 | 1800.000 /2300.000 | 1500.000 /1600.000 |
| rig | 8 | 4 | 1900.000 /1900.000 | 1500.000 /1600.000 |
| rig | 8 | 5 | 4100.000 /4500.000 | 1500.000 /1600.000 |
| rig | 53 | 1 | 9200.000 /9700.000 | 8100.000 /8300.000 |
| rig | 53 | 2 | 9200.000 /9700.000 | 8200.000 /8500.000 |
| rig | 53 | 3 | 10400.000 /11400.000 | 8100.000 /8200.000 |
| rig | 53 | 4 | 9100.000 /9600.000 | 8100.000 /8300.000 |
| rig | 53 | 5 | 9200.000 /9700.000 | 8200.000 /8500.000 |
| rig | 256 | 1 | 41000.000 /42100.000 | 37900.000 /38700.000 |
| rig | 256 | 2 | 40500.000 /41600.000 | 37900.000 /38800.000 |
| rig | 256 | 3 | 41600.000 /43100.000 | 38900.000 /40200.000 |
| rig | 256 | 4 | 40600.000 /41600.000 | 38800.000 /40300.000 |
| rig | 256 | 5 | 41500.000 /42500.000 | 37900.000 /39100.000 |

Mediana entre pasadas; ruido `(máximo-mínimo)/mediana` de sus medianas:

| Etapa | Entradas/joints | Antes ns | Después ns | Ruido antes % | Ruido después % |
| --- | ---: | ---: | ---: | ---: | ---: |
| find | 8 | 2.344 | 4.688 | 0.00 | 49.98 |
| find | 64 | 2.490 | 9.033 | 3.90 | 0.54 |
| find | 1024 | 3.116 | 16.333 | 3.24 | 2.36 |
| lifecycle | 8 | 3100.000 | 1900.000 | 6.45 | 57.89 |
| lifecycle | 64 | 21700.000 | 21600.000 | 2.76 | 1.85 |
| lifecycle | 1024 | 395800.000 | 400400.000 | 4.24 | 1.97 |
| order | 8 | 800.000 | 300.000 | 137.50 | 33.33 |
| order | 64 | 5800.000 | 1900.000 | 22.41 | 5.26 |
| order | 1024 | 116200.000 | 39100.000 | 31.33 | 1.79 |
| rig | 8 | 1900.000 | 1500.000 | 121.05 | 6.67 |
| rig | 53 | 9200.000 | 8100.000 | 14.13 | 1.23 |
| rig | 256 | 41000.000 | 37900.000 | 2.68 | 2.64 |

**Tradeoff medido:** find a 1.024 pasa de 3,116 a 16,333 ns/consulta; la
búsqueda binaria es más lenta que el hash anterior. Ambos asignan cero. El ID
está junto al puntero en un índice compacto, evitando leer cada registro durante
la búsqueda. No se atribuye una mejora de lookup ni se oculta este coste.
Los callers reales consultan durante carga, preparación y recarga:
AsyncAssetLoader::request, AssetResidencyCoordinator::reload y la preparación
de dependencias pendientes; no ordenan ni recorren el catálogo cada frame estable.

Order a 1.024: 116.200 → 39.100 ns, mejora superior al ruido observado.
Lifecycle 395.800 → 400.400 ns: diferencia del 1,16 %, dentro del ruido del 4,24 %.
Rig de 256 joints: 41.000 → 37.900 ns, supera ruido del 2,68 %. A 8/53 hay
diferencias de medianas, pero su dispersión no permite atribuirles mejora.
No se extrapola el control a FPS ni al coste de importación completo.

## Asignaciones y memoria, diagnóstico separado

Probe temporal de ordinary new/new[] y seguimiento de delete, mono-thread.
Incluye las copias de strings/dependencies legacy de AssetRecord; excluye
aligned new, DLL, malloc/realloc directos, cabecera propia y overhead del heap.
Las asignaciones permitidas de carga se declaran; no es RSS ni heap global.
El catálogo ordenado conserva registros en asignaciones individuales para que
sus direcciones no cambien. La tabla crece al insertar; no hay mapa en AssetCatalog.

| Entradas | Versión | Cold llamadas /bytes | Find llamadas /bytes | 100 order llamadas /bytes | 100 lifecycle llamadas /bytes |
| ---: | --- | ---: | ---: | ---: | ---: |
| 8 | before | 33 /2,064 | 0 /0 | 1,100 /48,000 | 3,300 /206,400 |
| 8 | after | 35 /1,832 | 0 /0 | 300 /20,000 | 3,500 /183,200 |
| 64 | before | 258 /15,632 | 0 /0 | 6,800 /374,400 | 25,800 /1,563,200 |
| 64 | after | 262 /14,824 | 0 /0 | 300 /160,000 | 26,200 /1,482,400 |
| 1024 | before | 4,100 /255,326 | 0 /0 | 103,000 /6,683,700 | 410,000 /25,532,600 |
| 1024 | after | 4,106 /237,544 | 0 /0 | 300 /2,560,000 | 410,600 /23,754,400 |

| Entradas | Versión | sizeof catálogo | Heap vivo | Peak heap | Total vivo |
| ---: | --- | ---: | ---: | ---: | ---: |
| 8 | before | 64 | 2,064 | 2,064 | 2,128 |
| 8 | after | 24 | 1,720 | 1,720 | 1,744 |
| 64 | before | 64 | 15,504 | 15,504 | 15,568 |
| 64 | after | 24 | 13,816 | 13,816 | 13,840 |
| 1024 | before | 64 | 245,943 | 245,943 | 246,007 |
| 1024 | after | 24 | 221,176 | 221,176 | 221,200 |

Con 1.024: **246.007 → 221.200 bytes (-10,08 %)**, objeto incluido.
Cold asigna ligeramente más veces (4.100 → 4.106) por crecimiento de la tabla,
pero pide menos bytes. No se afirma cero asignaciones de carga.
Order pasa de 1.030 a tres asignaciones por operación a 1.024, en esta fixture.

| Joints | Versión | 1.000 validaciones bind: llamadas /bytes |
| ---: | --- | ---: |
| 8 | before | 9,000 /676,000 |
| 8 | after | 4,000 /580,000 |
| 53 | before | 14,000 /4,576,000 |
| 53 | after | 4,000 /3,824,000 |
| 256 | before | 18,000 /22,279,000 |
| 256 | after | 4,000 /18,464,000 |

Cola de jerarquía reservada una vez y bitset de joints: cuatro asignaciones
por validación; no se copian ni ordenan los IDs de joints. La salida pública
puede reutilizar su Array, aunque este control de validación usa salida local.

## Control integrado normal

Antes: serie archivada normal del 116, mismo código base y contenido; no se
presenta como una repetición nueva del 116. Después: siete pasadas seriales del
avance 117. 1920×1080, Immediate, 120 frames calientes y 360 medidos; Factory
tres veces, Hound v17/LOD0 dos con audio nulo y dos con dispositivo real.
No se recocina ni cambia arte. Runtime allocation profile OFF en ambas series.

| Versión /pasada | Media ms | p50 | p95 | p99 | Máximo | >5 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 116 factory-1 | 2.335 | 2.336 | 2.351 | 2.358 | 2.368 | 0 /360 |
| 116 factory-2 | 2.341 | 2.340 | 2.360 | 2.370 | 2.486 | 0 /360 |
| 116 factory-3 | 2.350 | 2.347 | 2.389 | 2.451 | 2.504 | 0 /360 |
| 116 hound-null-1 | 3.280 | 3.275 | 3.511 | 3.520 | 3.546 | 0 /360 |
| 116 hound-null-2 | 3.282 | 3.275 | 3.509 | 3.525 | 3.533 | 0 /360 |
| 116 hound-audio-1 | 3.285 | 3.279 | 3.511 | 3.534 | 3.647 | 0 /360 |
| 116 hound-audio-2 | 3.280 | 3.276 | 3.507 | 3.524 | 3.527 | 0 /360 |
| 117 parcial factory-1 | 2.330 | 2.329 | 2.349 | 2.359 | 2.482 | 0 /360 |
| 117 parcial factory-2 | 2.335 | 2.334 | 2.353 | 2.367 | 2.393 | 0 /360 |
| 117 parcial factory-3 | 2.349 | 2.347 | 2.383 | 2.399 | 2.457 | 0 /360 |
| 117 parcial hound-null-1 | 3.279 | 3.272 | 3.507 | 3.519 | 3.528 | 0 /360 |
| 117 parcial hound-null-2 | 3.284 | 3.269 | 3.505 | 3.517 | 3.525 | 0 /360 |
| 117 parcial hound-audio-1 | 3.278 | 3.271 | 3.506 | 3.519 | 3.539 | 0 /360 |
| 117 parcial hound-audio-2 | 3.278 | 3.271 | 3.506 | 3.518 | 3.665 | 0 /360 |

Factory medias 2,330/2,335/2,349 ms; Hound 3,279/3,284/3,278/3,278 ms.
Sin regresión integrada atribuible ni ganancia global de FPS atribuida.
Hound conserva p99<4 ms, máximo<5 ms y cero frames >5 ms, 345,26 MiB, BC5/BC7,
mask127/weapon31, 53 skinned, peak_maps9 y cero missing/evictions/budget_limited
medidos. El contador total conserva frames limitados durante carga inicial.
Esto no recertifica ocho jugadores humanos ni sustituye H06.

## Reproducción y límites

Builds completos Release/Debug y formato pasan. Los siete CTest focalizados
pasan en Release (7,03 s) y Debug (53,46 s), incluidos los cuatro hijos Debug
que verifican asserts de catálogo. No se repite la suite completa por rutina;
sus últimos resultados completos corresponden al 116, no a este avance.

```powershell
rtk proxy cmake --build --preset windows-release --parallel 6
rtk proxy cmake --build --preset windows-debug --parallel 6
rtk proxy cmake --build --preset windows-release --target gloom_format_check
rtk proxy ctest --preset windows-release -R "^gloom\.(assets|skin_bounds|animation_vfx|factory_restoration|character_restoration|legacy_arsenal|hound_runtime)$" --output-on-failure
rtk proxy ctest --preset windows-debug -R "^gloom\.(assets|skin_bounds|animation_vfx|factory_restoration|character_restoration|legacy_arsenal|hound_runtime)$" --output-on-failure
rtk proxy python -X utf8 tools/perf/measure_cpp_baseline.py --output .cache/hito117/repeat-normal
```

Los controles históricos/actuales, logs y JSON están en .cache/hito117; solo
estas tablas Markdown se versionan. El probe conserva /EHsc porque ambos asset.cpp
incluyen todavía el VFS/codec legacy. No acredita flags de cierre del 117.
Cocinado/importación end-to-end, Unicode/VFS nativo y toda la matriz de formatos
históricos siguen pendientes para la entrega final. Fuentes/recursos versionados
en assets, art y assets-source no tienen diferencias contra HEAD.

Fuentes del control contrastadas con el workspace:

| Archivo | SHA-256 |
| --- | --- |
| include/gloom/assets/asset.hpp | `6ed92a71a391767632ff8ba58585ac76425885fb0d724c822faaf09ef7c41098` |
| src/assets/asset.cpp | `95a2320546b36b05f1478a5f54c12e825ac2fa75dfdfab3965788b4961387354` |
| include/gloom/assets/rig.hpp | `447ff24e2c602eb7a22bae3e363de81d35a7fc2fe381d2527e302c28f8377512` |
