# Medidas del registro de entidades, hito 116

5 de octubre de 2026. Base `2b98e83` (131 solo setup Hound; código C++ 115).
Ryzen 7 3700X / GTX 1070, driver 581.29, MSVC 14.44, Release x64.
Normal y diagnóstico separados, sin builds/pruebas simultáneos durante medidas.
Misma composición real de doce componentes; payload total 735 bytes por entidad
en ambas versiones. No cambian gameplay, protocolo 22, renderer ni recursos.

## CPU del registro, diagnóstico OFF

Driver idéntico recompila entity.cpp y los headers históricos del 115, frente al
actual. Cinco pasadas alternadas. Cada una: 1.000 lotes de consulta y 1.000 ciclos
de crear/componer/destruir a 8, 64 y 1.024 entidades, después de calentamiento.
get4 adquiere transform, authority, health y weapon y lee sus valores; no es el
tiempo aislado de una llamada get. lifecycle compone doce componentes y destruye
la entidad; se incluyen sus constructores/destructores. QPC, resolución 100 ns
por lote. Los valores siguientes son ns por entidad, mediana / p95 de cada pasada.

| Entidades | Etapa | Pasada | Antes mediana / p95 | Después mediana / p95 |
| ---: | --- | ---: | ---: | ---: |
| 8 | get4 | 1 | 137.5 / 137.5 | 12.5 / 12.5 |
| 8 | get4 | 2 | 137.5 / 137.5 | 12.5 / 12.5 |
| 8 | get4 | 3 | 137.5 / 137.5 | 12.5 / 12.5 |
| 8 | get4 | 4 | 137.5 / 137.5 | 12.5 / 12.5 |
| 8 | get4 | 5 | 137.5 / 137.5 | 12.5 / 12.5 |
| 8 | lifecycle | 1 | 1387.5 / 1500 | 112.5 / 125 |
| 8 | lifecycle | 2 | 1375 / 1500 | 112.5 / 125 |
| 8 | lifecycle | 3 | 1825 / 2125 | 112.5 / 125 |
| 8 | lifecycle | 4 | 1337.5 / 1637.5 | 125 / 125 |
| 8 | lifecycle | 5 | 1375 / 1525 | 125 / 125 |
| 64 | get4 | 1 | 135.938 / 137.5 | 9.375 / 9.375 |
| 64 | get4 | 2 | 134.375 / 135.938 | 9.375 / 9.375 |
| 64 | get4 | 3 | 179.688 / 187.5 | 9.375 / 9.375 |
| 64 | get4 | 4 | 135.938 / 137.5 | 9.375 / 9.375 |
| 64 | get4 | 5 | 135.938 / 137.5 | 9.375 / 9.375 |
| 64 | lifecycle | 1 | 1906.25 / 2056.25 | 115.625 / 143.75 |
| 64 | lifecycle | 2 | 1864.06 / 2032.81 | 117.188 / 148.438 |
| 64 | lifecycle | 3 | 1875 / 2595.31 | 115.625 / 143.75 |
| 64 | lifecycle | 4 | 1839.06 / 2021.88 | 115.625 / 117.188 |
| 64 | lifecycle | 5 | 1853.12 / 2034.38 | 114.062 / 115.625 |
| 1024 | get4 | 1 | 153.418 / 159.18 | 8.496 / 8.594 |
| 1024 | get4 | 2 | 153.32 / 159.375 | 8.594 / 8.691 |
| 1024 | get4 | 3 | 153.125 / 166.797 | 8.594 / 9.375 |
| 1024 | get4 | 4 | 152.051 / 163.086 | 8.594 / 8.691 |
| 1024 | get4 | 5 | 152.344 / 163.965 | 8.594 / 8.594 |
| 1024 | lifecycle | 1 | 1425.1 / 1885.16 | 113.965 / 121.875 |
| 1024 | lifecycle | 2 | 1446.68 / 1899.9 | 114.355 / 120.215 |
| 1024 | lifecycle | 3 | 1448.44 / 1949.12 | 113.867 / 122.559 |
| 1024 | lifecycle | 4 | 1435.06 / 1909.18 | 113.184 / 119.629 |
| 1024 | lifecycle | 5 | 1433.69 / 1899.81 | 112.695 / 117.871 |

