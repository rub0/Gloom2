# Propiedad, caducidad y decisiones para los siguientes hitos

Inspección sobre `517b16c` y diagnóstico del 112. Estas son condiciones para la
migración; no se afirma que las implementaciones heredadas ya cumplan AGENTS.
Los nombres permiten localizar cada operación con `rtk rg` sin cargar todo el plan.

## Operaciones CPU y trabajos diferidos

| Operación actual | Propietario y publicación | Caducidad y reutilización | Responsable |
| --- | --- | --- | --- |
| `rest_pose`, `sample_animation`, `blend_poses` | Devuelven un LocalPose propietario; las entradas se leen durante la llamada. | No retienen las entradas. La salida vive hasta su destrucción; no devolver una vista sobre un scratch local. | 113 |
| `pose_worlds` | Devuelve matrices y crea parents/queue temporales a partir del rig y pose. | Rig/pose válidos hasta retornar. Precalcular jerarquía por generación de rig; matrices persistentes por actor. | 113 |
| `skin_pose` | Devuelve un propietario compartido con matrices/copias para una vinculación de skin. RenderInstance y snapshot anterior conservan ese propietario. | El anterior no se sobrescribe mientras interpolación, snapshots o renderer lo leen. Dos buffers solo si todos esos lectores ya terminaron; GPU tiene fence separado. | 113/119 |
| `skinned_bounds` | Lee primitive/SkinBounds y SkinPose; retorna esfera por valor. | Lectura síncrona, sin retención. SkinBounds se prepara al cargar; misma generación de rig/paleta. | 113 |
| `JobSystem::schedule` | Cola posee WorkItem, callable y estado de grupo compartido. El worker o el llamador ejecuta el callable. | Capturas por referencia deben vivir hasta `wait(group)`; publicar resultados antes de decrementar remaining. No reutilizar contexto con pendientes. | 114 |
| `JobSystem::parallel_for` | Posee una copia compartida del callable; cada bloque posee ese owner y sus begin/end. | No hay orden entre bloques. Rango disjunto de escritura; entradas inmutables; wait precede a lectura/reasignación. `wait` puede ayudar ejecutando trabajos de otro grupo. | 114 |
| `VisibilitySystem::build` | Llamador posee PreparedVisibility; builder posee índices/JobOutput. CullJob apunta a frustum local y buffers del builder, y observa input. | `jobs.wait` termina antes de compactar, ordenar y retornar. Input no debe aliasar salida; no builds simultáneos sobre el mismo builder. Snapshot caduca al reconstruir/destruir su PreparedVisibility. | 119 |
| `ClusteredLightingBuilder::build` | Llamador posee PreparedLighting; builder posee LightBounds. view observa arrays de la salida y conserva la sonda heredada. | Síncrona; input no aliasa point_lights de salida. View caduca al rebuild/destrucción. No builds concurrentes sobre el mismo builder. | 119 |
| `ParticleSystem::advance/render` | Sistema posee recetas/partículas/emisores; render devuelve vector propietario nuevo cada frame. | Render lee materiales/cámara durante la llamada. En 115 salida del llamador; consumirla antes de siguiente render/rebuild. Conservar orden cronológico de nacimientos y decisiones de saturación. | 115 |
| `EntityRegistry::emplace/get` | Pool por tipo posee nodos de mapa; get devuelve puntero prestado. | Rehash del unordered_map no mueve sus elementos, pero remove/destroy/clear sí los invalidan. Mover a Array denso invalidaría caches: usar páginas estables o sustituir **todos** los caches por handles antes. | 116 |
| `EntityRegistry::destroy` | Retira todos los componentes del slot; incrementa generation y libera index. | Un EntityId viejo nunca accede al slot reciclado. Destructores de componentes de física deben correr mientras World sigue vivo. No reciclar un puntero como identidad. | 116/120 |
| main: `physics_to_render_group` y `snapshot_group` | Grupos persistentes; lambdas observan visual_bodies y física, escriben rangos de transform/snapshot. | Ambos waits preceden al siguiente consumidor y al siguiente tick. Crecimiento de visual_bodies prohibido mientras hay trabajos pendientes. | 114/127 |
| main: complete/previous animated instances | Vectores propietarios de RenderInstance, actualmente con propietarios de SkinPose. | Complete alimenta visibilidad; previous se usa para interpolación. No convertir shared_ptr a Span sin trasladar su propiedad a slots estables y registrar último lector. | 113/119/127 |
| GameUi::draw | Produce comandos/textos consumidos por renderer en este frame. | Texto transitorio válido durante copia/consumo CPU. Si el renderer difiere el consumo, copiar a su almacenamiento de frame; nunca guardar un puntero al buffer de formato local. | 126/119 |

