# Inventario C++ y responsables de cierre

Base: `5b96a911b69528b7381116ce817af7bdef0dbcc3`. 188 archivos versionados propios, 34.961 líneas,
50 CTest registrados en build Release. Enumeración por `git ls-files`, extensiones
.cpp/.hpp/.h/.c/.cc/.cxx; todos pertenecen a include/src/apps/tests. No existen
AGENTS anidados ni archivos shader independientes versionados; HLSL embebido
en DiligentRenderer se trata en 119. Build/cache/vcpkg no son código propio.

Actualización 112: **193 archivos propios, 35.412 líneas y 52 CTest**.
Los cinco archivos nuevos tienen responsable 112; líneas de las tablas actualizadas
al workspace del 112. Las señales de búsqueda siguientes conservan su base 110.
`tools/perf/measure_cpp_baseline.py` es herramienta no C++ del 112.

Actualización 113: **196 archivos propios, 36.004 líneas y 53 CTest**.
Tres archivos nuevos pertenecen al 113: Matrix4, CombatantView y pose_storage.
Las líneas de las tablas se actualizan al workspace final; las señales históricas
conservan su base 110, excepto el resultado propio de la ruta de poses.
Los tests mixtos del 113 conservan bloques legacy de 115/117/122; sus dueños y el
cierre transversal 128 deben terminarlos. Un cierre de poses no certifica esos bloques.

Actualización 114: **197 archivos propios, 36,393 líneas y 53 CTest**.
FixedFunction pertenece al 114, que pasa a cuatro archivos propietarios.
Scheduler y prueba propia cerrados; datos/futuros de assets siguen en 117/118.
Las señales restantes conservan su base histórica; las líneas reflejan el workspace final.
`tools/perf/measure_job_batches.py` es herramienta no C++ del 114.

Actualización 115: **198 archivos propios, 36,720 líneas y 54 CTest**.
La prueba autónoma particle_storage pertenece al 115, que pasa a cinco archivos.
Partículas/efectos sin STL directo, excepciones/RTTI ni temporales de render.
Los headers de Scene/animación y bloques mixtos conservan dueños 117/119/122.
Array sigue bajo 112; append/push y limpieza trivial se adaptan en 115, con
contratos de recursos/regrowth verificados. Las líneas reflejan el workspace
final; las señales fuera del módulo conservan su base histórica.

Actualización 116: **198 archivos propios, 36,801 líneas y 54 CTest**.
Registro/prueba propios cerrados, sin archivos ni targets CTest nuevos.
Composición/replicación directa adaptadas; adapters físicos reciben IDs.
LegacyArsenal, World/Subsystem y WorldSnapshot conservan dueños 120/122/127.
Las líneas se actualizan al workspace final; señales históricas fuera del módulo.

Actualización parcial 117: **198 archivos propios, 36,960 líneas y 54 CTest**.
Índice/recorrido de AssetCatalog y evaluación de bind con almacenamiento propio;
metadatos, VFS y pipeline aún legacy. No hay archivos C++ ni CTest nuevos.
El 117 permanece abierto; las señales históricas no acreditan cierre de sus 19 archivos.

## Señales de búsqueda, no violaciones contabilizadas

| Señal lexical | Archivos con coincidencias |
| --- | ---: |
| STL | 165 |
| auto | 97 |
| exceptions | 84 |
| RTTI | 1 |
| maps | 27 |
| owners | 65 |
| virtual | 29 |
| pimpl | 24 |

STL cuenta headers estándar C++ y `std::`, excepto initializer_list de Span.
auto cuenta también comentarios/tipos de retorno; exceptions cuenta try/throw/catch
en textos/comentarios; virtual incluye override. Mapas incluye sets. PIMPL detecta
Impl, pero puede coincidir con implementación interna que no sea una interfaz.
Las señales no son costes medidos ni sustituyen revisión semántica. La falta de
una señal no garantiza cumplimiento. RTTI directo está concentrado en entity.hpp.

Se revisan además lambdas/templates complejos, copias, punteros conservados,
asignaciones, defaults/designated initializers, parámetros Span/ByteSpan/GpuRange
y creación/destrucción/error/lifetime en cada ficha; no son conteos lexicales fiables.

## Asignación exhaustiva

Cada archivo tiene **un responsable de cierre**. Puede editarse antes para adaptar
una firma del hito activo; esa edición no cierra su migración completa. Las fichas
128/129/130 revisan transversalmente todo. Actualizar esta lista si se crean/eliminan
archivos; cero archivos nuevos sin responsable al cerrar 128.

