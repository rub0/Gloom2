# Hito 122: Red, replicación y gameplay con buffers acotados

Estado: **no iniciado**. Depende de: **114, 116 y 120**.
Objetivo: **CPU, memoria y cumplimiento C++**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
54 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

Protocol/session/transport/simulator y gameplay usan vectors, deque, maps, optional, expected, function y PIMPL. Mensajes heredados de otros structs
también cuentan como herencia. Los paquetes externos requieren validación; los buffers de prediction/snapshot tienen límites funcionales existentes.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [include/gloom/network/protocol.hpp](../../../include/gloom/network/protocol.hpp)
- [include/gloom/network/session.hpp](../../../include/gloom/network/session.hpp)
- [include/gloom/network/transport.hpp](../../../include/gloom/network/transport.hpp)
- [src/backends/gns_transport.cpp](../../../src/backends/gns_transport.cpp)
- [src/gameplay/vertical_slice.cpp](../../../src/gameplay/vertical_slice.cpp)
- [src/gameplay/vertical_slice_network.cpp](../../../src/gameplay/vertical_slice_network.cpp)
- [apps/gloom_slice_server/main.cpp](../../../apps/gloom_slice_server/main.cpp)

## Archivos responsables de cierre

- [apps/gloom_slice_server/main.cpp](../../../apps/gloom_slice_server/main.cpp)
- [include/gloom/backends/gns_transport.hpp](../../../include/gloom/backends/gns_transport.hpp)
- [include/gloom/gameplay/character_presentation.hpp](../../../include/gloom/gameplay/character_presentation.hpp)
- [include/gloom/gameplay/component_replication.hpp](../../../include/gloom/gameplay/component_replication.hpp)
- [include/gloom/gameplay/components.hpp](../../../include/gloom/gameplay/components.hpp)
- [include/gloom/gameplay/first_person.hpp](../../../include/gloom/gameplay/first_person.hpp)
- [include/gloom/gameplay/first_person_presentation.hpp](../../../include/gloom/gameplay/first_person_presentation.hpp)
- [include/gloom/gameplay/kinematic_motion.hpp](../../../include/gloom/gameplay/kinematic_motion.hpp)
- [include/gloom/gameplay/legacy_arsenal.hpp](../../../include/gloom/gameplay/legacy_arsenal.hpp)
- [include/gloom/gameplay/legacy_movement.hpp](../../../include/gloom/gameplay/legacy_movement.hpp)
- [include/gloom/gameplay/legacy_pickups.hpp](../../../include/gloom/gameplay/legacy_pickups.hpp)
- [include/gloom/gameplay/match_lobby.hpp](../../../include/gloom/gameplay/match_lobby.hpp)
- [include/gloom/gameplay/pickup_presentation.hpp](../../../include/gloom/gameplay/pickup_presentation.hpp)
- [include/gloom/gameplay/slice_selection.hpp](../../../include/gloom/gameplay/slice_selection.hpp)
- [include/gloom/gameplay/vertical_slice.hpp](../../../include/gloom/gameplay/vertical_slice.hpp)
- [include/gloom/gameplay/vertical_slice_network.hpp](../../../include/gloom/gameplay/vertical_slice_network.hpp)
- [include/gloom/network/clock_sync.hpp](../../../include/gloom/network/clock_sync.hpp)
- [include/gloom/network/combat.hpp](../../../include/gloom/network/combat.hpp)
- [include/gloom/network/movement_replication.hpp](../../../include/gloom/network/movement_replication.hpp)
- [include/gloom/network/network_simulator.hpp](../../../include/gloom/network/network_simulator.hpp)
- [include/gloom/network/prediction_history.hpp](../../../include/gloom/network/prediction_history.hpp)
- [include/gloom/network/presentation_smoother.hpp](../../../include/gloom/network/presentation_smoother.hpp)
- [include/gloom/network/protocol.hpp](../../../include/gloom/network/protocol.hpp)
- [include/gloom/network/session.hpp](../../../include/gloom/network/session.hpp)
- [include/gloom/network/snapshot_buffer.hpp](../../../include/gloom/network/snapshot_buffer.hpp)
- [include/gloom/network/transport.hpp](../../../include/gloom/network/transport.hpp)
- [src/backends/gns_transport.cpp](../../../src/backends/gns_transport.cpp)
- [src/gameplay/first_person.cpp](../../../src/gameplay/first_person.cpp)
- [src/gameplay/legacy_arsenal.cpp](../../../src/gameplay/legacy_arsenal.cpp)
- [src/gameplay/legacy_movement.cpp](../../../src/gameplay/legacy_movement.cpp)
- [src/gameplay/legacy_pickups.cpp](../../../src/gameplay/legacy_pickups.cpp)
- [src/gameplay/match_lobby.cpp](../../../src/gameplay/match_lobby.cpp)
- [src/gameplay/pickup_presentation.cpp](../../../src/gameplay/pickup_presentation.cpp)
- [src/gameplay/vertical_slice.cpp](../../../src/gameplay/vertical_slice.cpp)
- [src/gameplay/vertical_slice_network.cpp](../../../src/gameplay/vertical_slice_network.cpp)
- [src/network/clock_sync.cpp](../../../src/network/clock_sync.cpp)
- [src/network/combat.cpp](../../../src/network/combat.cpp)
- [src/network/movement_replication.cpp](../../../src/network/movement_replication.cpp)
- [src/network/network_simulator.cpp](../../../src/network/network_simulator.cpp)
- [src/network/protocol.cpp](../../../src/network/protocol.cpp)
- [src/network/session.cpp](../../../src/network/session.cpp)
- [tests/combat_tests.cpp](../../../tests/combat_tests.cpp)
- [tests/legacy_arsenal_tests.cpp](../../../tests/legacy_arsenal_tests.cpp)
- [tests/legacy_movement_tests.cpp](../../../tests/legacy_movement_tests.cpp)
- [tests/legacy_pickups_tests.cpp](../../../tests/legacy_pickups_tests.cpp)
- [tests/match_lobby_tests.cpp](../../../tests/match_lobby_tests.cpp)
- [tests/network_protocol_tests.cpp](../../../tests/network_protocol_tests.cpp)
- [tests/network_replication_tests.cpp](../../../tests/network_replication_tests.cpp)
- [tests/network_transport_tests.cpp](../../../tests/network_transport_tests.cpp)
- [tests/pickup_presentation_tests.cpp](../../../tests/pickup_presentation_tests.cpp)
- [tests/session_tests.cpp](../../../tests/session_tests.cpp)
- [tests/vertical_slice_network_tests.cpp](../../../tests/vertical_slice_network_tests.cpp)
- [tests/vertical_slice_tests.cpp](../../../tests/vertical_slice_tests.cpp)
- [tests/vertical_slice_transport_tests.cpp](../../../tests/vertical_slice_transport_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Migrar todos los headers/src de network y gameplay de simulación asignados en INVENTARIO: enteros propios, registros por slots/IDs,
   histories/queues reservadas, resultado explícito y callbacks mínimos. Retirar herencia propia de mensajes/Transport y PIMPL de simulación/GNS;
   mantener API externa de GNS y sincronización necesaria.
2. Dar propiedad a bytes encolados hasta completar envío/consumo; encode a buffer del llamador con longitud/capacidad y decode desde ByteSpan por
   valor. Preservar protocol_version=22, layout/endianness, secuencias, flags y MTU; no cambiar el protocolo para acomodar los tipos nuevos.
3. Conservar autoridad, autenticación de sesiones, token de resume, reconexión, clock sync, interpolation/extrapolation, loss/reorder, rollback y
   validación de disparos. No convertir validaciones de paquetes/credenciales externos en asserts ni usar RNG inseguro para tokens.
4. Migrar legacy arsenal/pickups/movement, first-person/presentation, selección de personajes, composición y snapshots. Reutilizar salida
   tick/presentation; separar estado autoritativo de animación/audio/efectos. Actualizar servidor dedicado y pruebas sin reducir capacidades
   existentes.

## Contratos que conservar

Mismos bytes del protocolo y misma simulación para el mismo input. No usar IDs temporales como dueño de un paquete retenido. Sin pérdida adicional de
eventos por saturación ni desconexión prematura al simplificar estructuras.

## Validación focalizada

CTest existentes: `gloom.network`, `gloom.network_protocol`, `gloom.network_replication`, `gloom.session`, `gloom.combat`, `gloom.vertical_slice`,
`gloom.vertical_slice_network`, `gloom.vertical_slice_transport`, `gloom.match_lobby`, `gloom.legacy_movement`, `gloom.legacy_arsenal`,
`gloom.legacy_pickups`, `gloom.pickup_presentation`, `gloom.slice_server_smoke`, `gloom.network_scene_smoke`, `gloom.animation_network`,
`gloom.animation_dedicated`.

Golden bytes de mensajes y roundtrip, paquetes malformados, capacity full, late/duplicate/reorder/loss, reconnect y datos de otra entidad. Comparar
ticks/snapshots deterministas; conservar pruebas de servidor y partidas de red, incluidos los fallos heredados documentados.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

CPU tick/encode/decode y bytes/asignaciones por paquete o snapshot a 2/8/64 clientes de pruebas. Objetivo: almacenamiento propio estable dentro de
capacidades; cargas de conexión/descarga informadas aparte. No declarar ocho humanos validados con el probe gráfico.

Todos los archivos asignados de network/simulación/servidor sin uso propio prohibido; pruebas de wire y autoridad intactas. Los callbacks no retienen
contextos caducados y el apagado drena conexiones/trabajos.

Entregar informe `reports/cpp-performance-122/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 122: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/122-red-y-simulacion.md. Ejecuta solo
> el hito 122 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
