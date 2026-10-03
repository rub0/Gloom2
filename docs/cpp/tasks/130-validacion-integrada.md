# Hito 130: Validación integrada y entrega del plan ejecutado

Estado: **no iniciado**. Depende de: **112–129 cerrados; bloqueos explícitos resueltos**.
Objetivo: **Cierre funcional, visual y de rendimiento**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
0 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

El 110 mantiene renderer/contenido y solo acredita ahorro de dos etapas. H06 sigue vigente; Hound H08 y una partida humana de ocho jugadores no forman
parte de esta migración.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [docs/ESTADO_ACTUAL.md](../../../docs/ESTADO_ACTUAL.md)
- [docs/art/hound/H06-contrato-presupuesto.md](../../../docs/art/hound/H06-contrato-presupuesto.md)
- [tools/art/measure_hound_budget.py](../../../tools/art/measure_hound_budget.py)
- [CMakeLists.txt](../../../CMakeLists.txt)
- [CMakePresets.json](../../../CMakePresets.json)

## Archivos responsables de cierre

Revisión transversal de los archivos del inventario; no tiene archivos propietarios nuevos.

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Revisar inventario/reglas/propietarios de toda la ruta y comparar con 112; ejecutar builds y suites completas Release/Debug tras el último cambio.
   Clasificar resultados por nombre y causa; ningún fallo nuevo puede esconderse en el total.
2. Repetir tres pasadas seriales Factory antes/después usando el baseline guardado y el sobre v17 con audio nulo/real. Conservar distribuciones,
   conteos, capacidades, CPU/GPU, memoria, uploads, missing, evictions, drops y capturas.
3. Revisar visuales Factory/personajes/armas/UI, animación/motion vectors y apagado/cancel/reload con frames/jobs/requests pendientes. No modificar
   arte/referencias ni el contrato H06.
4. Publicar tabla por hito: cambio, medida, cumplimiento, pruebas, limitación y commit. Actualizar estado/índice y retirar puentes o diagnósticos
   temporales que ya no se usan; conservar instrumentos mínimos reproducibles.

## Contratos que conservar

No equivaler ocho figuras animadas de un probe a ocho jugadores reales autoritativos. No declarar cero heap global, todas las imágenes idénticas o
suite verde si los datos solo acreditan un alcance parcial.

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

Suites completas, formato, flags efectivos, inventario y contratos; validación GPU y pruebas focales añadidas por cada propietario. Cierre requiere
informe y commit local verificados, nunca push implícito.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

Frame Hound: p99≤4 ms, máximo≤5 ms, 0/360 >5 ms; GPU resident≤384 MiB, missing/evictions/budgetlimited medidos cero según sobre vigente. Factory y
asignaciones muestran antes/después sin regresión fuera del ruido; ahorro por fase no se transforma en FPS prometidos.

Informe integrado con métricas/procedencia y limitaciones, todos los hitos cerrados con hashes o bloqueo declarado; cumplimiento global descrito sin
ambigüedad y workspace verificado.

Entregar informe `reports/cpp-performance-130/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 130: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/130-validacion-integrada.md. Ejecuta solo
> el hito 130 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
