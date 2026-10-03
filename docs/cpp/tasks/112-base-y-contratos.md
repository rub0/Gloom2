# Hito 112: Base reproducible, costes y contratos de propiedad

Estado: **no iniciado**. Depende de: **111**.
Objetivo: **Medición y diseño verificable**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
7 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

El 110 mide visibilidad/iluminación, pero no asignaciones completas de poses, grupos, partículas o snapshots. Persisten opciones /EHsc en varios
targets; Array construye toda la capacidad y no admite directamente tipos sin constructor por defecto. Jolt exige interfaces virtuales para filtros y
listeners; fastgltf y Diligent exponen tipos propios que requieren comprobar la frontera con STL.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [CMakeLists.txt](../../../CMakeLists.txt)
- [CMakePresets.json](../../../CMakePresets.json)
- [vcpkg.json](../../../vcpkg.json)
- [apps/gloom/performance.hpp](../../../apps/gloom/performance.hpp)
- [include/gloom/core/array.hpp](../../../include/gloom/core/array.hpp)
- [include/gloom/core/span.hpp](../../../include/gloom/core/span.hpp)

## Archivos responsables de cierre

- [apps/gloom/performance.hpp](../../../apps/gloom/performance.hpp)
- [include/gloom/core/array.hpp](../../../include/gloom/core/array.hpp)
- [include/gloom/core/clock.hpp](../../../include/gloom/core/clock.hpp)
- [include/gloom/core/span.hpp](../../../include/gloom/core/span.hpp)
- [include/gloom/core/types.hpp](../../../include/gloom/core/types.hpp)
- [src/core/clock.cpp](../../../src/core/clock.cpp)
- [tests/storage_tests.cpp](../../../tests/storage_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Repetir la línea base del 110 en el HEAD de entrada: Factory 1080p nativo, 120 frames de calentamiento, 360 medidos, tres pasadas seriales;
   conservar todas las distribuciones, etapas, contadores y configuración. Ejecutar también el sobre Hound v17 con audio nulo y dispositivo real según
   el protocolo común.
2. Añadir solo la instrumentación necesaria para atribuir new/new[], bytes, capacidad máxima y tiempo a poses, grupos/jobs, partículas, entidades,
   snapshots y UI. Los contadores deben cubrir los workers y excluir escritura de informes; controlar con una reserva que crece. Informar aparte
   malloc/realloc propios, memoria del proceso y recursos GPU: interceptar new no demuestra cero asignaciones globales.
3. Registrar por operación quién posee los datos, si la llamada es síncrona o diferida, cuándo caduca cada Span y qué evento permite reutilizar
   memoria. Medir tamaños y alineaciones de todas las capturas de jobs antes de fijar FixedFunction; inventariar entidades/componentes y tamaños
   reales antes de fijar capacidades.
4. Comprobar las versiones instaladas y las opciones de build de Jolt, Diligent, fastgltf y GNS frente a no excepciones/RTTI y al uso directo de STL.
   Para Jolt investigar los filtros incorporados y los eventos observables sin callbacks propios; hacer una prueba mínima que conserve
   entered/stayed/exited y multithreading. Si no existe alternativa equivalente, dejar 120 bloqueado con evidencia y solicitar una decisión concreta
   sobre esa regla antes de su implementación. No conceder una excepción ni sustituir la dependencia unilateralmente.
5. Definir el sustituto mínimo de textos/resultados/archivos cuando su primer consumidor lo necesite (117/124/125), y las primitivas nativas de
   sincronización para 114. Reutilizar types/Span/Array/clock; no crear una biblioteca estándar general. Publicar escenarios y comandos exactos para
   todas las medidas focalizadas.

## Contratos que conservar

No alterar el trabajo del benchmark para mejorar sus números; separar instrumentación activada/desactivada. No esconder código propio en vendor para
eludir reglas. Los datos externos corruptos y los fallos de inicialización continúan produciendo errores útiles.

## Validación focalizada

CTest existentes: `gloom.storage`, `gloom.jobs`, `gloom.visibility`, `gloom.lighting`, `gloom.vulkan_sync`, `gloom.hound_runtime`.

Builds completos y suites Release/Debug para fijar una base actual; controles positivos/negativos de contadores en todos los hilos. Verificar además
que el escenario Hound conserva 53 instancias skinned visibles y todas las máscaras. Este hito no certifica una partida autoritativa de ocho
jugadores.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

Tabla antes de optimizar: CPU por etapa, media/p50/p95/p99/máximo, asignaciones y bytes por etapa, memoria residente y máximos de capacidad. Cada
contador declara qué rutas cubre y cuáles quedan fuera.

Escenarios repetibles, propietario/caducidad documentados para las APIs diferidas, inventario actualizado y resolución o bloqueo explícito de las
fronteras externas. Eliminar instrumentación invasiva que distorsione el escenario; conservar la mínima opción de diagnóstico útil.

Entregar informe `reports/cpp-performance-112/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 112: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/112-base-y-contratos.md. Ejecuta solo
> el hito 112 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
