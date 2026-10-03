# Hito 117: Datos de assets, VFS, importación y cooker

Estado: **no iniciado**. Depende de: **113, 114 y 116**.
Objetivo: **Cumplimiento C++ y coste de carga**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
19 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Bloqueo concreto registrado en 112

**No iniciado; cierre bloqueado** para la frontera externa, pendiente de decisión
expresa del usuario o alternativa equivalente demostrada. fastgltf 0.9 exige std::filesystem::path para loadGltf y variantes; leer bytes con API C no elimina ese argumento.
[Evidencia y probe](../../../reports/cpp-performance-112/compatibilidad.md).
No se aplica excepción por ausencia de respuesta. Conservar eventos/formatos y
dependencias; no marcar cerrado mientras esa condición siga pendiente.

## Evidencia de partida

VirtualPath/VFS/catalog/ImportedScene/CookedAsset usan strings, filesystem, expected, vectores y mapas. AssetRecord contiene VirtualPath sin
constructor por defecto: no se puede sustituir mecánicamente por Array del 110. fastgltf, simdjson, meshoptimizer, KTX y MikkTSpace son dependencias
reales del pipeline.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [include/gloom/assets/asset.hpp](../../../include/gloom/assets/asset.hpp)
- [src/assets/asset.cpp](../../../src/assets/asset.cpp)
- [include/gloom/assets/gltf_importer.hpp](../../../include/gloom/assets/gltf_importer.hpp)
- [src/assets/gltf_importer.cpp](../../../src/assets/gltf_importer.cpp)
- [src/assets/asset_cooker.cpp](../../../src/assets/asset_cooker.cpp)
- [src/assets/mesh_processing.cpp](../../../src/assets/mesh_processing.cpp)
- [src/assets/texture_asset.cpp](../../../src/assets/texture_asset.cpp)
- [apps/gloom_asset_cooker/main.cpp](../../../apps/gloom_asset_cooker/main.cpp)

## Archivos responsables de cierre

- [apps/gloom_asset_cooker/main.cpp](../../../apps/gloom_asset_cooker/main.cpp)
- [include/gloom/assets/asset.hpp](../../../include/gloom/assets/asset.hpp)
- [include/gloom/assets/asset_cooker.hpp](../../../include/gloom/assets/asset_cooker.hpp)
- [include/gloom/assets/gltf_importer.hpp](../../../include/gloom/assets/gltf_importer.hpp)
- [include/gloom/assets/mesh_processing.hpp](../../../include/gloom/assets/mesh_processing.hpp)
- [include/gloom/assets/rig.hpp](../../../include/gloom/assets/rig.hpp)
- [include/gloom/assets/scene_catalog.hpp](../../../include/gloom/assets/scene_catalog.hpp)
- [include/gloom/assets/texture_asset.hpp](../../../include/gloom/assets/texture_asset.hpp)
- [include/gloom/gameplay/factory_scene.hpp](../../../include/gloom/gameplay/factory_scene.hpp)
- [src/assets/asset.cpp](../../../src/assets/asset.cpp)
- [src/assets/asset_cooker.cpp](../../../src/assets/asset_cooker.cpp)
- [src/assets/gltf_importer.cpp](../../../src/assets/gltf_importer.cpp)
- [src/assets/mesh_processing.cpp](../../../src/assets/mesh_processing.cpp)
- [src/assets/scene_catalog.cpp](../../../src/assets/scene_catalog.cpp)
- [src/assets/texture_asset.cpp](../../../src/assets/texture_asset.cpp)
- [src/gameplay/factory_scene.cpp](../../../src/gameplay/factory_scene.cpp)
- [tests/asset_pipeline_tests.cpp](../../../tests/asset_pipeline_tests.cpp)
- [tests/character_restoration_tests.cpp](../../../tests/character_restoration_tests.cpp)
- [tests/factory_restoration_tests.cpp](../../../tests/factory_restoration_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Añadir solo texto propietario/vista, bytes y resultados explícitos que requieran estos consumidores; C/native para archivos y rutas. Conservar
   Unicode de Windows, normalización, mounts y bloqueo de escapes del VFS. Definir si cada texto es terminado en cero y cuál es su longitud/caducidad;
   no asumir que un Span textual sirve como ruta nativa.
2. Migrar catálogo a arrays ordenados/índices y búsqueda explícita; preparar índice al modificar catálogo, no ordenar cada find. Conservar
   AssetId/fingerprint, dependencies y detección de ciclos. Mantener compatibilidad binaria del cooked_asset_version y de los formatos
   rig/mesh/texture.
3. Migrar almacenamiento propio de importación/rig/mesh/material/texture/scene catalog y operaciones de cooker, sin copiar payloads grandes ni
   construir capacidad de objetos costosos innecesariamente. Resolver el caso AssetRecord al primer consumidor; no replicar todo
   vector/expected/filesystem.
4. Adaptar APIs externas en una frontera pequeña. Los tipos y STL internos de una dependencia no se reescriben; el uso directo de std en código Gloom
   sigue prohibido. Si una API exige std explícito y no hay sobrecarga equivalente, registrar el conflicto antes de cerrar, no ocultarlo detrás de una
   typedef.
5. Mantener compresión BC5/BC7 y fallback existente, tangentes/UV, joints/pesos, LODs y validación del contenido corrupto antes de entrar en runtime.
   No regenerar arte ni modificar fuentes/exportaciones v16/v17.

## Contratos que conservar

Los buffers importados/cocinados siguen siendo propietarios; Span no reemplaza al dueño. Las vistas de una biblioteca externa no sobreviven al parser.
No cambiar el formato de assets ni añadir Boost/EASTL/NoGraphicsAPI como dependencia.

## Validación focalizada

CTest existentes: `gloom.assets`, `gloom.skin_bounds`, `gloom.animation_vfx`, `gloom.factory_restoration`, `gloom.character_restoration`,
`gloom.legacy_arsenal`, `gloom.hound_runtime`.

Roundtrip de formatos con blobs anteriores, corrupciones/truncado/dependencies cíclicas, rutas Unicode y traversal, importación de
rigs/skins/materiales existentes y cooker glTF/texture. Comprobar fuentes y recursos originales contra Git; probar textos vacíos/movidos y ownership
de buffers.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

Tiempo y pico de memoria de cook/load en assets pequeños y Factory/Hound; asignaciones de carga permitidas y declaradas. Objetivo runtime: no
introducir nuevas copias o búsquedas lineales no medidas en cada frame.

Código propio del pipeline, su herramienta y pruebas migrados; ABI de datos conservada, sin usos directos de STL, mapas, excepciones/RTTI o
propietarios compartidos. Cualquier conflicto de dependencia está resuelto explícitamente.

Entregar informe `reports/cpp-performance-117/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 117: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/117-assets-y-cooker.md. Ejecuta solo
> el hito 117 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