### 112: [Base reproducible, costes y contratos de propiedad](112-base-y-contratos.md) — 12 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [apps/gloom/performance.hpp](../../../apps/gloom/performance.hpp) | 105 | Sin coincidencia lexical |
| [include/gloom/core/array.hpp](../../../include/gloom/core/array.hpp) | 109 | Sin coincidencia lexical |
| [include/gloom/core/clock.hpp](../../../include/gloom/core/clock.hpp) | 6 | Sin coincidencia lexical |
| [include/gloom/core/span.hpp](../../../include/gloom/core/span.hpp) | 44 | Sin coincidencia lexical |
| [include/gloom/core/types.hpp](../../../include/gloom/core/types.hpp) | 13 | Sin coincidencia lexical |
| [src/core/clock.cpp](../../../src/core/clock.cpp) | 29 | Sin coincidencia lexical |
| [tests/storage_tests.cpp](../../../tests/storage_tests.cpp) | 113 | Sin coincidencia lexical |

| [include/gloom/core/allocation_profile.hpp](../../../include/gloom/core/allocation_profile.hpp) | 44 | Sin coincidencia lexical propia |
| [src/core/allocation_profile.cpp](../../../src/core/allocation_profile.cpp) | 137 | Sin coincidencia lexical propia |
| [src/core/allocation_new.cpp](../../../src/core/allocation_new.cpp) | 23 | Sin coincidencia lexical propia |
| [tests/allocation_profile_tests.cpp](../../../tests/allocation_profile_tests.cpp) | 72 | Sin coincidencia lexical propia |
| [tests/dependency_contract_tests.cpp](../../../tests/dependency_contract_tests.cpp) | 96 | Sin coincidencia lexical propia |

### 113: [Poses y animación sin temporales por frame](113-poses.md) — 11 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [apps/gloom/animation_review.hpp](../../../apps/gloom/animation_review.hpp) | 56 | Sin coincidencia lexical propia; dependencias legacy según informe 113 |
| [include/gloom/assets/animation.hpp](../../../include/gloom/assets/animation.hpp) | 83 | Sin coincidencia lexical propia; dependencias legacy según informe 113 |
| [include/gloom/gameplay/character_animation.hpp](../../../include/gloom/gameplay/character_animation.hpp) | 44 | Sin coincidencia lexical propia; dependencias legacy según informe 113 |
| [src/assets/animation.cpp](../../../src/assets/animation.cpp) | 179 | Sin coincidencia lexical propia; dependencias legacy según informe 113 |
| [src/gameplay/character_animation.cpp](../../../src/gameplay/character_animation.cpp) | 294 | Sin coincidencia lexical propia; dependencias legacy según informe 113 |
| [tests/animation_network_tests.cpp](../../../tests/animation_network_tests.cpp) | 226 | STL, auto, exceptions, owners |
| [tests/animation_vfx_tests.cpp](../../../tests/animation_vfx_tests.cpp) | 302 | STL, auto, exceptions |
| [tests/skin_bounds_tests.cpp](../../../tests/skin_bounds_tests.cpp) | 130 | Sin coincidencia lexical propia; dependencias legacy según informe 113 |
| [include/gloom/core/matrix.hpp](../../../include/gloom/core/matrix.hpp) | 35 | Sin coincidencia lexical propia |
| [include/gloom/gameplay/combatant_view.hpp](../../../include/gloom/gameplay/combatant_view.hpp) | 48 | Sin coincidencia lexical propia |
| [tests/pose_storage_tests.cpp](../../../tests/pose_storage_tests.cpp) | 163 | Sin coincidencia lexical propia |

### 114: [JobSystem y FixedFunction con grupos reutilizables](114-jobs.md) — 4 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [include/gloom/core/job_system.hpp](../../../include/gloom/core/job_system.hpp) | 110 | Sin coincidencia lexical propia; Win32/C, sin excepciones/RTTI |
| [include/gloom/core/fixed_function.hpp](../../../include/gloom/core/fixed_function.hpp) | 108 | Sin coincidencia lexical propia; Win32/C, sin excepciones/RTTI |
| [src/core/job_system.cpp](../../../src/core/job_system.cpp) | 200 | Sin coincidencia lexical propia; Win32/C, sin excepciones/RTTI |
| [tests/job_system_tests.cpp](../../../tests/job_system_tests.cpp) | 313 | Sin coincidencia lexical propia; Win32/C, sin excepciones/RTTI |

### 115: [Partículas y efectos con salida persistente](115-particulas.md) — 5 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [include/gloom/gameplay/combat_effects.hpp](../../../include/gloom/gameplay/combat_effects.hpp) | 28 | Sin coincidencia lexical propia; dependencias legacy según informe 115 |
| [include/gloom/render/particles.hpp](../../../include/gloom/render/particles.hpp) | 76 | Sin coincidencia lexical propia; dependencias legacy según informe 115 |
| [src/gameplay/combat_effects.cpp](../../../src/gameplay/combat_effects.cpp) | 90 | Sin coincidencia lexical propia; dependencias legacy según informe 115 |
| [src/render/particles.cpp](../../../src/render/particles.cpp) | 342 | Sin coincidencia lexical propia; dependencias legacy según informe 115 |
| [tests/particle_storage_tests.cpp](../../../tests/particle_storage_tests.cpp) | 220 | Sin coincidencia lexical propia; Win32/C, sin excepciones/RTTI |

