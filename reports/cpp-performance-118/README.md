# Hito 118: carga y residencia con propiedad explícita

Terminado el 7 de octubre de 2026 sobre **934b73d** (117 cerrado). Ponytail full;
sin dependencias nuevas, cambio de renderer, formatos, arte, referencias o umbrales.
Commit local de cierre: resolver `rtk git log -1 --oneline --grep='^hito 118:'`;
no push y 119 no iniciado.

La carga coalesce mediante handles de slots/generación; la preparación consume
payloads prestados y fijados por esos handles, publica una sola vez y mueve la
salida al dueño de la escena. El rig preparado sustituye la retención redundante
de ImportedScene. Scratch de prioridades persistente y tablas de IDs propias.

Resultados: pico propio inicial Factory/Hound **−32,47 %/−33,25 %**;
carga inicial Hound **−7,64 %**, recarga Hound **−9,98 %** en las series medidas.
1.000 updates de 64 solicitudes queued sin crecimiento: **12.000 → 0 new**.
Sin ganancia global de FPS atribuida. [Método, todas las cifras y límites](medidas.md).

## Alcance y fronteras

Siete propietarios: tres headers/tres fuentes de loader, residencia y bridge,
y [prueba autónoma](../../tests/asset_residency_tests.cpp). Sin headers/símbolos
STL directos, futuros, promesas, mapas, shared_ptr, auto ordinario, herencia,
PIMPL, excepciones o RTTI propios. Cuatro .cpp con /EHs-c- y RTTI=false efectivos
en Release/Debug; [flags](datos/flags.md). Span y IDs pequeños por valor.

Se reutilizan Array/Span/FixedFunction/JobSystem y SRWLOCK existentes. Array
recibe solo push_back(T&&) para transferir handles/paquetes; sin otro contenedor.
Callers adaptados en app, pruebas de pipeline, animación y personaje; no se
declara cerrado su módulo. Corrección puntual en DiligentRenderer::release y su
prueba GPU: purgar uploads pendientes por ID y restar queued_bytes antes de
encolar el retiro ya existente. No migra ni sustituye el backend del 119.

La autorización STL del usuario permanece limitada al **117** y adaptaciones
necesarias. CookedAsset/AssetRecord/ImportedScene/VirtualPath/VFS/resultados de
codecs son datos/API de ese dueño y conservan su STL autorizada. El bridge llena
los MeshUpload/TextureUpload/MaterialUpload existentes y mueve sus buffers;
RenderInstance y las colas/mutex/mapas del backend son frontera legacy **119**.
Se usan sus operaciones de campos al adaptar esa frontera; no se certifica que
los DTO/headers transitivos estén libres de STL ni cumplimiento C++ global.
Renderer virtual/PIMPL y bloques legacy mantienen sus dueños 119/127/128.

## Contratos exactos

| Dueño/ruta | Vida útil y transición |
| --- | --- |
| AssetLoadHandle | Pin centralizado; copia aumenta pin, movimiento lo transfiere, destructor lo suelta. get devuelve const referencia y requiere ready; nunca espera. Loader vive más que todos sus handles. |
| Slot/cache | ID binario ordenado; descriptor reutilizable con generación creciente y resultado heap estable. invalidate retira incluso un load pendiente; el siguiente request crea otra generación. |
| Resultado retirado | El job termina para sus pins existentes, sin reemplazar la caché ni el slot nuevo. Solo se recicla cuando no tiene caché, job ni pins. Una carga vieja no aparece como generación activa. |
| Metadata/VFS | Mounts configurados antes de iniciar loads y fijos hasta drain. Mutaciones de catálogo serializadas con request; job conserva snapshot de AssetRecord. Catálogo/VFS/jobs viven hasta drain del loader. Join de productores antes de wait/destrucción. |
| Request/preparación | Request con dirección estable; contexto propio de FixedFunction, handles fijados y salida heap. Worker publica bajo SRWLOCK solo si coincide generación; cancel/reload la incrementan bajo ese mismo lock. |
| Update | Operaciones públicas serializadas por consumidor. Queued scratch reservado al crear tickets; sort solo de pares triviales/IDs, prioridad descendente y ticket ascendente; budget conservado. No espera futures/jobs. |
| ResidentScene | Instancias y AnimationRig propios, salida movida una vez. Ya no hay bind_rig compartido: todos sus callers consultan el rig preparado. Vistas caducan al cancelar/recargar/destruir. |
| Recursos compartidos | Referencias por ID únicas dentro de escena; upload solo en primera referencia y release solo en última. Materiales/texturas/meshes se retiran también ante fallo parcial. |
| Upload/cancel | Paquetes transfieren dueño a la cola. Release cancela pendientes; uso GPU ya enviado conserva la política de fence anterior. Un enqueue nuevo posterior a release sobrevive en el mismo frame. |
| Shutdown | Coordinator drena preparación antes de borrar requests/contextos; después loader drena lecturas. Apps conservan catálogo/VFS/renderer/jobs hasta ese drain; jobs.stop precede engine.stop. |

