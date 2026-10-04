# Hito 114: JobSystem y FixedFunction con grupos reutilizables

Estado: **terminado, 4 de octubre de 2026**. Depende de: **112 y 113, terminados**.
Objetivo: **Asignaciones, sincronización y cumplimiento C++**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
4 archivos propietarios a este hito, incluido FixedFunction. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

create_group asigna State con shared_ptr; WorkItem lo comparte; parallel_for asigna otro callable compartido. La cola deque, jthread, mutex y
condition_variable son STL. Las pruebas contienen trabajos anidados y grupos reutilizados; el predicado de finalización se protege por el mismo mutex
de wait para evitar notificaciones perdidas.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [include/gloom/core/job_system.hpp](../../../include/gloom/core/job_system.hpp)
- [src/core/job_system.cpp](../../../src/core/job_system.cpp)
- [src/render/visibility.cpp](../../../src/render/visibility.cpp)
- [src/assets/asset_loader.cpp](../../../src/assets/asset_loader.cpp)
- [src/assets/residency_coordinator.cpp](../../../src/assets/residency_coordinator.cpp)
- [apps/gloom/main.cpp](../../../apps/gloom/main.cpp)
- [tests/job_system_tests.cpp](../../../tests/job_system_tests.cpp)

## Archivos responsables de cierre

- [include/gloom/core/fixed_function.hpp](../../../include/gloom/core/fixed_function.hpp)
- [include/gloom/core/job_system.hpp](../../../include/gloom/core/job_system.hpp)
- [src/core/job_system.cpp](../../../src/core/job_system.cpp)
- [tests/job_system_tests.cpp](../../../tests/job_system_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Resultado ejecutado

[Informe](../../../reports/cpp-performance-114/README.md) y
[medidas completas](../../../reports/cpp-performance-114/medidas.md).
FixedFunction void() 80/8, 104 bytes completos; grupos estables caller-owned,
sin pool ni asignación; cola 256×112 bytes, asistencia al saturar, hilos/lock y
dos condiciones Win32. Arranque devuelve error y revierte creación parcial;
stop drena descendientes y une handles. Todos los callers adaptados.
Datos, cache y futuros legacy de assets conservan dueño 117/118; se cierran
scheduler/capturas, no se certifica cumplimiento global de esos formatos.

Mil rondas con grupos nuevos/reutilizados, 6.000 jobs, cero new/bytes con uno y
cuatro workers. Factory/Hound: jobs 3/4 → 0 new/frame; ahorro del 110 separado.
Saturación forzada, múltiples productores, captura movible/destrucción, espera
anidada, cancel/reload, descendientes, reinicio y tres asserts Debug comprobados.
Arranque parcial falla controladamente y recupera; cinco capturas no soportadas
rechazan en compilación. Builds y formato correctos; Debug 50/53, Release 49/53,
fallos heredados descritos por causa. Siete vistas alineadas pasan.
No se afirma mejora global de FPS ni cumplimiento de H06 en todas las pasadas.
Commit local de cierre: `rtk git log -1 --oneline --grep='^hito 114:'`.
112/113 subidos por encargo; 114 sin push, 115 no iniciado.

## Trabajo concreto, en orden

1. Implementar FixedFunction solo para firmas/capturas reales: almacenamiento inline de tamaño/alineación determinados en 112, movimiento y
   destrucción explícitos, callable vacío y llamada con precondición, sin herencia ni heap fallback. Rechazar en compilación capturas demasiado
   grandes o no soportadas; los payloads grandes pertenecen a una solicitud estable y el trabajo guarda su referencia/handle.
2. Usar grupos reutilizables con propietario y generación, pendientes protegidos y liberación tras wait/drain. Prohibir reutilizar un slot con
   trabajos activos; permitir el patrón real de varios schedule y wait del mismo grupo. Sustituir la captura compartida de parallel_for por
   functor/contexto cuya vida cubra la espera.
3. Usar hilos y sincronización nativos; conservar inicialmente una cola protegida y buffers reservados. Mantener ejecución asistida por el llamador y
   wait anidado para que un único worker no se bloquee. Definir una política sin pérdida cuando la cola/grupos alcanzan capacidad: asistencia/drain
   con progreso comprobado, nunca descartar trabajos.
4. Actualizar TODOS los callers enumerados: visibilidad, loader, preparación de residencia y dos parallel_for de main. Retirar PIMPL/herencia de
   JobSystem y arrancarlo/pararlo directamente en sus aplicaciones, eliminando su alta virtual en Engine sin crear otro adapter heredado o sistema
   provisional. El inicio devuelve error y stop drena trabajos; la propagación de excepciones se elimina con todos los jobs migrados.

## Contratos que conservar

No retirar locks exigidos por concurrencia. Las reglas de sincronización externa de una API gráfica no se trasladan a jobs. No hacer wait dentro del
mismo grupo que incluye al propio trabajo; documentar y assert de esa precondición. No hay dereferencias de solicitudes canceladas ni grupos de otro
JobSystem.

## Validación focalizada

CTest existentes: `gloom.jobs`, `gloom.storage`, `gloom.visibility`, `gloom.assets`, `gloom.gpu_assets`, `gloom.vertical_slice_smoke`, `gloom.vulkan_sync`.

Un worker y varios, grupos vacíos/repetidos, múltiples productores, wait anidado en grupo distinto, saturación de cola, stop con trabajos pendientes y
reinicio; exactamente una ejecución y destrucción por job. Prueba de capturas movibles con recursos y control de lifetime. Reemplazar el caso de
excepción por resultados explícitos sin reducir la cobertura de finalización.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

1.000 ciclos de schedule/wait tras reserva dentro de capacidad sin new/new[] propios; medir también schedule+wait por lote, profundidad máxima, jobs
asistidos y espera. Visibilidad completa: separar el ahorro del scheduler del ahorro del 110.

No STL, shared_ptr, virtual/PIMPL, excepciones/RTTI o lambdas en JobSystem y jobs propios migrados; cierre completo sin trabajo perdido ni espera
infinita. La capacidad y la política de saturación se justifican con 112.

Entregar informe `reports/cpp-performance-114/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 114: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/114-jobs.md. Ejecuta solo
> el hito 114 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