### 116: [EntityRegistry sin mapas ni RTTI y con direcciones seguras](116-entidades.md) — 3 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [include/gloom/core/entity.hpp](../../../include/gloom/core/entity.hpp) | 155 | Sin coincidencia lexical propia; sin excepciones/RTTI |
| [src/core/entity.cpp](../../../src/core/entity.cpp) | 71 | Sin coincidencia lexical propia; sin excepciones/RTTI |
| [tests/entity_tests.cpp](../../../tests/entity_tests.cpp) | 190 | Sin coincidencia lexical propia; sin excepciones/RTTI |

### 117: [Datos de assets, VFS, importación y cooker](117-assets-y-cooker.md) — 19 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [apps/gloom_asset_cooker/main.cpp](../../../apps/gloom_asset_cooker/main.cpp) | 32 | STL, auto, exceptions, virtual |
| [include/gloom/assets/asset.hpp](../../../include/gloom/assets/asset.hpp) | 110 | STL, maps |
| [include/gloom/assets/asset_cooker.hpp](../../../include/gloom/assets/asset_cooker.hpp) | 19 | STL |
| [include/gloom/assets/gltf_importer.hpp](../../../include/gloom/assets/gltf_importer.hpp) | 129 | STL |
| [include/gloom/assets/mesh_processing.hpp](../../../include/gloom/assets/mesh_processing.hpp) | 27 | STL |
| [include/gloom/assets/rig.hpp](../../../include/gloom/assets/rig.hpp) | 129 | Evaluación sin std directo; ImportedScene sigue legacy |
| [include/gloom/assets/scene_catalog.hpp](../../../include/gloom/assets/scene_catalog.hpp) | 24 | STL |
| [include/gloom/assets/texture_asset.hpp](../../../include/gloom/assets/texture_asset.hpp) | 26 | STL |
| [include/gloom/gameplay/factory_scene.hpp](../../../include/gloom/gameplay/factory_scene.hpp) | 34 | STL, owners |
| [src/assets/asset.cpp](../../../src/assets/asset.cpp) | 397 | STL, auto, exceptions, maps |
| [src/assets/asset_cooker.cpp](../../../src/assets/asset_cooker.cpp) | 120 | STL, auto, maps |
| [src/assets/gltf_importer.cpp](../../../src/assets/gltf_importer.cpp) | 1351 | STL, auto, exceptions |
| [src/assets/mesh_processing.cpp](../../../src/assets/mesh_processing.cpp) | 214 | STL, auto |
| [src/assets/scene_catalog.cpp](../../../src/assets/scene_catalog.cpp) | 127 | STL, auto, maps, virtual |
| [src/assets/texture_asset.cpp](../../../src/assets/texture_asset.cpp) | 230 | STL, auto, owners |
| [src/gameplay/factory_scene.cpp](../../../src/gameplay/factory_scene.cpp) | 250 | STL, auto, exceptions, owners |
| [tests/asset_pipeline_tests.cpp](../../../tests/asset_pipeline_tests.cpp) | 623 | STL, auto, exceptions, maps, virtual |
| [tests/character_restoration_tests.cpp](../../../tests/character_restoration_tests.cpp) | 120 | STL, auto, exceptions |
| [tests/factory_restoration_tests.cpp](../../../tests/factory_restoration_tests.cpp) | 147 | STL, auto, exceptions |

### 118: [Carga asíncrona y residencia con propiedad explícita](118-carga-y-residencia.md) — 6 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [include/gloom/assets/asset_loader.hpp](../../../include/gloom/assets/asset_loader.hpp) | 58 | STL, maps |
| [include/gloom/assets/residency_coordinator.hpp](../../../include/gloom/assets/residency_coordinator.hpp) | 96 | STL, maps, owners |
| [include/gloom/assets/scene_gpu_bridge.hpp](../../../include/gloom/assets/scene_gpu_bridge.hpp) | 35 | STL |
| [src/assets/asset_loader.cpp](../../../src/assets/asset_loader.cpp) | 121 | STL, auto, exceptions, owners |
| [src/assets/residency_coordinator.cpp](../../../src/assets/residency_coordinator.cpp) | 475 | STL, auto, exceptions, owners |
| [src/assets/scene_gpu_bridge.cpp](../../../src/assets/scene_gpu_bridge.cpp) | 123 | STL, auto |