Reload mantiene el contrato anterior de compartir recursos GPU por ID mientras
otra escena los referencia. Reemplazar el contenido GPU de ese mismo ID con
otros consumidores vivos requiere el protocolo de versiones/retiro del **119**;
118 invalida/publica generaciones CPU y no afirma resolver ese reemplazo.
Tickets y entradas de referencias sin uso permanecen hasta destruir coordinator,
como los registros de requests anteriores; reutilizar reload evita crecimiento
por tickets nuevos. Índices ordenados/escaneos fríos son suficientes para el
fanout actual; no se añade una caché de preparación ni un gestor genérico.

## Validación

Builds completos finales Release/Debug y formato de **199 archivos** correctos. Suites
completas **51/55 y 52/55**, con los mismos fallos heredados: ui/factory/character
visual_review en ambas y animation_network/GNS resultado 25 en Release.
La revisión final preserva nombres contados (también tras cero embebido), con
prueba específica. Pruebas focalizadas finales **7/7**
en ambas: assets, asset_residency, gpu_assets, jobs, material_render, vulkan_sync,
hound_runtime. [Logs completos](datos/full-release.md), [Debug](datos/full-debug.md),
[focal Release](datos/focal-accepted-release.md), [Debug](datos/focal-accepted-debug.md).
Las tres pruebas adicionales de consumidores (animation_vfx, skin_bounds y
character_restoration) pasan en ambas configuraciones.

Pruebas significativas: duplicate pending requests con referencia idéntica;
invalidate pendiente y resultado viejo fijado; reciclaje con otra generación;
1.000 updates queued/ready y crecimiento positivo; nombres importados con cero
embebido conservados por búsqueda de bytes contados; jerarquía de 2.048 nodos;
dos escenas compartidas, reload de una sin liberar la otra, última referencia;
cancel/reload de preparación retenida, descarte comprobado por contador;
cancel/reload durante upload; fallo parcial y shutdown con job en vuelo.
Vulkan comprueba purge mesh/texture/material pendientes, contabilidad de bytes
y enqueue de la nueva generación antes de procesar el release anterior.

Los 104 blobs de entrada del 117 se fijan por SHA-256; no se recocinan para esta
comparación. Checksums/conteos de geometría y mipmaps coinciden antes/después y
tras reload. Todos los targets de consumidores compilan. No se cambian fuentes
de assets, protocolo 22, referencias Hound 131/132 ni el bloqueo de Meshy.

## Revisión visual

Primera serie de 67 capturas antes/después: 54 UI byte-idénticas; los comparadores de 6 Factory y
7 personajes pasan con los umbrales existentes. Frente a la primera captura,
Factory tiene max completo 245/255 y personaje 254/255; no se oculta esa diferencia.
Repetición del mismo ejecutable **anterior** reproduce la variabilidad: las seis
Factory quedan byte-idénticas a esa serie; personajes max control/serie **1/255**,
tres vistas byte-idénticas. Capturas completas Factory/archangel inspeccionadas.
El frame global gobierna jitter y el calentamiento de review cuenta frames tras
ready; la fase temporal es una explicación consistente con el código y los
controles, no una nueva certificación del renderer. Comparador/ref intactos.
[Capturas y hashes](datos/visuales.md), [control antiguo](datos/visuales-control.md).

Tras el último ajuste de nombres se repitieron las **67 capturas con el binario
final**: comparadores pasan y 54 UI siguen idénticas. Factory max completo
243/255 frente a la primera base y 242/255 frente al control antiguo; personajes
254/255 y 255/255, respectivamente. Media completa Factory 1,63–2,63/255,
personajes 2,83–4,11/255. Miniaturas: media ≤0,976/255 y ≤2,057/255,
peor tile ≤2,460/255 y ≤3,809/255; changed=0 % según el comparador existente.
La identidad de la serie previa no se extrapola a esta repetición; referencias,
umbrales y shaders conservados. [Serie final y hash](datos/visuales-final.md).

## Evidencia reproducible y siguiente frontera

[Manifest](datos/manifest.md) fija base, binarios/libs, propietarios, conteo
(199 archivos/37.767 líneas), 104 fixtures y hashes. [Probe/receta](datos/probe.md)
contiene fuentes, flags, ventanas y runner. Solo Markdown versionado; capturas,
binarios y caches quedan fuera de Git. No se atribuye cero heap global.
119 debe cerrar datos/colas CPU del renderer y retiro/versiones GPU; su ficha
sigue no iniciada. No se crea stdlib nueva, Boost/EASTL, GPU API alternativa ni
abstracción adicional de streams. Los listeners Jolt siguen pendientes en 120/128.
