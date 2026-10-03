# Hito 128: Cumplimiento global verificable y flags de compilación

Estado: **no iniciado**. Depende de: **113–127; ninguna frontera externa pendiente**.
Objetivo: **Cierre de migración, sin promesa automática de rendimiento**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
1 archivo propietario a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Bloqueo concreto registrado en 112

**No iniciado; cierre bloqueado** para la frontera externa, pendiente de decisión
expresa del usuario o alternativa equivalente demostrada. El cierre literal depende de resolver las fronteras fastgltf/Jolt, no de renombrar o esconder sus usos en vendor.
[Evidencia y probe](../../../reports/cpp-performance-112/compatibilidad.md).
No se aplica excepción por ausencia de respuesta. Conservar eventos/formatos y
dependencias; no marcar cerrado mientras esa condición siga pendiente.

## Evidencia de partida

Hoy solo las fuentes/pruebas focalizadas del 110 tienen no excepciones/RTTI; existen 188 archivos C++ propios y 50 CTest. Cambiar flags antes de
migrar todas las rutas provoca fallos de compile o errores de lifetime ocultos si se sustituyen catches sin conservar resultados.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [CMakeLists.txt](../../../CMakeLists.txt)
- [CMakePresets.json](../../../CMakePresets.json)
- [cmake/PatchDiligent.cmake](../../../cmake/PatchDiligent.cmake)
- [cmake/PackageMatchRuntime.cmake](../../../cmake/PackageMatchRuntime.cmake)
- [AGENTS.md](../../../AGENTS.md)

## Archivos responsables de cierre

- [tests/visual_capture_tests.cpp](../../../tests/visual_capture_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Actualizar inventario con todos los archivos nuevos/borrados y revisar semánticamente el código propio completo: headers STL/usos std excepto
   initializer_list de Span; auto ordinario, mapas, shared ownership, virtual/herencia/PIMPL, lambdas/templates complejos, RTTI y excepciones. Una
   búsqueda lexical es un índice, no la prueba de cierre.
2. Migrar residuos y tests/utilidades propios con su dueño de hito; los test doubles deben usar los contratos concretos nuevos. Mantener validación y
   cobertura, reemplazando tests de excepción por errores explícitos/asserts según frontera. No prohibir STL en Python/PowerShell ni reescribir el
   código interno de bibliotecas externas.
3. Centralizar flags finales sin /EHsc heredado, /GR- y /EHs-c- en MSVC, -fno-rtti/-fno-exceptions en otros compiladores, para TODOS los targets
   propios (engine, pipeline, object libs, storage, identity, https, apps y tests). Verificar opciones efectivas de todos los proyectos/fuentes, no
   solo CMake textual. Documentar cómo se construyen dependencias y cualquier incompatibilidad resuelta sin cambiar versiones/hardware por sorpresa.
4. Compilar desde configuración limpia Release/Debug; ejecutar formato y suites completas. Revisar la ABI de interfaces externas y modo de
   debug/asserts, Span/ByteSpan/GpuRange por valor, grandes structs por ref, defaults y los headers compartidos CPU/shader.

## Contratos que conservar

No declarar cumplimiento completo con exclusiones propias sin autorización. Las dependencias siguen teniendo su código interno; el alcance se declara
con precisión. No introducir tests que solo repiten líneas de la implementación ni modificar umbrales/referencias para hacerlos pasar.

## Validación focalizada

CTest existentes: `gloom.game_tickets`, `gloom.keycloak_identity`, `gloom.match_https`, `gloom.unit`, `gloom.jobs`, `gloom.entities`, `gloom.storage`,
`gloom.physics`, `gloom.render_scene`, `gloom.gpu_assets`, `gloom.visibility`, `gloom.lighting`, `gloom.temporal`, `gloom.vertical_slice`,
`gloom.vertical_slice_network`, `gloom.match_lobby`, `gloom.match_discovery`, `gloom.vertical_slice_transport`, `gloom.network`,
`gloom.network_protocol`, `gloom.network_replication`, `gloom.combat`, `gloom.session`, `gloom.legacy_movement`, `gloom.audio_no_device`,
`gloom.slice_server_smoke`, `gloom.audio`, `gloom.audio_network`, `gloom.legacy_pickups`, `gloom.pickup_presentation`, `gloom.legacy_arsenal`,
`gloom.ui`, `gloom.animation_vfx`, `gloom.skin_bounds`, `gloom.animation_network`, `gloom.animation_dedicated`, `gloom.ui_visual_review`,
`gloom.ui_flow`, `gloom.character_restoration`, `gloom.material_render`, `gloom.factory_restoration`, `gloom.assets`, `gloom.sdl_smoke`,
`gloom.visual_review`, `gloom.factory_visual_review`, `gloom.character_visual_review`, `gloom.network_scene_smoke`, `gloom.vertical_slice_smoke`,
`gloom.vulkan_sync`, `gloom.hound_runtime`.

50/50 casos existentes ejecutados en Release y Debug, más los casos significativos añadidos; comparar fallos conocidos por nombre/causa con 112.
Revisión de todas las compile options y comprobación global de formato/inventario.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

Lista de residuos cero en código propio o excepciones explícitamente autorizadas con alcance y evidencia; tamaño de binarios, memoria y tiempos de
arranque/frame antes/después, sin atribuir velocidad a eliminar auto.

Builds completos correctos, ningún nuevo fallo y evidencia de flags efectivos; ninguna regla pendiente disimulada. Si queda una excepción autorizada,
el informe describe cumplimiento con esa excepción y no cumplimiento literal total.

Entregar informe `reports/cpp-performance-128/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 128: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/128-cierre-cpp.md. Ejecuta solo
> el hito 128 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
