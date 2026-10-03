# Hito 127: Composición del runtime, arranque y aplicaciones

Estado: **no iniciado**. Depende de: **113–126 terminados**.
Objetivo: **Cumplimiento C++ y coste de snapshots**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
14 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

Engine posee vector de unique_ptr<Subsystem>, invoca virtuals y hace rollback mediante excepciones. Window/Placeholder y applications aún tienen
interfaces, lambdas, filesystem/containers, snapshots temporales y herramientas de revisión C++.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [include/gloom/core/engine.hpp](../../../include/gloom/core/engine.hpp)
- [src/core/engine.cpp](../../../src/core/engine.cpp)
- [include/gloom/core/subsystem.hpp](../../../include/gloom/core/subsystem.hpp)
- [src/backends/placeholder.cpp](../../../src/backends/placeholder.cpp)
- [include/gloom/platform/window.hpp](../../../include/gloom/platform/window.hpp)
- [src/backends/sdl_window.cpp](../../../src/backends/sdl_window.cpp)
- [apps/gloom/main.cpp](../../../apps/gloom/main.cpp)
- [apps/gloom_scene_viewer/main.cpp](../../../apps/gloom_scene_viewer/main.cpp)

## Archivos responsables de cierre

- [apps/gloom/main.cpp](../../../apps/gloom/main.cpp)
- [apps/gloom/visual_review.hpp](../../../apps/gloom/visual_review.hpp)
- [apps/gloom_scene_viewer/main.cpp](../../../apps/gloom_scene_viewer/main.cpp)
- [include/gloom/backends/placeholder.hpp](../../../include/gloom/backends/placeholder.hpp)
- [include/gloom/backends/sdl_window.hpp](../../../include/gloom/backends/sdl_window.hpp)
- [include/gloom/core/engine.hpp](../../../include/gloom/core/engine.hpp)
- [include/gloom/core/subsystem.hpp](../../../include/gloom/core/subsystem.hpp)
- [include/gloom/platform/input.hpp](../../../include/gloom/platform/input.hpp)
- [include/gloom/platform/movement_actions.hpp](../../../include/gloom/platform/movement_actions.hpp)
- [include/gloom/platform/window.hpp](../../../include/gloom/platform/window.hpp)
- [src/backends/placeholder.cpp](../../../src/backends/placeholder.cpp)
- [src/backends/sdl_window.cpp](../../../src/backends/sdl_window.cpp)
- [src/core/engine.cpp](../../../src/core/engine.cpp)
- [tests/engine_tests.cpp](../../../tests/engine_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Sustituir registro genérico de subsystems por composición concreta de los componentes usados en cada aplicación, con inicio/stop explícitos. Si se
   conserva una lista de callbacks, justificar consumidores reales y no reconstruir la misma jerarquía como framework.
2. Arranque devuelve resultado y deshace en orden inverso SOLO lo que se inició; tick exige estado válido con assert. Parada: dejar de producir,
   drenar network/HTTP/jobs/carga, detener audio, completar todos los frames GPU, retirar recursos, dispositivo y ventana. Detallar el orden para
   cliente, viewer y servidores.
3. Migrar ventana/input/placeholder y todos los review helpers C++ a tipos propios; retirar PIMPL/herencia/STL/excepciones/RTTI restantes de estas
   rutas. Mantener options/diagnostics y funciones que usan las pruebas automáticas.
4. Consolidar buffers persistentes de transforms/snapshots/instancias en main/viewer y convertir spans de compatibilidad del 110 que queden. Evitar
   copias grandes por valor, pero mantener IDs, Span/ByteSpan/GpuRange por valor y transferencias propietarias explícitas.
5. Eliminar wrappers/abstracciones muertas al desaparecer sus últimos consumers. No crear variables para expresiones triviales ni un
   filesystem/thread/function general; respetar defaults, designated initializers y 160 columnas.

## Contratos que conservar

Mismas opciones de cliente/viewer/cooker/server/service y mismo orden funcional. Un fallo parcial de start no deja hilos o recursos vivos. Retirar
Subsystem no autoriza a quitar capacidades de backend existentes.

## Validación focalizada

CTest existentes: `gloom.unit`, `gloom.sdl_smoke`, `gloom.slice_server_smoke`, `gloom.network_scene_smoke`, `gloom.vertical_slice_smoke`,
`gloom.ui_flow`, `gloom.animation_dedicated`, `gloom.vulkan_sync`, `gloom.hound_runtime`.

Start/stop/restart y fallo después de cada etapa de inicialización, cierre durante loading o request, resize y frame en vuelo. Todos los
ejecutables/flags de revisión aún funcionan; build completo en ambas configuraciones.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

CPU snapshots/update/presentation y asignaciones/capacidades calentadas; objetivo cero temporales propios de snapshots dentro de capacidad. Informar
copies obligatorias y transferencias una vez, sin perseguir cada paso por valor.

Aplicaciones y core/platform propios sin registros virtuales/PIMPL/STL/excepciones/RTTI, rollback explícito correcto y no queda puente de
compatibilidad std::span del 110.

Entregar informe `reports/cpp-performance-127/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 127: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/127-runtime-y-apps.md. Ejecuta solo
> el hito 127 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