Resumen de medianas entre las cinco pasadas y ruido `(max-min)/mediana`:

| Entidades | Etapa | Antes ns | Después ns | Ruido antes % | Ruido después % |
| ---: | --- | ---: | ---: | ---: | ---: |
| 8 | get4 | 137.500 | 12.500 | 0.00 | 0.00 |
| 8 | lifecycle | 1375.000 | 112.500 | 35.45 | 11.11 |
| 64 | get4 | 135.938 | 9.375 | 33.33 | 0.00 |
| 64 | lifecycle | 1864.062 | 115.625 | 3.60 | 2.70 |
| 1024 | get4 | 153.125 | 8.594 | 0.89 | 1.14 |
| 1024 | lifecycle | 1435.059 | 113.867 | 1.63 | 1.46 |

Las mejoras superan el ruido y la resolución de lote en esta carga. A 1.024:
get4 153,125 → 8,594 ns por entidad; lifecycle 1.435,059 → 113,867 ns.
Las consultas conservan checksum **658.436.000** en todas las pasadas/versiones.
La cuantización es visible con ocho entidades; no se extrapola esta microcarga
a FPS. La nueva ruta evita búsquedas de tipos/nodos y construye en slots ya
reservados; no se añaden SIMD, arenas, arquetipos ni un registro global de tipos.

## Asignaciones y memoria, diagnóstico separado

Contador de ordinary new/new[], excluye aligned new, DLL y C directo. Probe
temporal añade seguimiento de delete para sumar bytes solicitados que siguen
vivos; su cabecera interna y overhead del allocator quedan excluidos. Solo se
usa en las medidas de memoria/asignaciones, nunca en la serie CPU anterior.
Es mono-thread; no se presenta como RSS, memoria GPU o heap global del juego.
Los tipos de la composición real tienen alineación ≤8. La prueba permanente
verifica por separado un componente alignas(64), sin afirmar que el contador
ordinary new cubra su asignación alineada.

| Entidades | Versión | cold llamadas / bytes solicitados | get4 llamadas / bytes | 1.000 lifecycle llamadas / bytes |
| ---: | --- | ---: | ---: | ---: |
| 8 | before | 153 / 13,168 | 0 / 0 | 96,000 / 8,000,000 |
| 64 | before | 843 / 83,528 | 0 / 0 | 768,000 / 64,000,000 |
| 1024 | before | 12,393 / 1,362,806 | 0 / 0 | 12,288,000 / 1,024,000,000 |
| 8 | after | 26 / 6,136 | 0 / 0 | 0 / 0 |
| 64 | after | 146 / 49,760 | 0 / 0 | 0 / 0 |
| 1024 | after | 1,634 / 797,600 | 0 / 0 | 0 / 0 |

El ahorro de nodos durante reutilización es completo: 12 asignaciones antiguas
por entidad/ciclo → cero. Cold load/páginas sí asignan. El test permanente
gloom.entities comprueba mil ciclos alternando destroy y clear con la misma
composición a las tres capacidades: cero llamadas/bytes en todas las fases,
Release y Debug. Una primera página fría activa el control positivo de crecimiento.

Memoria: heap vivo medido por el probe, más sizeof del registro como columna
separada. La comparación llena usa cold lleno; peak incluye free list que el
registro viejo reserva posteriormente. Ambos mantienen sus buffers de índices.

| Entidades | Versión | sizeof registro | Heap lleno | Heap vacío tras clear | Peak heap | Total lleno por entidad |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 8 | before | 120 | 12,912 | 4,944 | 12,944 | 1629.000 |
| 8 | after | 920 | 6,136 | 6,136 | 6,136 | 882.000 |
| 64 | before | 120 | 80,344 | 16,600 | 80,600 | 1257.250 |
| 64 | after | 920 | 49,088 | 49,088 | 49,088 | 781.375 |
| 1024 | before | 120 | 1,232,947 | 213,082 | 1,237,082 | 1204.167 |
| 1024 | after | 920 | 785,408 | 785,408 | 785,408 | 767.898 |

