# Hito 115: Partículas y efectos con salida persistente

Estado: **terminado**, 5 de octubre de 2026. Dependencias **113 y 114 terminadas**.
[Informe y medidas](../../../reports/cpp-performance-115/README.md).
Objetivo: **CPU y asignaciones**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
5 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Resultado ejecutado

Render añade directamente al Array persistente de main, conserva su prefijo y
recibe materiales por Span por valor sin retenerlos ni solapar salida. Recetas,
partículas y emisores propios; receta resuelta una vez al crear emisor. Los dos
estados de combate ya eran persistentes: no se inventa una cola adicional.
Se preservan RNG, vida/owner, orden cronológico, saturación y métricas. Carga
externa devuelve error sin excepciones y deja intacta la salida previa.
Precondiciones por assert; /GR- /EHs-c- en fuentes/prueba nativa.

1.000 update/render y 1.000 eventos calentados: cero asignaciones propias, con
control positivo de crecimiento. Juego: una asignación por frame → cero en
partículas. Estrés 2.048: mediana 48,8 → 37,2 µs; no se atribuye mejora global
de FPS. 49.080 registros comparados; máximo numérico 4,8e-7.
Builds/formato correctos; Release 50/54 y Debug 51/54, mismos fallos heredados
por causa. Detalles visuales, ruido, límites y reproducción en el informe.
Siguiente 116 no iniciado. Commit local, sin push: resolver con
`rtk git log -1 --oneline --grep='^hito 115:'`.

## Evidencia de partida

ParticleSystem ya reserva partículas, con capacidad por defecto 2.048, pero render devuelve un vector nuevo por llamada. La vida de emitters depende
de refresh/owner, y la receta se busca por texto. Las instancias se copian desde materiales para configurar billboards/ribbons.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [include/gloom/render/particles.hpp](../../../include/gloom/render/particles.hpp)
- [src/render/particles.cpp](../../../src/render/particles.cpp)
- [include/gloom/gameplay/combat_effects.hpp](../../../include/gloom/gameplay/combat_effects.hpp)
- [src/gameplay/combat_effects.cpp](../../../src/gameplay/combat_effects.cpp)
- [apps/gloom/main.cpp](../../../apps/gloom/main.cpp)
- [tests/animation_vfx_tests.cpp](../../../tests/animation_vfx_tests.cpp)

## Archivos responsables de cierre

- [include/gloom/gameplay/combat_effects.hpp](../../../include/gloom/gameplay/combat_effects.hpp)
- [include/gloom/render/particles.hpp](../../../include/gloom/render/particles.hpp)
- [src/gameplay/combat_effects.cpp](../../../src/gameplay/combat_effects.cpp)
- [src/render/particles.cpp](../../../src/render/particles.cpp)
- [tests/particle_storage_tests.cpp](../../../tests/particle_storage_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Cambiar render a resultado Array persistente del llamador y entrada Span por valor; reservar hasta la capacidad real. Mantener un único
   almacenamiento de salida hasta acabar visibilidad/draw, sin volver a concatenar mediante un vector temporal.
2. Migrar recetas/partículas/emisores a tipos propios y resolver identificadores de receta al crear/enlazar el emisor cuando sea estable. Conservar
   las reglas de saturación y los contadores spawned/expired/dropped; una capacidad fija no autoriza a eliminar efectos.
3. Reutilizar colas de efectos de combate y retirar lambdas/auto ordinario de esta ruta; no introducir partículas GPU ni cambiar materiales, sombras,
   blending, soft distance o distorsión. Mantener el generador y orden aleatorio deterministas.

## Contratos que conservar

La cancelación usa owner de una vida de entidad, no un índice reutilizado. El Span de materiales no se conserva. El resultado no solapa entradas ni
caduca antes del renderer.

## Validación focalizada

CTest existentes: `gloom.animation_vfx`, `gloom.combat`, `gloom.material_render`, `gloom.vertical_slice_smoke`, `gloom.vulkan_sync`.

0, 1, 2.048 y exceso de partículas; ráfagas con misma semilla, trails paralelos a cámara, nacimiento/muerte, translate/cancel/clear y refresh
faltante. Comparar transforms/colores/métricas y capturas con baseline.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

1.000 update/render tras reserva, misma carga y capacidad: cero asignaciones propias en render y ninguna pérdida adicional. Medir CPU
particles/effects y capacidad máxima; la carga de estrés no sustituye al benchmark integrado.

Salida persistente integrada en main y todos los callers; recetas inválidas detectadas al cargar; mismos efectos y límites, sin STL ni propietarios
compartidos en esta ruta.

Entregar informe `reports/cpp-performance-115/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 115: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/115-particulas.md. Ejecuta solo
> el hito 115 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
