# Hito 119: Datos CPU del renderer y retiro seguro de recursos GPU

Estado: **no iniciado**. Depende de: **113, 117 y 118**.
Objetivo: **CPU, cumplimiento C++ y sincronización GPU**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
21 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

Snapshot aún usa std::span y RenderInstance shared_ptr de poses; EnvironmentProbe mantiene vector/array y shared ownership. DiligentRenderer contiene
PIMPL, mapas/vectores, colas de upload y shaders HLSL embebidos. enqueue por valor transfiere payload: convertirlo a referencia sin conservar dueño
sería un error.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [include/gloom/render/scene.hpp](../../../include/gloom/render/scene.hpp)
- [include/gloom/render/gpu_assets.hpp](../../../include/gloom/render/gpu_assets.hpp)
- [include/gloom/render/renderer.hpp](../../../include/gloom/render/renderer.hpp)
- [include/gloom/render/lighting.hpp](../../../include/gloom/render/lighting.hpp)
- [src/backends/diligent_renderer.cpp](../../../src/backends/diligent_renderer.cpp)
- [include/gloom/backends/vulkan_present.hpp](../../../include/gloom/backends/vulkan_present.hpp)

## Archivos responsables de cierre

- [include/gloom/backends/diligent_renderer.hpp](../../../include/gloom/backends/diligent_renderer.hpp)
- [include/gloom/backends/vulkan_present.hpp](../../../include/gloom/backends/vulkan_present.hpp)
- [include/gloom/render/gpu_assets.hpp](../../../include/gloom/render/gpu_assets.hpp)
- [include/gloom/render/lighting.hpp](../../../include/gloom/render/lighting.hpp)
- [include/gloom/render/material_surface.hpp](../../../include/gloom/render/material_surface.hpp)
- [include/gloom/render/renderer.hpp](../../../include/gloom/render/renderer.hpp)
- [include/gloom/render/scene.hpp](../../../include/gloom/render/scene.hpp)
- [include/gloom/render/shadow_visibility.hpp](../../../include/gloom/render/shadow_visibility.hpp)
- [include/gloom/render/temporal.hpp](../../../include/gloom/render/temporal.hpp)
- [include/gloom/render/visibility.hpp](../../../include/gloom/render/visibility.hpp)
- [src/backends/diligent_renderer.cpp](../../../src/backends/diligent_renderer.cpp)
- [src/render/lighting.cpp](../../../src/render/lighting.cpp)
- [src/render/scene.cpp](../../../src/render/scene.cpp)
- [src/render/temporal.cpp](../../../src/render/temporal.cpp)
- [src/render/visibility.cpp](../../../src/render/visibility.cpp)
- [tests/gpu_asset_tests.cpp](../../../tests/gpu_asset_tests.cpp)
- [tests/lighting_tests.cpp](../../../tests/lighting_tests.cpp)
- [tests/material_render_tests.cpp](../../../tests/material_render_tests.cpp)
- [tests/render_scene_tests.cpp](../../../tests/render_scene_tests.cpp)
- [tests/temporal_tests.cpp](../../../tests/temporal_tests.cpp)
- [tests/visibility_tests.cpp](../../../tests/visibility_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Migrar scene/gpu_assets/environment/material/temporal y el almacenamiento propio de DiligentRenderer a Span/Array/tipos propios. Mantener una vista
   de pose por instancia según 113 y un propietario explícito de sonda; hacer lookups por índices/arrays preparados. Retirar PIMPL/herencia propias
   conservando Diligent/Vulkan y todas las capacidades actuales.
2. Reutilizar buffers CPU de draws, instancias, sombras, skin y uploads dentro de su capacidad. Distinguir copia CPU de matrices síncrona de
   almacenamiento GPU en vuelo. Las colas de uploads adquieren propiedad o retienen al propietario hasta consumir; no reemplazar transferencias por
   referencias temporales.
3. Centralizar el contrato de destrucción: última grabación/submit que utiliza el recurso, valor de timeline/fence que lo cubre, retiro cuando se
   completa y drain al apagar. Usar el mecanismo ya disponible en Diligent/Vulkan; añadir una cola mínima solo si falta, sin incorporar
   NoGraphicsAPIUtility. No hacer wait_idle cada frame.
4. Colocar creación junto a destrucción y verificar errores de creación inmediatamente. Quitar shadow state usado solo para duplicar validación
   Vulkan; conservar estado necesario para residencia/lifetime, activar validation en desarrollo.
5. Mover shaders embebidos a archivos por shader relacionado, VS/PS/variantes juntos; declaraciones CPU compartidas en su header correspondiente y
   tipos comunes en header neutral. Conservar source/defines/layout y resultados de compilación. No combinar shaders distintos ni optimizarlos aquí.

## Contratos que conservar

Mismo backend, GTX 1070, resolución nativa, materiales, sombras, alpha y temporal. Los headers externos pueden tener internamente STL/virtual; no son
permiso para uso propio prohibido. Recurso grabado pero aún no enviado también cuenta como uso pendiente.

## Validación focalizada

CTest existentes: `gloom.render_scene`, `gloom.gpu_assets`, `gloom.visibility`, `gloom.lighting`, `gloom.temporal`, `gloom.material_render`,
`gloom.sdl_smoke`, `gloom.vulkan_sync`, `gloom.hound_runtime`.

Release de recurso tras draw antes de submit, varios frames en vuelo, reload/resize y shutdown con upload pendiente; comprobar que destrucción ocurre
después de completar último uso. Validation sin VUID nuevos; layouts CPU/GPU comprobados y capturas completas antes/después.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

CPU begin/draw/end-present, buffers/capacidades, upload/skin bytes y frames en vuelo; misma GPU por pase. 1.000 frames estables dentro de capacidad
sin asignaciones propias de preparación/draw; declarar aparte trabajo interno de Diligent/driver.

Datos y API propios del renderer sin STL/shared_ptr/PIMPL/herencia/excepciones/RTTI; política única de lifetime y shutdown probada; shaders
reorganizados sin cambio visual ni cambio de renderer.

Entregar informe `reports/cpp-performance-119/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 119: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/119-renderer-y-lifetime.md. Ejecuta solo
> el hito 119 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