Con 1.024 entidades llenas: 1.233.067 → 786.328 bytes, incluyendo registro,
**-36,23 %**. Objeto registro 120 → 920 bytes por la tabla inline de 18 tipos.
El coste por entidad de slots+free list es 12 bytes, tabla de páginas 12 bytes
por composición y datos de componentes 735; máscara/padding añaden 8 bytes
por entidad en páginas completas. La última página puede desperdiciar hasta
siete slots por tipo. Solo se asigna una página cuando se usa, incluso con
índices dispersos; la tabla sí cubre el índice máximo de ese tipo.

Tradeoff explícito: clear destruye todos los objetos/recursos pero conserva
las páginas. El heap vacío nuevo es mayor que el viejo (785.408 vs 213.082 con
1.024), para que recomponer no asigne. Se recupera al destruir el registro.
No se añade un trim especulativo ni se promete memoria cero tras clear.

## Serie integrada normal

Tres Factory y cuatro Hound v17/LOD0 (dos audio nulo/dos real), seriales.
1920×1080 nativo, Immediate, 120 frames calientes y 360 medidos por pasada.
No se miden durante builds, tests u otra app gráfica de esta tarea.

| Versión / pasada | Media ms | FPS | p50 | p95 | p99 | Máximo | >5 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 115 factory-1 | 2.344 | 426.63 | 2.341 | 2.361 | 2.389 | 3.632 | 0/360 |
| 115 factory-2 | 2.345 | 426.37 | 2.345 | 2.363 | 2.381 | 2.395 | 0/360 |
| 115 factory-3 | 2.350 | 425.55 | 2.349 | 2.369 | 2.396 | 2.413 | 0/360 |
| 115 hound-null-1 | 3.222 | 310.35 | 3.269 | 3.514 | 3.563 | 3.858 | 0/360 |
| 115 hound-null-2 | 3.282 | 304.67 | 3.275 | 3.507 | 3.522 | 3.659 | 0/360 |
| 115 hound-audio-1 | 3.286 | 304.34 | 3.278 | 3.515 | 3.530 | 3.541 | 0/360 |
| 115 hound-audio-2 | 3.283 | 304.58 | 3.277 | 3.512 | 3.529 | 3.567 | 0/360 |
| 116 factory-1 | 2.335 | 428.20 | 2.336 | 2.351 | 2.358 | 2.368 | 0/360 |
| 116 factory-2 | 2.341 | 427.16 | 2.340 | 2.360 | 2.370 | 2.486 | 0/360 |
| 116 factory-3 | 2.350 | 425.47 | 2.347 | 2.389 | 2.451 | 2.504 | 0/360 |
| 116 hound-null-1 | 3.280 | 304.86 | 3.275 | 3.511 | 3.520 | 3.546 | 0/360 |
| 116 hound-null-2 | 3.282 | 304.70 | 3.275 | 3.509 | 3.525 | 3.533 | 0/360 |
| 116 hound-audio-1 | 3.285 | 304.37 | 3.279 | 3.511 | 3.534 | 3.647 | 0/360 |
| 116 hound-audio-2 | 3.280 | 304.88 | 3.276 | 3.507 | 3.524 | 3.527 | 0/360 |

Sin regresión integrada atribuible ni mejora global de FPS atribuida al registro:
las diferencias de frame quedan dentro de dispersión/resolución entre pasadas.
Hound: p99<4 ms, máximo<5 ms, 0/1.440 >5 ms en ambas series; 345,26 MiB,
BC5/BC7, TPS mask127/weapon31, 53 skinned visibles y peak_maps9. Reservados/copias
442.368/81.408 bytes/frame; cero missing, evicción y budget_limited_frames medidos.
Presupuesto runtime512 MiB conservado. No recertifica ocho jugadores humanos
ni borra los picos históricos del 114. Factory y Hound mantienen los controles
de contenido de la herramienta. Detalle de etapas y memoria de proceso:

| Versión / pasada | update | presentation | visibility | lighting | draw | end/present | poses | bounds | GPU media | private MiB | working set MiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 115 factory-1 | 0.140 | 0.105 | 0.026 | 0.018 | 0.455 | 1.577 | 0.075 | 0.003 | 2.050 | 1093.18 | 571.28 |
| 115 factory-2 | 0.142 | 0.104 | 0.026 | 0.018 | 0.454 | 1.583 | 0.074 | 0.003 | 2.055 | 1093.11 | 571.74 |
| 115 factory-3 | 0.146 | 0.106 | 0.025 | 0.019 | 0.456 | 1.579 | 0.076 | 0.004 | 2.058 | 1093.21 | 571.52 |
| 115 hound-null-1 | 0.252 | 0.594 | 0.033 | 0.018 | 0.733 | 1.570 | 0.513 | 0.051 | 2.922 | 1810.12 | 997.08 |
| 115 hound-null-2 | 0.239 | 0.588 | 0.031 | 0.018 | 0.710 | 1.675 | 0.507 | 0.052 | 2.988 | 1771.35 | 993.89 |
| 115 hound-audio-1 | 0.230 | 0.566 | 0.029 | 0.017 | 0.677 | 1.748 | 0.489 | 0.052 | 2.989 | 1776.17 | 994.65 |
| 115 hound-audio-2 | 0.227 | 0.565 | 0.029 | 0.017 | 0.684 | 1.741 | 0.488 | 0.051 | 2.985 | 1775.75 | 995.04 |
| 116 factory-1 | 0.139 | 0.105 | 0.026 | 0.018 | 0.451 | 1.579 | 0.075 | 0.003 | 2.045 | 1094.49 | 571.22 |
| 116 factory-2 | 0.140 | 0.106 | 0.026 | 0.018 | 0.453 | 1.580 | 0.076 | 0.004 | 2.050 | 1090.92 | 570.33 |
| 116 factory-3 | 0.161 | 0.113 | 0.025 | 0.019 | 0.481 | 1.528 | 0.081 | 0.004 | 2.052 | 1091.14 | 570.51 |
| 116 hound-null-1 | 0.234 | 0.587 | 0.031 | 0.019 | 0.710 | 1.679 | 0.506 | 0.052 | 2.987 | 1775.34 | 994.11 |
| 116 hound-null-2 | 0.236 | 0.599 | 0.036 | 0.019 | 0.715 | 1.657 | 0.511 | 0.053 | 2.988 | 1777.15 | 996.80 |
| 116 hound-audio-1 | 0.226 | 0.565 | 0.029 | 0.018 | 0.682 | 1.747 | 0.488 | 0.052 | 2.989 | 1771.71 | 995.12 |
| 116 hound-audio-2 | 0.222 | 0.564 | 0.030 | 0.018 | 0.682 | 1.746 | 0.485 | 0.052 | 2.986 | 1773.66 | 996.19 |

## Validación y reproducción

Builds completos Release/Debug y gloom_format_check correctos. Prueba propia:
crecimiento de 2.050 entidades y 1.024 composiciones, doce caches inmóviles,
alignas(64), componente no copiable/no movible/no default constructible,
const get, remove idempotente, destroy/recreate, clear repetido, teardown y
destrucción exactamente una vez. Snapshot de 1.024 tras crecer: mismo movimiento,
autoridad server preservada y aplicación de remoto correcta. Los asserts de
duplicado/handle caducado se ejecutan en hijos Debug, sin abort manual en código.
Los cuerpos físicos y la simulación/replicación reales se verifican en CTest.

