# Plan de optimización y migración C++ de Gloom

Formalizado en el **hito 111**, 3 de octubre de 2026. Código investigado: `5b96a91`
(110). **14 hitos pendientes, 117–130; 112–116 terminados**, con fichas, dependencias, contratos,
medidas y pruebas. Base 111: 188 archivos/50 CTest; actual 116: 198 archivos/54 CTest.
El 128 revisa todo el código además de su prueba de captura; 129/130 son
transversales y no tienen archivos propietarios nuevos.

Para empezar: leer inicio de [ESTADO_ACTUAL](../../ESTADO_ACTUAL.md),
[AGENTS](../../../AGENTS.md), [CONTEXTO](CONTEXTO.md) y **solo la ficha del hito**.
El siguiente es **117**, no iniciado; su cierre mantiene el bloqueo fastgltf.
El [116](../../../reports/cpp-performance-116/README.md) cierra el registro nativo,
punteros estables y reutilización sin asignaciones, con medidas CPU/memoria. El [115](../../../reports/cpp-performance-115/README.md)
cierra partículas/efectos con salida persistente, cero asignaciones por frame
y comparación numérica/visual antes-después. El [114](../../../reports/cpp-performance-114/README.md)
cierra scheduler/capturas con trabajos nativos, grupos estables y cero asignaciones
por frame en esa ruta. El [113](../../../reports/cpp-performance-113/README.md)
cierra poses persistentes con medidas y validación. El [112](../../../reports/cpp-performance-112/README.md)
añade medición/contratos y probe de compatibilidad; no promete mejora de FPS.
Renderer y contenido conservados. Decisión externa pendiente en 117/120/128.

| Hito | Resultado previsto | Dependencias | Estado | Archivos propietarios |
| --- | --- | --- | --- | ---: |
| 112 | [Base reproducible, costes y contratos de propiedad](112-base-y-contratos.md) | 111 | Terminado | 12 |
| 113 | [Poses y animación sin temporales por frame](113-poses.md) | 112 | Terminado | 11 |
| 114 | [JobSystem y FixedFunction con grupos reutilizables](114-jobs.md) | 112 y 113, por secuencia del plan | Terminado | 4 |
| 115 | [Partículas y efectos con salida persistente](115-particulas.md) | 113 y 114 | Terminado | 5 |
| 116 | [EntityRegistry sin mapas ni RTTI y con direcciones seguras](116-entidades.md) | 112 y 115 por secuencia | Terminado | 3 |
| 117 | [Datos de assets, VFS, importación y cooker](117-assets-y-cooker.md) | 113, 114 y 116 | No iniciado; cierre bloqueado | 19 |
| 118 | [Carga asíncrona y residencia con propiedad explícita](118-carga-y-residencia.md) | 114 y 117 | No iniciado | 6 |
| 119 | [Datos CPU del renderer y retiro seguro de recursos GPU](119-renderer-y-lifetime.md) | 113, 117 y 118 | No iniciado | 21 |
| 120 | [Física y consultas con eventos reutilizables](120-fisica.md) | 116 y 119; decisión de compatibilidad Jolt de 112 resuelta | No iniciado; cierre bloqueado | 6 |
| 121 | [Audio sin propietarios compartidos ni temporales de mezcla](121-audio.md) | 114 y 117 | No iniciado | 12 |
| 122 | [Red, replicación y gameplay con buffers acotados](122-red-y-simulacion.md) | 114, 116 y 120 | No iniciado | 54 |
| 123 | [Directorio de partidas y navegador asíncrono](123-directorio.md) | 114, 117 y 122 | No iniciado | 5 |
| 124 | [Persistencia durable de partidas con datos propios](124-persistencia.md) | 117 y 123 | No iniciado | 1 |
| 125 | [Identidad, tickets y servicios HTTP/HTTPS](125-identidad-y-http.md) | 123 y 124 | No iniciado | 18 |
| 126 | [UI y presentación de menús con buffers reutilizados](126-ui.md) | 119, 121, 123 y 125 | No iniciado | 6 |
| 127 | [Composición del runtime, arranque y aplicaciones](127-runtime-y-apps.md) | 113–126 terminados | No iniciado | 14 |
| 128 | [Cumplimiento global verificable y flags de compilación](128-cierre-cpp.md) | 113–127; ninguna frontera externa pendiente | No iniciado; cierre bloqueado | 1 |
| 129 | [Perfil GPU del renderer actual y optimización justificada](129-perfil-gpu.md) | 119 y 128 | No iniciado | 0 |
| 130 | [Validación integrada y entrega del plan ejecutado](130-validacion-integrada.md) | 112–129 cerrados; bloqueos explícitos resueltos | No iniciado | 0 |

El orden por número es el camino predeterminado. Si un hito está bloqueado,
se puede trabajar en otro únicamente cuando todas sus dependencias estén cerradas;
no marcar cumplida una dependencia por estar planificada. Cada ficha contiene
un encargo copiable para ejecutarse paso a paso. No iniciar el siguiente sin encargo.

El 110 ya terminó buffers de visibilidad/iluminación; 119 cierra tipos y ownership
restantes de esas APIs, sin repetir su optimización. El 111 entrega este plan y su
[informe](../../../reports/cpp-performance-111/README.md). El inventario es de código
propio: [lista completa y señales](INVENTARIO.md), no una estimación por carpetas.

## Qué queda firme y qué necesita datos

Los ámbitos, fuentes, invariantes, pruebas y condiciones de cierre quedan definidos.
El 112 mide capturas y reservas y propone inline 80/8 con payloads de carga en
contextos estables; la implementación y saturación se verifican en 114.
El candidato GPU sigue pendiente de medidas en 129. La elección de almacenamiento
de componentes exige comprobar todos los punteros conservados antes de implementarse.
Cada decisión se registra en el informe de su hito con medida y alternativa descartada.

El [probe 112](../../../reports/cpp-performance-112/compatibilidad.md) confirma
filtros Jolt incorporados y polling simple, sin equivalencia completa de listeners.
Los listeners Jolt y el filesystem de fastgltf chocan con las reglas literales:
117/120/128 tienen cierre bloqueado hasta decisión expresa o alternativa equivalente.
No hay excepción autorizada ni cambio de dependencia previsto.

La migración de estilo/dependencias no se presenta como ahorro de FPS. Los hitos
de rendimiento necesitan antes/después; si GPU domina o el ahorro queda en el ruido,
se informa así. No se planea escribir una stdlib completa, añadir Boost/EASTL,
reemplazar Diligent/Vulkan ni regenerar arte. Una mejora nueva que salga al medir
se formaliza en otra ficha antes de ampliar la implementación.
