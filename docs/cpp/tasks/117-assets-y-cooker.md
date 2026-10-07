# Hito 117: Datos de assets, VFS, importación y cooker

Estado: **terminado; STL autorizada en su alcance**, 7 de octubre de 2026.
Depende de: **113, 114 y 116 terminados**.
[Informe final, medidas y límites](../../../reports/cpp-performance-117/README.md).
Objetivo: **Cumplimiento C++ y coste de carga**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
19 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Decisión del usuario: STL autorizada para el 117

El 7 de octubre de 2026 el usuario indicó: **«vale entonces usa la stl para esta tarea»**.
Se autoriza STL en el alcance del 117: sus 19 archivos propietarios y las
adaptaciones necesarias de callers. Conservar fastgltf 0.9 y los tipos estándar
existentes para texto/vistas, buffers, rutas y resultados; no crear sustitutos
propios para cumplir una prohibición que aquí queda exceptuada. La autorización
incluye el adaptador fastgltf y resuelve su bloqueo de compatibilidad:
std::filesystem::path, std::byte y los tipos que exige su API pueden usarse.
[Evidencia y probe](../../../reports/cpp-performance-112/compatibilidad.md).

Esta excepción está registrada en AGENTS.md. No se extiende a otros módulos o
hitos ni autoriza mapas, shared ownership, std::span/std::function en lugar de
Span/FixedFunction, excepciones, RTTI o herencia. Los enteros propios, las
precondiciones, la seguridad de datos externos y las medidas siguen exigidos.
Los listeners Jolt del 120 continúan pendientes de decisión independiente.
La implementación y validación de este alcance están terminadas; no certifican otros módulos.

## Cierre comprobado

VFS/Unicode/errores explícitos, modelos/codecs, cooker, Factory y callers adaptados;
STL existente conservada según autorización, Span/enteros propios y flags finales.
Los 104 blobs cocinados son byte-idénticos. Pico WS Factory/Hound alrededor de
22 % menor; encode aproximadamente 34 % menor y una asignación. Carga/cook wall
sin ganancia concluyente. Catálogo/bind del checkpoint conservados, incluido
el coste de find más lento documentado; no se vuelve a atribuir su mejora.
Builds/formato correctos, 7/7 focalizados en ambos; Release 50/54 y Debug 51/54,
fallos heredados por las mismas causas. Hound 0/1.440 >5 ms, 345,26 MiB GPU;
Factory pico aislado de 7,870 ms, no repetido en control. Fuentes/referencias intactas.
[Contratos, STL enumerada, flags, checks y reproducción](../../../reports/cpp-performance-117/README.md).
Commit local de cierre: `rtk git log -1 --oneline --grep='^hito 117: optimizar'`.
118 no iniciado; Jolt/otros propietarios pendientes de sus respectivos hitos.

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

## Trabajo ejecutado, en orden

1. Reutilizar string/string_view, vector de bytes, expected y filesystem existentes según la excepción del usuario; no añadir una biblioteca propia
   de texto/resultados/archivos. Conservar Unicode de Windows, normalización, mounts y bloqueo de escapes del VFS. Documentar propiedad/caducidad;
   emplear las sobrecargas filesystem con error_code para fallos externos, sin depender de excepciones.
2. Migrar catálogo a arrays ordenados/índices y búsqueda explícita; preparar índice al modificar catálogo, no ordenar cada find. Conservar
   AssetId/fingerprint, dependencies y detección de ciclos. Mantener compatibilidad binaria del cooked_asset_version y de los formatos
   rig/mesh/texture.
3. Optimizar almacenamiento de importación/rig/mesh/material/texture/scene catalog y operaciones de cooker conservando contenedores estándar útiles,
   sin copiar payloads grandes ni construir capacidad de objetos costosos innecesariamente. Reutilizar Array donde ya funciona; justificar sustituciones
   por costes medidos y cumplir las restantes reglas de AGENTS, sin reescribir vector/expected/filesystem.
4. Mantener las APIs externas en una frontera pequeña. Los tipos y STL internos de una dependencia no se reescriben; el uso directo de std necesario
   en este hito está autorizado. Registrar los usos permitidos y verificar que la excepción no se extienda a otros módulos ni otras reglas.
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

Código propio del pipeline, su herramienta y pruebas revisados según AGENTS y la excepción STL del 7 de octubre; ABI de datos conservada,
sin mapas, excepciones/RTTI o propietarios compartidos. Enumerar el uso STL permitido, comprobar las demás reglas y completar las medidas/pruebas.
Cualquier conflicto de dependencia restante está resuelto explícitamente; autorizar STL no certifica por sí solo los otros requisitos.

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