### 119: [Datos CPU del renderer y retiro seguro de recursos GPU](119-renderer-y-lifetime.md) — 21 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [include/gloom/backends/diligent_renderer.hpp](../../../include/gloom/backends/diligent_renderer.hpp) | 57 | STL, owners, virtual, pimpl |
| [include/gloom/backends/vulkan_present.hpp](../../../include/gloom/backends/vulkan_present.hpp) | 42 | Sin coincidencia lexical |
| [include/gloom/render/gpu_assets.hpp](../../../include/gloom/render/gpu_assets.hpp) | 86 | STL |
| [include/gloom/render/lighting.hpp](../../../include/gloom/render/lighting.hpp) | 112 | STL, owners |
| [include/gloom/render/material_surface.hpp](../../../include/gloom/render/material_surface.hpp) | 55 | STL, auto |
| [include/gloom/render/renderer.hpp](../../../include/gloom/render/renderer.hpp) | 87 | STL, virtual |
| [include/gloom/render/scene.hpp](../../../include/gloom/render/scene.hpp) | 119 | STL, owners |
| [include/gloom/render/shadow_visibility.hpp](../../../include/gloom/render/shadow_visibility.hpp) | 23 | Sin coincidencia lexical |
| [include/gloom/render/temporal.hpp](../../../include/gloom/render/temporal.hpp) | 87 | STL |
| [include/gloom/render/visibility.hpp](../../../include/gloom/render/visibility.hpp) | 61 | Sin coincidencia lexical |
| [src/backends/diligent_renderer.cpp](../../../src/backends/diligent_renderer.cpp) | 2669 | STL, auto, exceptions, maps, owners, pimpl |
| [src/render/lighting.cpp](../../../src/render/lighting.cpp) | 154 | Sin coincidencia lexical |
| [src/render/scene.cpp](../../../src/render/scene.cpp) | 108 | STL, auto, exceptions |
| [src/render/temporal.cpp](../../../src/render/temporal.cpp) | 111 | STL |
| [src/render/visibility.cpp](../../../src/render/visibility.cpp) | 196 | Sin coincidencia lexical |
| [tests/gpu_asset_tests.cpp](../../../tests/gpu_asset_tests.cpp) | 57 | STL, auto, exceptions |
| [tests/lighting_tests.cpp](../../../tests/lighting_tests.cpp) | 70 | Sin coincidencia lexical |
| [tests/material_render_tests.cpp](../../../tests/material_render_tests.cpp) | 134 | STL, auto, exceptions |
| [tests/render_scene_tests.cpp](../../../tests/render_scene_tests.cpp) | 90 | STL, auto, exceptions |
| [tests/temporal_tests.cpp](../../../tests/temporal_tests.cpp) | 47 | Sin coincidencia lexical |
| [tests/visibility_tests.cpp](../../../tests/visibility_tests.cpp) | 83 | Sin coincidencia lexical |

### 120: [Física y consultas con eventos reutilizables](120-fisica.md) — 6 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [include/gloom/backends/jolt_world.hpp](../../../include/gloom/backends/jolt_world.hpp) | 64 | STL, owners, virtual, pimpl |
| [include/gloom/physics/components.hpp](../../../include/gloom/physics/components.hpp) | 101 | STL, exceptions |
| [include/gloom/physics/triangle_query.hpp](../../../include/gloom/physics/triangle_query.hpp) | 40 | STL, auto |
| [include/gloom/physics/world.hpp](../../../include/gloom/physics/world.hpp) | 177 | STL, owners, virtual |
| [src/backends/jolt_world.cpp](../../../src/backends/jolt_world.cpp) | 848 | STL, auto, exceptions, maps, owners, virtual, pimpl |
| [tests/physics_tests.cpp](../../../tests/physics_tests.cpp) | 213 | STL, auto, exceptions, virtual |

### 121: [Audio sin propietarios compartidos ni temporales de mezcla](121-audio.md) — 12 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [apps/gloom/audio_review.hpp](../../../apps/gloom/audio_review.hpp) | 82 | STL, auto, exceptions |
| [include/gloom/audio/events.hpp](../../../include/gloom/audio/events.hpp) | 112 | STL, auto |
| [include/gloom/audio/mixer.hpp](../../../include/gloom/audio/mixer.hpp) | 76 | STL, owners, virtual |
| [include/gloom/gameplay/audio_events.hpp](../../../include/gloom/gameplay/audio_events.hpp) | 13 | STL |
| [include/gloom/gameplay/audio_presentation.hpp](../../../include/gloom/gameplay/audio_presentation.hpp) | 41 | STL, owners |
| [src/audio/mixer.cpp](../../../src/audio/mixer.cpp) | 197 | STL, auto, exceptions, owners, virtual |
| [src/backends/sdl_audio.cpp](../../../src/backends/sdl_audio.cpp) | 61 | STL, owners, virtual |
| [src/gameplay/audio_events.cpp](../../../src/gameplay/audio_events.cpp) | 140 | STL, auto |
| [src/gameplay/audio_presentation.cpp](../../../src/gameplay/audio_presentation.cpp) | 142 | STL, auto, exceptions, owners |
| [tests/audio_backend_tests.cpp](../../../tests/audio_backend_tests.cpp) | 24 | STL, auto, exceptions, owners |
| [tests/audio_network_tests.cpp](../../../tests/audio_network_tests.cpp) | 95 | STL, auto, exceptions, maps |
| [tests/audio_tests.cpp](../../../tests/audio_tests.cpp) | 259 | STL, auto, exceptions, owners |

