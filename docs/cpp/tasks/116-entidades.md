# Hito 116: EntityRegistry sin mapas ni RTTI y con direcciones seguras

Estado: **no iniciado**. Depende de: **112 y 115 por secuencia**.
Objetivo: **Localidad, memoria y reglas C++**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
3 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

Registro por type_index/typeid y ComponentPool virtual; unordered_map por componente. Combatant guarda doce punteros a componentes, por lo que crecer
o compactar un array contiguo sin adaptar esos consumidores dejaría punteros colgando. EntityId es index+generation: dos enteros de 32 bits;
require_alive por valor es correcto.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [include/gloom/core/entity.hpp](../../../include/gloom/core/entity.hpp)
- [src/core/entity.cpp](../../../src/core/entity.cpp)
- [include/gloom/gameplay/components.hpp](../../../include/gloom/gameplay/components.hpp)
- [src/gameplay/vertical_slice.cpp](../../../src/gameplay/vertical_slice.cpp)
- [include/gloom/gameplay/component_replication.hpp](../../../include/gloom/gameplay/component_replication.hpp)
- [tests/entity_tests.cpp](../../../tests/entity_tests.cpp)

## Archivos responsables de cierre

- [include/gloom/core/entity.hpp](../../../include/gloom/core/entity.hpp)
- [src/core/entity.cpp](../../../src/core/entity.cpp)
- [tests/entity_tests.cpp](../../../tests/entity_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Enumerar tipos realmente usados y darles identificadores explícitos sin typeid ni jerarquía virtual. Elegir almacenamiento por slots/chunks
   estables para conservar los punteros existentes; si se elige almacenamiento contiguo que se mueve, convertir antes TODOS los cachés a handles/get y
   comprobar crecimiento/compactación. Registrar la alternativa y su coste antes de implementarla; no mezclar ambas sin necesidad.
2. Mantener create/destroy/alive, reutilización de índices con generación no cero y destrucción de componentes al retirar una entidad. Preparar free
   list/capacidad para que crear/destruir dentro de la reserva no asigne nodos de mapas; clear reutiliza buffers y conserva la invalidación de
   handles.
3. Reemplazar excepciones por asserts para duplicados y operaciones que exigen entidad viva; get/remove/destroy conservan sus resultados útiles para
   handles caducados. Mantener EntityId/otros IDs pequeños por valor; structs grandes por referencia y spans por valor.
4. Migrar composición, replicación y todos los consumidores a la política de estabilidad elegida. No convertir esto en un ECS arquetípico general ni
   imponer límite de ocho entidades a pruebas o servidor.

## Contratos que conservar

Nunca acceder a un componente de una generación anterior aunque se reutilice el índice. No permitir punteros invalidables en Combatant. Tipos públicos
con valores por defecto útiles y designated initializers en las llamadas migradas.

## Validación focalizada

CTest existentes: `gloom.entities`, `gloom.vertical_slice`, `gloom.vertical_slice_network`, `gloom.vertical_slice_transport`,
`gloom.network_replication`, `gloom.combat`, `gloom.legacy_movement`.

Crecer después de componer varios Combatant, retirar componente, destruir/recrear, clear, destrucción exactamente una vez y consulta de handle viejo;
ejecutar simulación/replicación sobre ese crecimiento. Assert inválido se prueba en Debug en proceso hijo si se añade un death check, sin abort manual
en producción.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

Comparar memoria por entidad/componente, tiempo de get y de crear/destruir a 8, 64 y 1.024 entidades con la misma composición; 1.000 ciclos dentro de
capacidad sin asignaciones de nodos. Separar inicialización de tick.

Sin mapas, RTTI, pool virtual, STL o excepciones en registro/composición migrados; estabilidad de direcciones o handles demostrada en todos los
usuarios y métricas sin regresión integrada.

Entregar informe `reports/cpp-performance-116/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 116: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/116-entidades.md. Ejecuta solo
> el hito 116 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
