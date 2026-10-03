# Hito 125: Identidad, tickets y servicios HTTP/HTTPS

Estado: **no iniciado**. Depende de: **123 y 124**.
Objetivo: **Cumplimiento C++ y propiedad de solicitudes**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
18 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

Backends de identidad/tickets/WinHTTP/HTTPS usan strings/resultados/containers, requests diferidas y interfaces del directorio. winhttp_request.hpp ya
tiene tipos propios; se debe reutilizar su frontera. Existen grants, credenciales reader y presupuestos por petición.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [include/gloom/backends/keycloak_identity.hpp](../../../include/gloom/backends/keycloak_identity.hpp)
- [src/backends/keycloak_identity.cpp](../../../src/backends/keycloak_identity.cpp)
- [src/backends/winhttp_identity.cpp](../../../src/backends/winhttp_identity.cpp)
- [src/backends/game_ticket.cpp](../../../src/backends/game_ticket.cpp)
- [src/backends/match_https_host.cpp](../../../src/backends/match_https_host.cpp)
- [src/backends/winhttp_match_service.cpp](../../../src/backends/winhttp_match_service.cpp)
- [apps/gloom_match_service/main.cpp](../../../apps/gloom_match_service/main.cpp)

## Archivos responsables de cierre

- [apps/gloom_match_service/main.cpp](../../../apps/gloom_match_service/main.cpp)
- [include/gloom/backends/game_ticket.hpp](../../../include/gloom/backends/game_ticket.hpp)
- [include/gloom/backends/keycloak_identity.hpp](../../../include/gloom/backends/keycloak_identity.hpp)
- [include/gloom/backends/match_https_host.hpp](../../../include/gloom/backends/match_https_host.hpp)
- [include/gloom/backends/match_reader_credential.hpp](../../../include/gloom/backends/match_reader_credential.hpp)
- [include/gloom/backends/match_reader_grants.hpp](../../../include/gloom/backends/match_reader_grants.hpp)
- [include/gloom/backends/match_request_budget.hpp](../../../include/gloom/backends/match_request_budget.hpp)
- [include/gloom/backends/winhttp_match_service.hpp](../../../include/gloom/backends/winhttp_match_service.hpp)
- [include/gloom/backends/winhttp_request.hpp](../../../include/gloom/backends/winhttp_request.hpp)
- [src/backends/game_ticket.cpp](../../../src/backends/game_ticket.cpp)
- [src/backends/keycloak_identity.cpp](../../../src/backends/keycloak_identity.cpp)
- [src/backends/match_https_host.cpp](../../../src/backends/match_https_host.cpp)
- [src/backends/match_reader_grants.cpp](../../../src/backends/match_reader_grants.cpp)
- [src/backends/winhttp_identity.cpp](../../../src/backends/winhttp_identity.cpp)
- [src/backends/winhttp_match_service.cpp](../../../src/backends/winhttp_match_service.cpp)
- [tests/game_ticket_tests.cpp](../../../tests/game_ticket_tests.cpp)
- [tests/keycloak_identity_tests.cpp](../../../tests/keycloak_identity_tests.cpp)
- [tests/match_https_tests.cpp](../../../tests/match_https_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Migrar todos los archivos de auth/HTTP asignados en INVENTARIO, su ejecutable y pruebas a textos/resultados/bytes propios; usar
   WinHTTP/OpenSSL/simdjson actuales. Mantener conversión UTF-8/UTF-16, límites de response/body y propietario de request/response hasta completar
   async.
2. Retirar jerarquías/PIMPL propios y callbacks lambda por estado/contexto concreto; cerrar/cancelar handles y drenar threads/jobs antes de destruir
   credenciales o contexto. Errores HTTP/TLS/auth se mantienen como resultados externos, nunca asserts.
3. Preservar TLS/verificación del servidor, JWT/JWKS/algoritmos/issuer/audience/exp, firma y expiración de game tickets, rotation/cache y refresh; no
   eliminar controles para reducir CPU. Mantener grants readers y request budgets.
4. Conservar comandos, endpoints, JSON/status y despliegue/package existentes. El hito no publica servicios, no modifica Keycloak remoto ni usa
   credenciales reales en pruebas/informes.

## Contratos que conservar

No registrar secretos/tokens. Las vistas del parser/response no se conservan tras liberar su buffer; zero-copy solo mientras el dueño vive. Invalid
external data se rechaza con las comprobaciones originales.

## Validación focalizada

CTest existentes: `gloom.game_tickets`, `gloom.keycloak_identity`, `gloom.match_https`, `gloom.match_discovery`, `gloom.session`.

Happy path y todos los casos actuales de rechazo TLS/auth/signature/expiry/reader/rate budget, refresh/retry y shutdown de requests pendientes.
Fixture local de HTTPS y servidor; sin red de producción necesaria.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

Latencia/allocations de parse/sign/verify/request en fixtures; inicialización y red externa separadas. Objetivo primario equivalencia/cumplimiento,
sin prometer mejora de FPS.

Targets identity/https/service y pruebas sin usos propios prohibidos; seguridad y códigos externos conservados, cierres sin requests ni buffers pendientes.

Entregar informe `reports/cpp-performance-125/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 125: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/125-identidad-y-http.md. Ejecuta solo
> el hito 125 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