CTest completo: Release **50/54, 107,93 s**; Debug **51/54, 257,49 s**.
Los tres fallos visuales heredados son ui_visual_review, factory_visual_review
y character_visual_review. animation_network conserva el fallo GNS send 25
solo Release. El resto pasa, incluidos entidades, cuerpos físicos, simulación,
replicación, combate, movimiento, smoke/Vulkan y Hound. Se comparan causas,
no solo totales; no se modifican referencias o umbrales para cerrar el hito.

Captura animada de 1.200 frames a 30 Hz/640×360 contra el archivo del 115:
todos los frames y los 172 grupos pasan el comparador C++ original (miniaturas
160×90, media ≤4/255, píxeles distintos ≤1,5 %, peor tile ≤12/255).
Comparación adicional a resolución completa: máximo de canal **1/255**, media
RGB máxima **0,001444/255**, máximo de píxeles con algún canal distinto
**0,429688 %**. Trece pares de frames inspeccionados visualmente, con armas,
destellos, impactos y muerte; misma salida de eventos/partículas:
41 /5.410 creadas /4.746 expiradas /0 descartadas.
El capturador temporal alinea fase TAA/frame (832 frente a 768, ambos módulo
ocho cero) después de calentar; no modifica runtime, referencias ni umbrales.
Este control de equivalencia no declara arreglados los tres CTest visuales.

Registro y composición migrados no usan directamente headers STL ni std::,
mapas, RTTI, pool virtual, auto ordinario o excepciones. Flags efectivos /GR- /EHs-c-
en entity.cpp y entity_tests.cpp, ambas configuraciones. /wd4530 se limita al
test consumidor de headers legacy y /wd4324 a su fixture de alineación explícita.
LegacyArsenal, World/Subsystem y WorldSnapshot conservan representación legacy
de 120/122/127. Capturar WorldSnapshot aún asigna su vector: fuera de la ventana
de registro, pendiente de 122. Los adapters físicos solo reciben IDs en 116;
su cierre y errores del backend siguen en 120. No certifica cumplimiento global.

Prueba permanente, sin framework nuevo:

```powershell
rtk proxy cmake --preset windows -DGLOOM_ALLOCATION_PROFILE=OFF
rtk proxy cmake --build --preset windows-release --parallel 6
rtk proxy cmake --build --preset windows-debug --parallel 6
rtk proxy cmake --build --preset windows-release --target gloom_format_check
rtk proxy build/windows-vs/Release/gloom_entity_tests.exe
rtk proxy build/windows-vs/Debug/gloom_entity_tests.exe
rtk proxy ctest --preset windows-release --output-on-failure
rtk proxy ctest --preset windows-debug --output-on-failure
rtk proxy python -X utf8 tools/perf/measure_cpp_baseline.py --output .cache/hito116/repeat-normal
```

Logs, series, probe temporal control.cpp/measure_control.py y seguimiento de
memoria memory_new.cpp en .cache/hito116; excluidos del commit. Datos completos
arriba permanecen en Markdown. Solo la prueba existente se amplía; ningún target
CTest nuevo, 198 archivos propios/54 CTest. Diagnóstico de runtime OFF; el test
enlaza contador independiente. No push ni inicio del 117.

SHA-256 de ejecutables normales y fuentes medidas, contrastadas al cerrar:

| Elemento | SHA-256 |
| --- | --- |
| gloom.exe 115 normal | `41d2f4ac7ec89f05c04abb7d281f06ed261ca2c12dc8299e8aec2768bb233e2d` |
| gloom.exe 116 normal | `c6dbdc145e98eb2c4d4068f492900ef10713e1e9cf5ccfe1ef17525bc831d97a` |
| include/gloom/core/entity.hpp | `56cd045432d0e6a6d82203af85ecf56eeba03f7f400a1f7482c09cfce079d49f` |
| src/core/entity.cpp | `1500f38dfd072635218732d8ceddad6a4bf833a24f8fac6b118446ae6a55635a` |
| include/gloom/gameplay/components.hpp | `1a12666a0adeaa2c70b5cf91efe66d1fb389426d9c7668358c401f42ed39bce0` |