### 122: [Red, replicación y gameplay con buffers acotados](122-red-y-simulacion.md) — 54 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [apps/gloom_slice_server/main.cpp](../../../apps/gloom_slice_server/main.cpp) | 180 | STL, auto, exceptions, owners |
| [include/gloom/backends/gns_transport.hpp](../../../include/gloom/backends/gns_transport.hpp) | 43 | STL, owners, virtual, pimpl |
| [include/gloom/gameplay/character_presentation.hpp](../../../include/gloom/gameplay/character_presentation.hpp) | 79 | STL |
| [include/gloom/gameplay/component_replication.hpp](../../../include/gloom/gameplay/component_replication.hpp) | 85 | STL, auto, exceptions |
| [include/gloom/gameplay/components.hpp](../../../include/gloom/gameplay/components.hpp) | 234 | STL, auto, exceptions |
| [include/gloom/gameplay/first_person.hpp](../../../include/gloom/gameplay/first_person.hpp) | 33 | Sin coincidencia lexical |
| [include/gloom/gameplay/first_person_presentation.hpp](../../../include/gloom/gameplay/first_person_presentation.hpp) | 50 | STL |
| [include/gloom/gameplay/kinematic_motion.hpp](../../../include/gloom/gameplay/kinematic_motion.hpp) | 72 | STL, exceptions |
| [include/gloom/gameplay/legacy_arsenal.hpp](../../../include/gloom/gameplay/legacy_arsenal.hpp) | 117 | STL |
| [include/gloom/gameplay/legacy_movement.hpp](../../../include/gloom/gameplay/legacy_movement.hpp) | 34 | Sin coincidencia lexical |
| [include/gloom/gameplay/legacy_pickups.hpp](../../../include/gloom/gameplay/legacy_pickups.hpp) | 51 | STL |
| [include/gloom/gameplay/match_lobby.hpp](../../../include/gloom/gameplay/match_lobby.hpp) | 96 | STL, owners, virtual |
| [include/gloom/gameplay/pickup_presentation.hpp](../../../include/gloom/gameplay/pickup_presentation.hpp) | 32 | Sin coincidencia lexical |
| [include/gloom/gameplay/slice_selection.hpp](../../../include/gloom/gameplay/slice_selection.hpp) | 68 | STL |
| [include/gloom/gameplay/vertical_slice.hpp](../../../include/gloom/gameplay/vertical_slice.hpp) | 210 | STL, owners, pimpl |
| [include/gloom/gameplay/vertical_slice_network.hpp](../../../include/gloom/gameplay/vertical_slice_network.hpp) | 105 | STL, owners, pimpl |
| [include/gloom/network/clock_sync.hpp](../../../include/gloom/network/clock_sync.hpp) | 35 | STL |
| [include/gloom/network/combat.hpp](../../../include/gloom/network/combat.hpp) | 88 | STL, maps, owners |
| [include/gloom/network/movement_replication.hpp](../../../include/gloom/network/movement_replication.hpp) | 206 | STL, maps |
| [include/gloom/network/network_simulator.hpp](../../../include/gloom/network/network_simulator.hpp) | 73 | STL, owners, pimpl |
| [include/gloom/network/prediction_history.hpp](../../../include/gloom/network/prediction_history.hpp) | 70 | STL, auto, exceptions |
| [include/gloom/network/presentation_smoother.hpp](../../../include/gloom/network/presentation_smoother.hpp) | 82 | STL, exceptions |
| [include/gloom/network/protocol.hpp](../../../include/gloom/network/protocol.hpp) | 58 | STL |
| [include/gloom/network/session.hpp](../../../include/gloom/network/session.hpp) | 218 | STL, maps |
| [include/gloom/network/snapshot_buffer.hpp](../../../include/gloom/network/snapshot_buffer.hpp) | 98 | STL, auto, exceptions |
| [include/gloom/network/transport.hpp](../../../include/gloom/network/transport.hpp) | 72 | STL, virtual |
| [src/backends/gns_transport.cpp](../../../src/backends/gns_transport.cpp) | 392 | STL, auto, exceptions, maps, owners, pimpl |
| [src/gameplay/first_person.cpp](../../../src/gameplay/first_person.cpp) | 45 | STL |
| [src/gameplay/legacy_arsenal.cpp](../../../src/gameplay/legacy_arsenal.cpp) | 210 | STL, auto |
| [src/gameplay/legacy_movement.cpp](../../../src/gameplay/legacy_movement.cpp) | 46 | STL, exceptions |
| [src/gameplay/legacy_pickups.cpp](../../../src/gameplay/legacy_pickups.cpp) | 117 | STL, auto, exceptions |
| [src/gameplay/match_lobby.cpp](../../../src/gameplay/match_lobby.cpp) | 361 | STL, auto, exceptions, owners, virtual |
| [src/gameplay/pickup_presentation.cpp](../../../src/gameplay/pickup_presentation.cpp) | 52 | Sin coincidencia lexical |
| [src/gameplay/vertical_slice.cpp](../../../src/gameplay/vertical_slice.cpp) | 1344 | STL, auto, exceptions, owners, pimpl |
| [src/gameplay/vertical_slice_network.cpp](../../../src/gameplay/vertical_slice_network.cpp) | 1209 | STL, auto, exceptions, maps, owners, pimpl |
| [src/network/clock_sync.cpp](../../../src/network/clock_sync.cpp) | 49 | STL, exceptions |
| [src/network/combat.cpp](../../../src/network/combat.cpp) | 381 | STL, auto, exceptions |
| [src/network/movement_replication.cpp](../../../src/network/movement_replication.cpp) | 1101 | STL, auto, exceptions, maps |
| [src/network/network_simulator.cpp](../../../src/network/network_simulator.cpp) | 171 | STL, auto, exceptions, owners, pimpl |
| [src/network/protocol.cpp](../../../src/network/protocol.cpp) | 109 | STL, auto, exceptions |
| [src/network/session.cpp](../../../src/network/session.cpp) | 573 | STL, auto, exceptions |
| [tests/combat_tests.cpp](../../../tests/combat_tests.cpp) | 196 | STL, auto, exceptions |
| [tests/legacy_arsenal_tests.cpp](../../../tests/legacy_arsenal_tests.cpp) | 94 | STL, auto, exceptions |
| [tests/legacy_movement_tests.cpp](../../../tests/legacy_movement_tests.cpp) | 189 | STL, auto, exceptions |
| [tests/legacy_pickups_tests.cpp](../../../tests/legacy_pickups_tests.cpp) | 135 | STL, auto, exceptions, maps |
| [tests/match_lobby_tests.cpp](../../../tests/match_lobby_tests.cpp) | 115 | STL, auto, exceptions |
| [tests/network_protocol_tests.cpp](../../../tests/network_protocol_tests.cpp) | 205 | STL, auto, exceptions |
| [tests/network_replication_tests.cpp](../../../tests/network_replication_tests.cpp) | 396 | STL, auto, exceptions |
| [tests/network_transport_tests.cpp](../../../tests/network_transport_tests.cpp) | 210 | STL, auto, exceptions |
| [tests/pickup_presentation_tests.cpp](../../../tests/pickup_presentation_tests.cpp) | 54 | Sin coincidencia lexical |
| [tests/session_tests.cpp](../../../tests/session_tests.cpp) | 262 | STL, auto, exceptions |
| [tests/vertical_slice_network_tests.cpp](../../../tests/vertical_slice_network_tests.cpp) | 495 | STL, auto, exceptions, owners, virtual |
| [tests/vertical_slice_tests.cpp](../../../tests/vertical_slice_tests.cpp) | 395 | STL, auto, exceptions |
| [tests/vertical_slice_transport_tests.cpp](../../../tests/vertical_slice_transport_tests.cpp) | 223 | STL, auto, exceptions |

