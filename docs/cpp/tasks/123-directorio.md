# Hito 123: Directorio de partidas y navegador asíncrono

Estado: **no iniciado**. Depende de: **114, 117 y 122**.
Objetivo: **Simplificación de interfaces y cumplimiento C++**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
5 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

SliceMatchDirectory/Service/Connector son interfaces virtuales; factories shared/unique, expected, function y future. AsyncSliceMatchBrowser lanza un
futuro y conserva estado de selección/resultados. Publication hace withdrawal owner-checked y tiene leases/retries.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [include/gloom/gameplay/match_discovery.hpp](../../../include/gloom/gameplay/match_discovery.hpp)
- [src/gameplay/match_discovery.cpp](../../../src/gameplay/match_discovery.cpp)
- [include/gloom/gameplay/match_service_wire.hpp](../../../include/gloom/gameplay/match_service_wire.hpp)
- [src/gameplay/match_service_wire.cpp](../../../src/gameplay/match_service_wire.cpp)
- [tests/match_discovery_tests.cpp](../../../tests/match_discovery_tests.cpp)

## Archivos responsables de cierre

- [include/gloom/gameplay/match_discovery.hpp](../../../include/gloom/gameplay/match_discovery.hpp)
- [include/gloom/gameplay/match_service_wire.hpp](../../../include/gloom/gameplay/match_service_wire.hpp)
- [src/gameplay/match_discovery.cpp](../../../src/gameplay/match_discovery.cpp)
- [src/gameplay/match_service_wire.cpp](../../../src/gameplay/match_service_wire.cpp)
- [tests/match_discovery_tests.cpp](../../../tests/match_discovery_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Separar datos y contratos de directory/store/connect en structs propios y operaciones concretas; elegir implementación local/HTTP/durable de forma
   explícita según rutas reales. No reconstruir una jerarquía equivalente con una tabla enorme de callbacks; usar callbacks solo para dependencias
   necesarias y pruebas.
2. Cambiar AsyncSliceMatchBrowser a solicitud propia con estado, dueño y generación que sobreviva al trabajo; conservar cancel/shutdown y publicación
   de errores sin bloquear UI. Los resultados tienen un propietario que cubre selección y resolve.
3. Migrar parse/encode de match_service_wire a tipos propios preservando campos JSON, validación, errores y compatibilidad con service actual.
   Mantener lease, revision, instance ownership, capacity y filtros de protocolo.
4. Actualizar consumidores e integración con 124/125 mediante contratos compartidos; no quitar reintentos acotados, refresh de bearer, publicaciones o
   recovery por expiración.

## Contratos que conservar

Anunciar/retirar exige el mismo dueño y revisiones monótonas; la salida no conserva string views del JSON temporal. Un browser destruido o nueva
búsqueda no recibe el resultado de una generación anterior.

## Validación focalizada

CTest existentes: `gloom.match_discovery`, `gloom.match_lobby`, `gloom.session`, `gloom.match_https`, `gloom.ui_flow`.

List/resolve, expiración, owner distinto, revisiones viejas, idempotencia, retries/backoff y async browser. Pruebas HTTP de entrada garantizan que los
contratos intermedios de 124/125 siguen enlazando; mantener soportadas ambas implementaciones reales.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

Tiempo/capacidad de list y refresh, memoria retenida por solicitud; medir carga de menú separada del frame. No exigir cero asignaciones durante
respuesta externa variable, sí evitar duplicar resultados completos.

Directory/browser/wire sin STL/future/shared ownership/herencia/PIMPL propios; contratos estables para 124/125 y comportamiento de
selección/publicación preservado.

Entregar informe `reports/cpp-performance-123/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 123: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/123-directorio.md. Ejecuta solo
> el hito 123 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
