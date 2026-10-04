# Hito 113: Poses y animación sin temporales por frame

Estado: **terminado**, 4 de octubre de 2026. Depende de: **112**.
Objetivo: **CPU, memoria y propiedad**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
11 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

sample_animation/rest_pose/blend_poses devuelven arrays nuevos; pose_worlds reconstruye parents/queue/result en cada evaluación. skin_pose crea un
shared_ptr y dos arrays por enlace. main comparte poses entre primitivas hermanas y conserva previous_animated_instances para motion vectors; esa
reutilización debe mantenerse.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [include/gloom/assets/animation.hpp](../../../include/gloom/assets/animation.hpp)
- [src/assets/animation.cpp](../../../src/assets/animation.cpp)
- [include/gloom/gameplay/character_animation.hpp](../../../include/gloom/gameplay/character_animation.hpp)
- [src/gameplay/character_animation.cpp](../../../src/gameplay/character_animation.cpp)
- [include/gloom/render/scene.hpp](../../../include/gloom/render/scene.hpp)
- [apps/gloom/main.cpp](../../../apps/gloom/main.cpp)

## Archivos responsables de cierre

- [apps/gloom/animation_review.hpp](../../../apps/gloom/animation_review.hpp)
- [include/gloom/assets/animation.hpp](../../../include/gloom/assets/animation.hpp)
- [include/gloom/gameplay/character_animation.hpp](../../../include/gloom/gameplay/character_animation.hpp)
- [src/assets/animation.cpp](../../../src/assets/animation.cpp)
- [src/gameplay/character_animation.cpp](../../../src/gameplay/character_animation.cpp)
- [tests/animation_network_tests.cpp](../../../tests/animation_network_tests.cpp)
- [tests/animation_vfx_tests.cpp](../../../tests/animation_vfx_tests.cpp)
- [tests/skin_bounds_tests.cpp](../../../tests/skin_bounds_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Preparar padres y orden topológico una vez por rig/generación al cargarlo, validando ciclos, índices, bind matrices, tiempos y pesos en esa
   frontera. Mantener rig_transform/rig_inverse, slerp, wrap/clamp y orden de evaluación actuales.
2. Convertir las operaciones de poses a salidas propiedad del animador/llamador con Array y vistas Span; reservar al enlazar o cambiar rig. Reutilizar
   poses local/blend/world y scratch de cada actor; las funciones no retienen vistas de initializer lists ni comparten scratch entre evaluaciones
   concurrentes.
3. Dar a cada actor/enlace mesh_node dos juegos persistentes de SkinPose (actual/anterior), compartidos por sus primitivas mediante vistas/handles sin
   shared_ptr. Mantener la pose anterior inmutable hasta terminar todos sus consumidores CPU; verificar en el renderer cuándo copia matrices a
   almacenamiento GPU. La reutilización del almacenamiento GPU espera su frame/timeline, independientemente de la vida de las matrices CPU.
4. Actualizar también armas, brazos FPS, máscaras TPS, bind/death poses, equip, cortes, respawn y cambio de rig; reconstruir límites con las cajas de
   joints ya preparadas. No volver al recorrido de todos los vértices por frame ni desactivar animación para reducir CPU.

## Contratos que conservar

La identidad de la caché incluye actor, generación de rig y enlace de skin; nunca solo nombre de clip o mesh. Un corte no hereda una pose anterior de
otro actor. La propiedad de RenderInstance cambia junto con todos sus llamadores, visibilidad, sombras y pruebas.

## Validación focalizada

CTest existentes: `gloom.animation_vfx`, `gloom.skin_bounds`, `gloom.character_restoration`, `gloom.animation_network`, `gloom.animation_dedicated`,
`gloom.material_render`, `gloom.visibility`, `gloom.vulkan_sync`, `gloom.hound_runtime`.

Comparar matrices, attachments y límites contra la evaluación de entrada en tiempos de clips, loop, blending y discontinuidades; dos actores del mismo
rig con poses distintas; cambio de rig y destrucción tras último uso. Revisar capturas de personaje/arma y temporal con el comparador común.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

Después de preparar rig/capacidad: 1.000 evaluaciones del mismo rig sin nuevas asignaciones en sample/blend/world/skin; crecimiento/cambio de rig
fuera de ese intervalo medidos aparte. Antes/después de CPU animation poses y bounds en Factory/Hound con la misma carga.

Todas las poses actuales/anteriores tienen propietario y caducidad comprobados; no quedan STL/shared_ptr/excepciones/RTTI/auto ordinario en la ruta
migrada. Informar ahorro CPU real aunque FPS no cambie.

Entregar informe `reports/cpp-performance-113/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 113: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Resultado verificado

[Informe 113](../../../reports/cpp-performance-113/README.md): rig preparado una
vez, salidas propias reutilizadas y dos slots de skin por actor/enlace. Cero new
en poses/bounds en siete pasadas instrumentadas; cuatro rigs reales × dos actores
× 1.000 y control concurrente/growth/enlace. Comparación numérica de 3.495.720
floats y 1.200 frames + siete vistas alineadas con el comparador común.
Builds completos y comprobaciones finales correctos; suites 49/53 Release y
50/53 Debug con los mismos fallos heredados. Véanse tiempos, scopes y limitaciones
en el informe; no certifica FPS ni cumplimiento global.

Los tres archivos nuevos son Matrix4, CombatantView y pose_storage. Los tests
mixtos conservan piezas heredadas de importer/partículas/red para 117/115/122 y
128. Scene y matemática transitiva siguen en 119; residencia en 118. Se migra
su uso de poses sin declarar esos módulos cerrados. Los bloqueos 117/120/128
siguen sin autorización nueva. Commit local obligatorio según AGENTS; sin push.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/113-poses.md. Ejecuta solo
> el hito 113 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