### 123: [Directorio de partidas y navegador asíncrono](123-directorio.md) — 5 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [include/gloom/gameplay/match_discovery.hpp](../../../include/gloom/gameplay/match_discovery.hpp) | 172 | STL, owners, virtual |
| [include/gloom/gameplay/match_service_wire.hpp](../../../include/gloom/gameplay/match_service_wire.hpp) | 9 | STL |
| [src/gameplay/match_discovery.cpp](../../../src/gameplay/match_discovery.cpp) | 505 | STL, auto, maps, owners, virtual |
| [src/gameplay/match_service_wire.cpp](../../../src/gameplay/match_service_wire.cpp) | 71 | STL, auto |
| [tests/match_discovery_tests.cpp](../../../tests/match_discovery_tests.cpp) | 304 | STL, auto, maps, owners, virtual |

### 124: [Persistencia durable de partidas con datos propios](124-persistencia.md) — 1 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [src/gameplay/durable_match_service.cpp](../../../src/gameplay/durable_match_service.cpp) | 217 | STL, auto, exceptions, maps, owners, virtual |

### 125: [Identidad, tickets y servicios HTTP/HTTPS](125-identidad-y-http.md) — 18 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [apps/gloom_match_service/main.cpp](../../../apps/gloom_match_service/main.cpp) | 114 | STL, auto, exceptions, owners |
| [include/gloom/backends/game_ticket.hpp](../../../include/gloom/backends/game_ticket.hpp) | 21 | STL, owners, pimpl |
| [include/gloom/backends/keycloak_identity.hpp](../../../include/gloom/backends/keycloak_identity.hpp) | 56 | STL, owners, pimpl |
| [include/gloom/backends/match_https_host.hpp](../../../include/gloom/backends/match_https_host.hpp) | 26 | STL, owners, pimpl |
| [include/gloom/backends/match_reader_credential.hpp](../../../include/gloom/backends/match_reader_credential.hpp) | 52 | STL, auto |
| [include/gloom/backends/match_reader_grants.hpp](../../../include/gloom/backends/match_reader_grants.hpp) | 50 | STL, owners, pimpl |
| [include/gloom/backends/match_request_budget.hpp](../../../include/gloom/backends/match_request_budget.hpp) | 31 | STL |
| [include/gloom/backends/winhttp_match_service.hpp](../../../include/gloom/backends/winhttp_match_service.hpp) | 25 | STL, owners |
| [include/gloom/backends/winhttp_request.hpp](../../../include/gloom/backends/winhttp_request.hpp) | 17 | Sin coincidencia lexical |
| [src/backends/game_ticket.cpp](../../../src/backends/game_ticket.cpp) | 164 | STL, auto, exceptions, maps, owners, virtual, pimpl |
| [src/backends/keycloak_identity.cpp](../../../src/backends/keycloak_identity.cpp) | 280 | STL, auto, exceptions, maps, owners, pimpl |
| [src/backends/match_https_host.cpp](../../../src/backends/match_https_host.cpp) | 269 | STL, auto, exceptions, owners, pimpl |
| [src/backends/match_reader_grants.cpp](../../../src/backends/match_reader_grants.cpp) | 272 | STL, auto, exceptions, maps, owners, pimpl |
| [src/backends/winhttp_identity.cpp](../../../src/backends/winhttp_identity.cpp) | 123 | STL |
| [src/backends/winhttp_match_service.cpp](../../../src/backends/winhttp_match_service.cpp) | 216 | STL, auto, exceptions, owners, virtual |
| [tests/game_ticket_tests.cpp](../../../tests/game_ticket_tests.cpp) | 178 | STL, auto, exceptions, owners |
| [tests/keycloak_identity_tests.cpp](../../../tests/keycloak_identity_tests.cpp) | 394 | STL, auto, exceptions, owners |
| [tests/match_https_tests.cpp](../../../tests/match_https_tests.cpp) | 548 | STL, auto, exceptions, owners |

