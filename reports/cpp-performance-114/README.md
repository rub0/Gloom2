# Hito 114: trabajos nativos, grupos estables y capturas inline

4 de octubre de 2026. Entrada `8b96c71e2f7975d9ef0b1e42e2f21edbb667c825` (113).
112 y 113 subidos a origin/main por encargo; cierre 114 local. Ponytail full.
Renderer Diligent/Vulkan, shaders, recursos, protocolo y referencias conservados.

## Resultado y alcance

JobSystem deja de heredar Subsystem y elimina PIMPL, shared_ptr, deque,
jthread, mutex/condition_variable STL y la propagación de excepciones. Las
aplicaciones lo poseen directamente, comprueban start y drenan antes de cerrar
los demás sistemas. No se añade un adapter ni una dependencia.

FixedFunction implementa únicamente void(): 80 bytes inline, alineación 8,
movimiento y destrucción explícitos, sin copia ni fallback al heap. Rechaza
en compilación tamaño/alineación y movimiento, destrucción o llamada que puedan
lanzar. El objeto completo mide 104 bytes; el WorkItem, 112. Los contextos grandes
se crean explícitamente en la carga y transfieren su puntero al trabajo: eso
es propiedad de una solicitud, no un fallback de FixedFunction.

Los grupos viven en sus llamadores, no se asignan ni tienen un pool limitado.
Su dirección permanece estable; no se copian/mueven. Propietario y generación
de arranque rechazan otro JobSystem o una sesión anterior. El contador pendiente
se protege por el mismo SRWLOCK que las colas, la finalización y las esperas.
Destruir un grupo activo y esperar en un grupo ancestro son errores de programación.

Cola circular reservada de 256 entradas (28 KiB), frente al pico de arranque 102
medido en 112. Si se llena, el productor ejecuta trabajos encolados y vuelve a
intentar publicar: no crece, no descarta y no establece un límite funcional nuevo.
La prueba fuerza capacidades 16 y 1. Se mantienen asistencia y espera anidada
con un worker. Hay dos condiciones sobre un único lock: nuevos trabajos despiertan
un worker; publicación/finalización despierta a los llamadores que esperan. No hay
espera activa ni retirada de la sincronización necesaria.

## Propiedad y contratos

| Elemento | Propietario y final de vida |
| --- | --- |
| Grupo | Objeto estable del llamador; esperar/drain antes de destruir o reconstruir. Reiniciar el sistema invalida grupos anteriores. |
| Función inline | Job, transferido por movimiento a la cola/ejecutor; destrucción antes de publicar pending=0. |
| Contexto de carga | Job posee copia estable del registro y el productor del resultado. Loader, VFS y catálogo viven hasta wait; nunca se retiene un iterador de caché. |
| Preparación | Job posee escena/dependencias y productor del resultado. No observa Request, renderer ni su resident. Cancel/reload descarta el futuro; la generación abandonada puede terminar sin publicarse en Request. |
| parallel_for | Functor nombrado y sus spans pertenecen al llamador y viven hasta wait. No acepta temporales ni crea una función compartida, incluso con rango vacío. |
| Visibilidad | Frustum, entradas e índices viven hasta wait; rangos disjuntos, compactación/orden posterior determinista. |
| Cierre | Productores externos unidos antes de wait/stop. stop admite descendientes de trabajos en ejecución, drena y une todos los handles. start parcial fallido también los une. |
| GPU | Los jobs preparan datos CPU. Conservan la política de uploads, fences y destrucción existente; ninguna migración del renderer. |

Se adaptan todos los sitios: visibilidad, carga, preparación de residencia y ambos
parallel_for de main. Las dos funciones de main pasan a functors con Span por
valor y espera antes de consumir o reconstruir sus datos. El loader libera su
bloqueo de caché antes de schedule: la asistencia por saturación podría ejecutar
otro loader que necesita ese mismo bloqueo para publicar su resultado.

**El cierre cubre el scheduler y sus capturas, no los formatos legacy de assets.**
ImportedScene, CookedAsset, GpuSceneUploads, bind_rig y los resultados/futuros
existentes conservan su representación de 117/118. No se inventa otro futuro
provisional ni se amplía FixedFunction para alojar escenas. Los contextos mueven
los mismos payloads que antes capturaban los jobs. Sus fallos de archivos, decode,
texturas y jerarquía se publican como resultados; los try/catch se retiran.
Las excepciones/RTTI están desactivadas en scheduler, prueba propia y los dos
consumidores de carga, en ambos builds. MSVC /wd4530 se limita a estos consumidores
con headers legacy (como ya sucede en skin_bounds); no certifica esos headers.
El ticket de state debe proceder del coordinador y se comprueba con assert.

## Medidas y validación

[Medidas y reproducción](medidas.md). Diagnóstico de frames: jobs **3 → 0
new/frame Factory** y **4 → 0 Hound**, **344/416 → 0 bytes solicitados/frame**.
No son todos los allocators del proceso ni RAM viva. El ahorro de buffers del
110 ya está presente en ambos lados; se separa del scheduler del 114.

La prueba autónoma gloom.jobs siempre enlaza su contador de asignaciones,
independientemente del diagnóstico del juego. Mil rondas con grupo nuevo y grupo
reutilizado, 6.000 trabajos, dan **0 new/new[] y 0 bytes** con uno y cuatro workers.
El control histórico de 1.000 grupos nuevos/3.000 jobs registra 1.000 llamadas,
200.000 bytes; el control nativo, cero. Los tiempos usan ejecutables sin diagnóstico.

Builds completos Release/Debug y formato correctos. Debug **50/53**, Release
**49/53**: mismos tres comparadores visuales heredados del 113, y GNS send result
25 únicamente en Release. Jobs, assets, visibilidad, Vulkan y Hound pasan en ambos.
Las siete vistas estáticas alineadas antes/después pasan el comparador original;
en 1280×720, diferencia máxima por canal 1/255. No se modifican referencias o
tolerancias. El control no sustituye los comparadores heredados que siguen fallando.

La serie normal no demuestra mejora global de FPS. Hound conserva p99 <4 ms y
345,26 MiB, pero registra picos >5 ms en ambas versiones: 1/1.440 frames antes,
3/1.440 después. No se certifica de nuevo H06; se conservan todas las pasadas.
Lotes aislados, mediana de siete pasadas: 0,7 → 0,4 µs con un worker y 4,7 →
4,0 µs con cuatro; la espera acumulada con cuatro aumenta y se documenta.

Comprobaciones adicionales: ejecución exacta por índice con cuatro productores;
capturas movibles y destrucción antes de wait; capacidad inline exacta 80/8;
errores explícitos sin perder otros trabajos; gates que fuerzan saturación;
cancelación y reload mientras la preparación está encolada; stop con descendientes
pendientes y reinicio. Debug comprueba asserts de grupo extranjero, generación
obsoleta y espera propia en procesos aislados, sin mostrar diálogos.
Un probe compilado con la implementación real sustituye solamente CreateThread:
falla el segundo arranque con error 1450, une el primer worker y después completa
64/64 trabajos tras reiniciar. No se introduce un hook en producción.
Cinco probes de compilación rechazan 81 bytes, alineación 16 y call/move/destructor
que puedan lanzar; 80/8 se acepta. Tests, builds y evidencias en medidas.

No se inicia 115 ni se sube el nuevo cierre automáticamente. Los bloqueos
fastgltf/Jolt de 117/120/128 siguen pendientes de decisión expresa.
