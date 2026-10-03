# Hito 118: Carga asíncrona y residencia con propiedad explícita

Estado: **no iniciado**. Depende de: **114 y 117**.
Objetivo: **CPU, memoria y vidas útiles**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
6 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

Loader usa shared_future/promise y mapa de solicitudes; residencia usa futuros, mapas de referencias, preparación asíncrona y make_shared de escenas.
update crea una lista temporal de solicitudes ordenadas. Cancelar/invalidate/reload puede coincidir con jobs activos y con recursos ya en GPU.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [include/gloom/assets/asset_loader.hpp](../../../include/gloom/assets/asset_loader.hpp)
- [src/assets/asset_loader.cpp](../../../src/assets/asset_loader.cpp)
- [include/gloom/assets/residency_coordinator.hpp](../../../include/gloom/assets/residency_coordinator.hpp)
- [src/assets/residency_coordinator.cpp](../../../src/assets/residency_coordinator.cpp)
- [include/gloom/assets/scene_gpu_bridge.hpp](../../../include/gloom/assets/scene_gpu_bridge.hpp)
- [src/assets/scene_gpu_bridge.cpp](../../../src/assets/scene_gpu_bridge.cpp)

## Archivos responsables de cierre

- [include/gloom/assets/asset_loader.hpp](../../../include/gloom/assets/asset_loader.hpp)
- [include/gloom/assets/residency_coordinator.hpp](../../../include/gloom/assets/residency_coordinator.hpp)
- [include/gloom/assets/scene_gpu_bridge.hpp](../../../include/gloom/assets/scene_gpu_bridge.hpp)
- [src/assets/asset_loader.cpp](../../../src/assets/asset_loader.cpp)
- [src/assets/residency_coordinator.cpp](../../../src/assets/residency_coordinator.cpp)
- [src/assets/scene_gpu_bridge.cpp](../../../src/assets/scene_gpu_bridge.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Sustituir futuras/promesas por slots de solicitud con ID/generación, estado y payload propietario. Coalescer requests al mismo asset como ahora;
   publicar datos con sincronización correcta y mantener al dueño hasta terminar el job. El consumidor recibe una vista/handle estable, no una copia
   de todo CookedAsset.
2. Reutilizar scratch de prioridades/queues y preparación; ordenar índices/IDs triviales, nunca qsort de objetos propietarios. Conservar estados,
   prioridades, invalidación y error_placeholder. No bloquear el frame esperando assets que antes eran asíncronos.
3. Conservar referencias por escena/material/textura/rig y liberación de la última referencia CPU; el último uso GPU se retira según 119. Cancelación
   impide publicar una generación vieja, pero drena o conserva su contexto hasta finalizar el job.
4. Cambiar scene_gpu_bridge y todos los consumers de ResidentScene/bind_rig con la misma política; asegurar loader.wait/stop al apagar y el orden de
   destrucción de jobs, catálogo, solicitudes y renderer.

## Contratos que conservar

Ningún Span de uploads/request vive menos que la cola que lo retiene. Los payloads diferidos se mueven/transfieren o permanecen en propietario
estable. No inventar referencias manuales dispersas ni contador de uso que ignore jobs en vuelo.

## Validación focalizada

CTest existentes: `gloom.assets`, `gloom.gpu_assets`, `gloom.jobs`, `gloom.material_render`, `gloom.vulkan_sync`, `gloom.hound_runtime`.

Requests duplicadas, cancel/reload durante decode/upload, assets compartidos por dos escenas, fallo parcial, última referencia y shutdown con trabajo
en vuelo. Preservar missing/budget/evictions y limitar la preparación sin cambiar prioridad.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

update sin nuevas solicitudes ni crecimiento: 1.000 pasos sin asignaciones propias de scratch/colas. Medir carga inicial, cancel/reload y pico de
memoria por separado. Mantener presupuesto residente configurado de 512 MiB y el sobre Hound medido de 384 MiB.

No futuros, mapas, shared_ptr ni STL/excepciones/RTTI en loader/residencia propios; todas las solicitudes tienen una transición final y ninguna
generación antigua publica resultados.

Entregar informe `reports/cpp-performance-118/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 118: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/118-carga-y-residencia.md. Ejecuta solo
> el hito 118 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