### 126: [UI y presentación de menús con buffers reutilizados](126-ui.md) — 6 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [apps/gloom/desktop.cpp](../../../apps/gloom/desktop.cpp) | 457 | STL, auto, exceptions, owners |
| [apps/gloom/desktop.hpp](../../../apps/gloom/desktop.hpp) | 21 | STL, owners |
| [apps/gloom/game_ui.hpp](../../../apps/gloom/game_ui.hpp) | 267 | STL, auto |
| [include/gloom/render/ui.hpp](../../../include/gloom/render/ui.hpp) | 72 | STL, maps |
| [src/render/ui.cpp](../../../src/render/ui.cpp) | 250 | STL, auto, exceptions |
| [tests/ui_tests.cpp](../../../tests/ui_tests.cpp) | 99 | STL, auto, exceptions |

### 127: [Composición del runtime, arranque y aplicaciones](127-runtime-y-apps.md) — 14 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [apps/gloom/main.cpp](../../../apps/gloom/main.cpp) | 2705 | STL, auto, exceptions, owners |
| [apps/gloom/visual_review.hpp](../../../apps/gloom/visual_review.hpp) | 61 | STL |
| [apps/gloom_scene_viewer/main.cpp](../../../apps/gloom_scene_viewer/main.cpp) | 143 | STL, auto, exceptions |
| [include/gloom/backends/placeholder.hpp](../../../include/gloom/backends/placeholder.hpp) | 25 | STL, virtual |
| [include/gloom/backends/sdl_window.hpp](../../../include/gloom/backends/sdl_window.hpp) | 42 | STL, owners, virtual, pimpl |
| [include/gloom/core/engine.hpp](../../../include/gloom/core/engine.hpp) | 24 | STL, owners |
| [include/gloom/core/subsystem.hpp](../../../include/gloom/core/subsystem.hpp) | 24 | STL, virtual |
| [include/gloom/platform/input.hpp](../../../include/gloom/platform/input.hpp) | 58 | STL |
| [include/gloom/platform/movement_actions.hpp](../../../include/gloom/platform/movement_actions.hpp) | 24 | Sin coincidencia lexical |
| [include/gloom/platform/window.hpp](../../../include/gloom/platform/window.hpp) | 31 | STL, virtual |
| [src/backends/placeholder.cpp](../../../src/backends/placeholder.cpp) | 27 | STL |
| [src/backends/sdl_window.cpp](../../../src/backends/sdl_window.cpp) | 229 | STL, auto, exceptions, owners, pimpl |
| [src/core/engine.cpp](../../../src/core/engine.cpp) | 62 | STL, auto, exceptions, owners |
| [tests/engine_tests.cpp](../../../tests/engine_tests.cpp) | 58 | STL, auto, exceptions, owners |