`Span`, `ByteSpan` y `GpuRange`: siempre parámetros por valor. EntityId actual
ocupa 8 bytes (dos enteros); pasarlo por referencia no elimina una copia costosa.
Estructuras grandes se leen por referencia. Un Span no sincroniza ni posee.
`Span` de initializer_list solo sirve durante la llamada completa; no en una cola.

`Array` actual construye toda la capacidad, necesita default construction y move
assignment, no es copiable ni movible. reserve invalida vistas si crece; resize
dentro de capacidad no asigna y libera/reseteará propietarios en elementos retirados.
Usarlo para matrices/índices es sencillo; usarlo para cuerpos RAII o contextos sin
constructor por defecto exige primero revisar construcción y estabilidad, no
añadir un contenedor general preventivamente.

## Carga, cancelación y GPU

| Operación | Propiedad actual y condición de terminación | Condición que debe conservarse |
| --- | --- | --- |
| `AsyncAssetLoader::request` | Cache conserva shared_future; job copia AssetRecord y promise y observa `this`. Filesystem/catalog son referencias externas. | Loader, VFS, catálogo y JobSystem deben vivir hasta wait. invalidate devuelve false mientras future no está ready; no libera contexto en ejecución. |
| `AssetResidencyCoordinator::update` | Job de preparación **posee** ImportedScene, dependencias y registros movidos, promise, IDs y bandera BC. No captura renderer. Future publica PreparedScene; hilo principal publica uploads. | Contexto propietario estable hasta fin de job y consumo/cancelación. Una referencia a decoded_scene local caducaría al salir de update. |
| `cancel(ticket)` | Marca estado cancelado, retira ResidentScene y libera recursos. Trabajo de preparación pendiente puede terminar posteriormente, sin acceder al Request. | No publicar resultado cancelado. El contexto sigue vivo hasta terminar; un futuro ticket reciclado necesita generación para impedir resultado tardío. |
| Uploads de mesh/texture/material | Colas del renderer reciben propietarios; prepared mantiene listas de recursos y bind_rig. | Mover un upload por valor transfiere ownership. Si se cambia a Span, el owner debe cubrir la cola y la copia/consumo completo, no solo enqueue. |
| Retirada GPU | Política actual Diligent y prueba `gloom.vulkan_sync` conservadas. | GPU no queda libre por terminar wait CPU. Retirar tras último uso **grabado y enviado**, timeline/fence que lo cubre; drenar todas las submissions antes de device. Implementación final centralizada en 119. |
| Cierre main | Destruye residency/loader antes de apagar JobSystem; sus destructores esperan los grupos. | Mantener orden y drain GPU. Un shared_future ready no demuestra que todos los recursos GPU de su escena hayan dejado de usarse. |

El 118 sustituirá futures/owners heredados por slots de solicitud propios con
estado y generación, no por Spans a temporales. El 119 documentará una única
política de lifetime con create/destroy juntos. El 112 no reescribe estas rutas.

## Capturas y capacidades: decisión concreta

Medidas completas en [asignaciones](asignaciones.md). Todos los sitios de jobs de
src/apps son loader.request, residency.update, Visibility.build y los dos
parallel_for de main; el wrapper de parallel_for se mide con `gloom.jobs`, donde
el rango sí es no vacío. No fijar tamaño con el callable original olvidando el wrapper.

