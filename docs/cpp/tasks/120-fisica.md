# Hito 120: Física y consultas con eventos reutilizables

Estado: **no iniciado**. Depende de: **116 y 119; decisión de compatibilidad Jolt de 112 resuelta**.
Objetivo: **Cumplimiento C++ y coste de simulación**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
6 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

JoltWorld implementa World virtual/PIMPL y usa mapas para cuerpos/contactos, arrays temporales de eventos y consultas de CharacterVirtual. Cinco
adaptadores propios heredan de interfaces Jolt (layers, pairs y listeners). Esto entra en conflicto con la prohibición literal de herencia del AGENTS
actual.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [include/gloom/physics/world.hpp](../../../include/gloom/physics/world.hpp)
- [include/gloom/physics/components.hpp](../../../include/gloom/physics/components.hpp)
- [include/gloom/physics/triangle_query.hpp](../../../include/gloom/physics/triangle_query.hpp)
- [include/gloom/backends/jolt_world.hpp](../../../include/gloom/backends/jolt_world.hpp)
- [src/backends/jolt_world.cpp](../../../src/backends/jolt_world.cpp)
- [src/gameplay/legacy_movement.cpp](../../../src/gameplay/legacy_movement.cpp)

## Archivos responsables de cierre

- [include/gloom/backends/jolt_world.hpp](../../../include/gloom/backends/jolt_world.hpp)
- [include/gloom/physics/components.hpp](../../../include/gloom/physics/components.hpp)
- [include/gloom/physics/triangle_query.hpp](../../../include/gloom/physics/triangle_query.hpp)
- [include/gloom/physics/world.hpp](../../../include/gloom/physics/world.hpp)
- [src/backends/jolt_world.cpp](../../../src/backends/jolt_world.cpp)
- [tests/physics_tests.cpp](../../../tests/physics_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Aplicar la solución equivalente demostrada en 112 para la frontera Jolt. Si sigue requiriendo herencia propia y la regla no se ha resuelto
   explícitamente, declarar este hito bloqueado antes de editar esa frontera; no eliminar contactos, esconder el adapter en vendor ni cambiar motor
   físico para cerrar el inventario.
2. Migrar World/JoltWorld propios a composición directa con estado/lifetime explícitos y sin PIMPL; usar registros por slots/IDs y buffers de eventos
   reutilizados. Conservar sincronización de callbacks/eventos cuando haya varios workers; el consumidor no ve memoria que el siguiente simulate
   sobrescribe antes de usarla.
3. Reservar scratch de consultas y contactos al configurar capacidades; conservar body/character owner y generación de instancia al reiniciar. Retener
   mesh/shape hasta retirar sus cuerpos; no almacenar spans de mesh temporal.
4. Mantener fixed_step, max_sub_steps, broadphase, filtros, gravedad, cápsula, grounded/slope/step, moving platforms, trigger entered/stayed/exited y
   consultas de rollback sin historia oculta. No sustituir Jolt por colisiones simplificadas.

## Contratos que conservar

No retirar validación de datos externos de colisión. Configuración inválida se detecta antes de start; handles/precondiciones internas usan asserts.
No suprimir locks porque el renderer sea de sincronización externa.

## Validación focalizada

CTest existentes: `gloom.physics`, `gloom.legacy_movement`, `gloom.vertical_slice`, `gloom.combat`, `gloom.network_replication`, `gloom.vertical_slice_smoke`.

Reinicio e IDs viejos, destrucción con contactos activos, varios workers, triggers múltiples, character impulses/platforms, ground/step y queries tras
rollback. Comparar snapshots de simulación con la entrada para misma secuencia de inputs.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

Tiempo simulate/query y memoria de contactos/eventos; 1.000 ticks propios estables sin temporales de eventos. Informar asignaciones internas de Jolt
aparte y conservar sus límites de bodies/pairs/constraints.

Simulación equivalente y almacenamiento propio migrado. Solo cerrar cumplimiento completo si el conflicto de callbacks ha quedado resuelto de manera
explícita; si existe una excepción autorizada, enumerarla y no afirmar cero herencia global.

Entregar informe `reports/cpp-performance-120/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 120: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/120-fisica.md. Ejecuta solo
> el hito 120 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