### 128: [Cumplimiento global verificable y flags de compilación](128-cierre-cpp.md) — 1 archivos

| Archivo | Líneas | Señales |
| --- | ---: | --- |
| [tests/visual_capture_tests.cpp](../../../tests/visual_capture_tests.cpp) | 145 | STL, auto, exceptions |

## Build, validación y herramientas que no son C++

- CMakeLists/CMakePresets: opciones y consumers en cada hito; cierre global 128.
- cmake/PatchDiligent.cmake: compatibilidad de dependencia en 119/128;
  cmake/PackageMatchRuntime.cmake: distribución de servicios en 125/128.
- cmake/RunGraphicsValidation, RunHoundRuntime, RunMaterialReview, RunVisualReview
  y tools/review/ui_capture.cmake: pruebas existentes conservadas en 119/126/130.
- tools/review y tools/legacy: scripts de pruebas/importación de contenido, conservar
  outputs/contratos; no migrar Python/PowerShell fuera de sus bibliotecas estándar.
- tools/art/measure_hound_budget.py: protocolo existente en 112/129/130; los demás
  scripts de arte no se reescriben ni ejecutan para regenerar fuentes en esta tarea.
- AGENTS/RTK: requisitos, no objetivos de optimización. vcpkg.json/CMake fijan
  dependencias actuales; ningún hito permite actualizarlas o sustituirlas en bloque.

## Escenarios focalizados requeridos en 112

| Frente | Casos mínimos | Datos a registrar |
| --- | --- | --- |
| Poses | Mismo rig, dos actores; loop/blend/corte; rigs Factory/Hound | CPU poses/bounds, asignaciones/bytes, joints, poses actuales/anteriores y uploads |
| Jobs | Un worker y varios, grupos repetidos/anidados, productores y saturación | Capturas sizeof/alignof, capacidad/depth, schedule/wait/assist y asignaciones |
| Partículas | 0, 1, 2.048 y exceso, ráfagas/emisores/trails/owner cancel | update/render CPU, capacity, spawned/expired/dropped y salida |
| Entidades | 8, 64 y 1.024, misma composición; crecer con punteros retenidos | get/create/destroy CPU, bytes por componente, estabilidad y allocations |
| Assets/residencia | Frío, cache hit, duplicado, cancel/reload, dos escenas compartiendo | CPU I/O/prep/update, copias, peak, requests y generación/refcount |
| Renderer | Frames estables, uploads, release en vuelo, resize/shutdown | CPU preparación y GPU por pase, counts/skin/upload bytes, fences, resident |
| Física | Fixed tick, consultas rollback, contactos/triggers con varios workers | simulate/query CPU, events y memory, own/external allocations |
| Audio | 0/64/exceso voices, nulo/SDL, loop/stop/reload | mix/pump CPU, voices/drops, buffers/underruns y allocations |
| Red/gameplay | Mismos inputs 2/8/64 clientes de fixtures; loss/reorder/resume | Tick/encode/decode CPU, bytes, queues/history, autoridad y drops |
| Directory/HTTP/storage | Refresh/resolve y fixtures de request/auth/store | Latencia, copies/peak, ownership/cancel, validaciones y errores |
| UI/snapshots | Menú/HUD estables, campos/foco/resize, buffers frame | CPU rebuild, capacity, own allocations y copias de snapshots |

Esos escenarios aún no están medidos en este hito. 112 fija fixtures/semillas,
duración/comandos y atribución; no los llama benchmarks existentes si hay que
añadir un modo focalizado. Usar los tests actuales como base y añadir solo el
diagnóstico necesario. No confundir escenarios sintéticos con partida de producción.