- CullJob: **72 bytes/alineación 8**; wrapper parallel_for: **32/8**;
  callables de main **16/8 y 32/8**. Candidato mínimo práctico de 114:
  FixedFunction con **80 bytes inline y alineación 8**, move-only y sin fallback heap,
  con static_assert de tamaño/alineación y invoke/destroy/move explícitos.
- Loader: **152/8**; preparación: **304/8**. Antes de admitirlos en el inline,
  mover payloads a contextos propietarios estables de solicitud; callable contiene
  solo puntero/handle. No elegir inline 304 para disimular copias de assets.
- TaskGroup::State: **184/8**, WorkItem: **80/8** actuales. Shared control block
  añade coste separado. Reutilizar estado solo con remaining=0; error de programmer
  assert, datos externos publicados como resultado. Los jobs con throw de la prueba
  heredada se sustituyen en 114, no se suprimen sus casos de fallo.
- La cola alcanza más de cien entradas durante arranque. Su pico depende del
  scheduling; la serie completa se conserva. No inferir límite máximo del juego
  del steady state. 114 debe medir saturación y resolver falta de hueco ayudando/
  esperando con progreso garantizado, sin pérdida silenciosa ni deadlock en jobs anidados.
- Entity slots medidos: **4**, no ocho jugadores. No fijar 4 como límite. 116
  comprobará 64/1.024 entidades, crecimiento, reciclaje y todos los punteros retenidos.
- Partículas/emisores reservados: **2.048/64**, límites ya existentes. Salidas de
  snapshots/visibilidad necesitan reserva por contenido real y crecimiento fuera
  de medición; los picos observados no autorizan truncar contenido mayor.

## Primitivas mínimas cuando aparezca el consumidor

No se implementa una stdlib propia ni se añade Boost/EASTL. Reusar types, Span,
Array y clock. Los nuevos tipos se añaden al hito consumidor, con su prueba.

| Primer consumidor | Diseño mínimo | Validación necesaria |
| --- | --- | --- |
| 114 Windows jobs | SRWLOCK para cola, CONDITION_VARIABLE con SleepConditionVariableSRW en bucle de predicado, WakeConditionVariable/WakeAllConditionVariable; CreateThread y HANDLE con join antes de liberar contexto; Interlocked para contadores publicados. | Sin espera activa; nested wait ayuda a ejecutar; no lost wakeup, shutdown con pendientes y saturación. Otros SO solo con necesidad real. |
| 117 textos/path | Vista `{const char*, size_t}` por valor, UTF-8 con tamaño explícito; owner de bytes con Array cuando se necesita conservar; terminador adicional solo al llamar una API C. | No asumir NUL en vista; conversión UTF-16 en la frontera Windows; traversal/UTF-8/longitudes inválidas devuelven error. |
| 117 resultado externo | Struct de resultado por operación con enum de error, dato propietario y mensaje acotado útil. No generalizar un expected con storage manual antes de ver el consumidor. | Roundtrip, datos truncados/corruptos y mensajes; assert reservado a misuse interno. |
| 117 archivos/VFS | FILE*/fread/fwrite/fseek o API nativa para garantías necesarias; root/mount/path policy existente. Read produce owner de bytes; vista no sobrevive al owner. | EOF/lectura parcial, symlink/traversal y límites del formato. No eliminar comprobaciones de seguridad. |
| 124 persistencia | Reutilizar lectura/resultado de 117; conservar flush/durable replace y validación de registros. | Crash/restart, corrupción y reemplazo duradero; no sustituir por fwrite sin garantías. |
| 125 HTTP/identidad | Reutilizar owner/vista/resultado anteriores; mantener API WinHTTP/OpenSSL y estados de error de transporte/credenciales. | Longitudes, TLS, timeouts, autorización y datos hostiles; no generic network framework. |

Los escenarios de aceptación de cada consumidor siguen en su ficha. Los comandos
de esta base se publican en [medidas](medidas.md); las pruebas sintéticas futuras
de 64/1.024 entidades o 1.000 poses no se presentan aquí como ejecutadas.
