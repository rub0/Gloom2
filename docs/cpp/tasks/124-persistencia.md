# Hito 124: Persistencia durable de partidas con datos propios

Estado: **no iniciado**. Depende de: **117 y 123**.
Objetivo: **Cumplimiento C++ sin pérdida de datos**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
1 archivo propietario a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

La persistencia está en src/gameplay/durable_match_service.cpp, no en un header durable independiente; consume el contrato de match_discovery y
mantiene errores de I/O, ownership, revisions y recuperación.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [src/gameplay/durable_match_service.cpp](../../../src/gameplay/durable_match_service.cpp)
- [include/gloom/gameplay/match_discovery.hpp](../../../include/gloom/gameplay/match_discovery.hpp)
- [tests/match_discovery_tests.cpp](../../../tests/match_discovery_tests.cpp)
- [CMakeLists.txt](../../../CMakeLists.txt)

## Archivos responsables de cierre

- [src/gameplay/durable_match_service.cpp](../../../src/gameplay/durable_match_service.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Migrar almacenamiento/string/path/result de la implementación durable y utilidades de archivos a tipos propios/C/native; usar el contrato concreto
   de 123. No añadir una base de datos ni un nuevo framework de persistencia.
2. Conservar store/erase atómico, validación y comparación de instance/revision/mutation_id, escritura temporal y reemplazo/flush según la
   implementación existente; estados corruptos producen errores externos claros.
3. Mantener sincronización entre operaciones y readers; views obtenidas al listar no apuntan a buffers de archivo liberados. Actualizar los tests de
   directorio y build de gloom_match_storage sin excepciones/RTTI.

## Contratos que conservar

Nunca borrar o sobrescribir almacenamiento válido para silenciar un fallo. Si no se puede reemplazar/flush el archivo, retornar error y conservar la
versión válida anterior.

## Validación focalizada

CTest existentes: `gloom.match_discovery`, `gloom.match_https`.

Reabrir estado escrito por la versión de entrada, interrupción antes/durante replace según seam de I/O, archivo truncado/corrupto, revisiones
duplicadas, retirada de otro owner y rutas Unicode. Verificar estado final tras fallos, no solo el código retornado.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

Latencia y pico de memoria store/list/get en fixtures existentes y carga ampliada comparable; preservar durability aunque un cambio más rápido no
cumpla. No atribuir aquí mejora de FPS.

Persistencia y pruebas propias migradas con formato compatible, errores externos y durabilidad intactos; no STL/herencia/PIMPL/excepciones/RTTI en este módulo.

Entregar informe `reports/cpp-performance-124/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 124: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/124-persistencia.md. Ejecuta solo
> el hito 124 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
