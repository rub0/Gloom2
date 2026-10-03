# Plan de optimización y migración C++ de Gloom

Formalizado en el **hito 111**, 3 de octubre de 2026. Código investigado: `5b96a91`
(110). **19 hitos pendientes, 112–130**, con fichas, dependencias, contratos,
medidas y pruebas. 188 archivos C++ propios asignados; 50 pruebas CTest existentes.
El 128 revisa todo el código además de su prueba de captura; 129/130 son
transversales y no tienen archivos propietarios nuevos.

Para empezar: leer inicio de [ESTADO_ACTUAL](../../ESTADO_ACTUAL.md),
[AGENTS](../../../AGENTS.md), [CONTEXTO](CONTEXTO.md) y **solo la ficha del hito**.
El siguiente es **112**; este encargo ha investigado/formalizado el plan, no ha
ejecutado nuevas optimizaciones ni benchmarks. Renderer y contenido conservados.

| Hito | Resultado previsto | Dependencias | Estado | Archivos propietarios |
| --- | --- | --- | --- | ---: |
| 112 | [Base reproducible, costes y contratos de propiedad](112-base-y-contratos.md) | 111 | No iniciado | 7 |
| 113 | [Poses y animación sin temporales por frame](113-poses.md) | 112 | No iniciado | 8 |
| 114 | [JobSystem y FixedFunction con grupos reutilizables](114-jobs.md) | 112 y 113, por secuencia del plan | No iniciado | 3 |
| 115 | [Partículas y efectos con salida persistente](115-particulas.md) | 113 y 114 | No iniciado | 4 |
| 116 | [EntityRegistry sin mapas ni RTTI y con direcciones seguras](116-entidades.md) | 112 y 115 por secuencia | No iniciado | 3 |
| 117 | [Datos de assets, VFS, importación y cooker](117-assets-y-cooker.md) | 113, 114 y 116 | No iniciado | 19 |
| 118 | [Carga asíncrona y residencia con propiedad explícita](118-carga-y-residencia.md) | 114 y 117 | No iniciado | 6 |
| 119 | [Datos CPU del renderer y retiro seguro de recursos GPU](119-renderer-y-lifetime.md) | 113, 117 y 118 | No iniciado | 21 |
| 120 | [Física y consultas con eventos reutilizables](120-fisica.md) | 116 y 119; decisión de compatibilidad Jolt de 112 resuelta | No iniciado | 6 |
| 121 | [Audio sin propietarios compartidos ni temporales de mezcla](121-audio.md) | 114 y 117 | No iniciado | 12 |
| 122 | [Red, replicación y gameplay con buffers acotados](122-red-y-simulacion.md) | 114, 116 y 120 | No iniciado | 54 |
| 123 | [Directorio de partidas y navegador asíncrono](123-directorio.md) | 114, 117 y 122 | No iniciado | 5 |
| 124 | [Persistencia durable de partidas con datos propios](124-persistencia.md) | 117 y 123 | No iniciado | 1 |
| 125 | [Identidad, tickets y servicios HTTP/HTTPS](125-identidad-y-http.md) | 123 y 124 | No iniciado | 18 |
| 126 | [UI y presentación de menús con buffers reutilizados](126-ui.md) | 119, 121, 123 y 125 | No iniciado | 6 |
| 127 | [Composición del runtime, arranque y aplicaciones](127-runtime-y-apps.md) | 113–126 terminados | No iniciado | 14 |
| 128 | [Cumplimiento global verificable y flags de compilación](128-cierre-cpp.md) | 113–127; ninguna frontera externa pendiente | No iniciado | 1 |
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
El tamaño de FixedFunction, las reservas reales y el candidato GPU se deciden con
medidas en 112/129: fijarlos ahora sería especulación. La elección de almacenamiento
de componentes exige comprobar todos los punteros conservados antes de implementarse.
Cada decisión se registra en el informe de su hito con medida y alternativa descartada.

Existe una incompatibilidad comprobada entre callbacks de Jolt y la prohibición
literal de herencia propia; se trata en 112 y condiciona 120/128. No hay excepción
autorizada ni motor alternativo previsto. No prometer cierre literal sin resolverla.

La migración de estilo/dependencias no se presenta como ahorro de FPS. Los hitos
de rendimiento necesitan antes/después; si GPU domina o el ahorro queda en el ruido,
se informa así. No se planea escribir una stdlib completa, añadir Boost/EASTL,
reemplazar Diligent/Vulkan ni regenerar arte. Una mejora nueva que salga al medir
se formaliza en otra ficha antes de ampliar la implementación.
