# Hito 117: carga y cocinado de assets con STL autorizada

Terminado el 7 de octubre de 2026; commit local de cierre identificable con
`rtk git log -1 --oneline --grep='^hito 117: optimizar'`. Sin push; 118 no iniciado.
Base de esta entrega `54a51b7`; avance de catálogo/bind `da72ffe` conservado.
Ponytail full: reutilizar los tipos existentes, sin nueva biblioteca estándar,
Boost/EASTL ni cambio de fastgltf/render/dependencias.

Cocinado Factory/Hound reduce pico WS alrededor del 22 %; encode de escena
aproximadamente 34 % menos tiempo y una asignación. Carga y tiempo total de
cocinado no mejoran de forma concluyente. [Medidas completas y límites](medidas.md).
Hound mantiene 345,26 MiB GPU y 0/1.440 frames >5 ms. Factory conserva un pico
aislado de 7,870 ms en el informe; el control adicional no lo reproduce.

## Implementación y contratos

VFS mantiene strings/vistas y filesystem propietarios; mounts en vector pequeño
en vez de mapa. mount devuelve expected y todos los callers lo comprueban al
arrancar. UTF-8 y argv UTF-16 válidos, nombres nativos Windows, normalización,
raíces y archivos ausentes y escapes por .. o junction se validan antes de leer.
Componentes Windows con punto/espacio final se rechazan para impedir escapes
por normalización Win32. Se usan sobrecargas filesystem con error_code y streams
sin excepciones. El límite previo del cooker/VFS es 1 GiB; no se añade al importador.

AssetCatalog conserva registros individualmente estables e índice ID/puntero
ordenado. Find binario y orden dependency-first iterativo, sin mapa ni recursión;
punteros sobreviven a growth/upsert, campos vistos caducan al upsert. Destruir o
move-assign del dueño invalida esos punteros; mutación exige sincronizar lectores.
Bind usa Array/bitset y ahora verifica también la jerarquía sin skins/animación.
Los resultados/costes del avance frente al 116 están en [histórico](medidas-parciales.md):
memoria −10,08 %, order menor, **find 3,116 → 16,333 ns**, coste explícito.

Cooker resuelve una vez y deja al importador leer, sin leer/copiar primero el glTF
completo. Payloads se transfieren por move. URI normalizada produce AssetId canónico;
aliases compatibles cocinan una textura, aliases de color/data/normal incompatibles
devuelven error. Dependencias ordenadas por ID, mismos nombres/fingerprints/formato.
scene_catalog usa el catálogo para deduplicar; vistas de componentes apuntan a un
string propietario inmóvil durante la resolución, no se retienen después.

ImportedScene conserva propiedad de buffers/textos; sus vistas externas se copian
antes de destruir fastgltf/simdjson. Se comprueban layout/counts de accessors, valores
finitos, índices, joints/pesos, referencias/materiales/UV, animación y graph. URI de
fastgltf se extrae como texto percent-decoded, sin conversión ANSI. Extras usan
callback nativo y resultados get comprobados, sin lambdas ni .value que pueda lanzar.
Nodos nuevos tienen matriz identidad. Decoder comprueba presupuestos mínimos y
restantes **antes de reservar**; reserved/version/endian/fingerprint y dependencias
canónicas se rechazan si son inválidos. El checksum se verifica antes de copiar
payload. Encoder exige datos previamente válidos por assert Debug y reserva tamaño
exacto; no valida de nuevo en cada encode Release. Datos externos conservan expected.

Meshoptimizer/MikkTSpace, tangentes/UV/joints/pesos y LODs conservados; loops simples,
reservas conocidas y enteros propios. Texturas conservan KTX/UASTC, mips/semántica,
BC5/BC7 y fallback RGBA8/RRRG. Se mantiene un mip temporal, se copia cada nivel en
KTX y se libera STBI/mips antes de comprimir; no una copia RGBA base y toda la cadena.
La copia final del bloque asignado por KTX a vector propietario es necesaria.

FactoryScene posee directamente colisión y EnvironmentProbe. load_factory_scene
devuelve resultado propietario; errores de los tres archivos, JSON/probe truncado,
vectores no finitos o índices inválidos se detectan al cargar. original_factory_error
inicializa/comprueba el único dueño inmutable de vida de proceso antes de usarlo.
IDs/hash/97 entidades/9 spawns/73 pickups y contenido original se conservan.
SceneMovement y FactoryCharacterResolver son función/contexto prestados; el contexto
vive más que todas las settings que lo usan. La consulta Jolt se prepara durante
inicialización y se serializa con SRWLOCK; cada llamada reinicia su cápsula. El techo
del lock global y opción por worker si se mide contención están marcados ponytail.
No migra Jolt World/listeners: siguen siendo del 120.

Callers de colisión y luz pasan punteros prestados al dueño Factory de vida de
proceso; physics create_body copia datos, el renderer conserva probe mientras su
dueño vive. No cambia retiro/fences/drain de GPU. La adaptación del coordinador
ofrece AssetUploadSink función/contexto para probar sin subclase; la ruta real de
Renderer sigue disponible. Uploads por valor transfieren propiedad, contexto vive
hasta drain. Se eliminan copias de future.get y el matching por sufijo de URI:
la preparación usa IDs de dependencia canónicos, incluido el test de aliases.
Contextos/futuros/mapas/bind_rig compartido heredados siguen bajo 118/119, no cerrados.

## Alcance STL y flags comprobados

La autorización expresa del usuario del 7 de octubre aplica a **19 archivos
propietarios** de la [ficha 117](../../docs/cpp/tasks/117-assets-y-cooker.md) y sus
adaptaciones necesarias; [AGENTS](../../AGENTS.md) conserva el resto de reglas.
No se extiende a otros hitos. Usos permitidos y revisados:

- string/string_view/u8string/vistas UTF-8 y conversiones de texto/charconv;
  vector/array/byte para almacenamiento propietario y tablas;
- filesystem/error_code/streams e IO de CLI para archivos nativos y errores;
  expected/unexpected y optional/nullopt para resultados;
- move, bit_cast/to_integer/numeric_limits/type_traits y variant exigido por
  fastgltf; thread::hardware_concurrency para KTX;
- unique_ptr con deleter de **objetos opacos STBI/KTX**, que no pueden almacenarse
  como miembros por valor y exigen destructor C correcto;
- shared_future/chrono/this_thread en tests que consumen la API asíncrona existente
  del 118. No shared_ptr propio del 117; no ownership compartido nuevo.

[Inventario exacto de símbolos](datos/manifest.md). Span propio por valor,
enteros propios y designated defaults revisados. Sin mapas/algoritmos STL,
auto local ordinario, lambdas, throw/catch, RTTI/herencia/PIMPL propios en sus
19 archivos. El auto de surface_fields es tipo de retorno de un helper de
layout común, no variable local. Helpers de deleter/conversión de dependencia
son fronteras reales. No asserts de OOM, abort manual ni sombras de Vulkan.

Los **11 .cpp propietarios** tienen excepciones y RTTI desactivados: MSVC
/EHs-c- y RuntimeTypeInfo=false (/GR-), alternativas -fno-exceptions/-fno-rtti
en CMake. gltf_importer se compila en engine y pipeline; Factory y glTF definen
SIMDJSON_EXCEPTIONS=0. [Flags efectivos](datos/flags.md), Release y Debug,
13 entradas de fuentes/26 registros entre ambas configuraciones; sin targets temporales.
/wd4530 se limita al patrón existente de fuentes con headers legacy transitivos;
el override D9025 de /EHsc es visible. No certifica esos headers/futuros/Jolt
ni flags globales: se terminan por sus dueños y en 128. Runtime profile OFF.

## Validación final

Builds completos Release/Debug, formato y diff --check correctos. **7/7 CTest
focalizados en ambos**, dentro de las suites finales completas. Release **50/54**
(108,96 s), Debug **51/54** (256,21 s).
Mismos fallos heredados por causa: ui_visual_review, factory_visual_review y
character_visual_review por referencias antiguas; animation_network GNS 25 solo
Release. visual_review, física/movimiento/combat/red, Vulkan y Hound pasan.
No se cambian referencias ni umbrales. [54 resultados por build](datos/pruebas.md).

Comparación directa de Release con el ejecutable anterior: 67 capturas completas
(seis Factory, siete personajes, 54 UI; el comparador usa 51 de UI). UI idéntica
en todos los canales, personajes máximo 1/255. Factory pasa los seis comparadores:
media de miniatura 0,361–0,612/255, changed 0 %, worst_tile ≤1,578/255. Las capturas
completas tienen diferencias locales de hasta 246/255; no se afirma igualdad pixel
a pixel. Se contrastaron las capturas completas y se inspeccionó la vista general;
se repitió Factory con el ejecutable
**anterior**: también cambia, max 242/255 y medias completas 2,188–3,919/255,
frente a 0,916–1,706/255 antes/después. La variación de escena no se atribuye al
117; ambos controles pasan sin modificar umbrales. [Capturas y hashes](datos/visual.md),
[control de variación anterior](datos/visual-control.md). Las imágenes quedan en caché.
La herramienta de load final se vuelve a ejecutar con sus enteros propios y metadata:
mismas libs/checksums/asignaciones, [pasadas separadas](datos/load-herramienta.md);
no se mezclan con la mediana principal.

Tests existentes ampliados, sin framework/CTest nuevo: todos los prefijos
truncados del envelope y escena pequeña, header/counts/deps/fingerprint corruptos,
escena vacía, owned/move, matrices, joints y graph sin skins/ciclos; catálogo
de 4.096 registros, growth/upsert/move, cinco hijos Debug de asserts; fallos de
loader, cancelación/hot reload/saturación y aliases canónicos hasta upload.
Factory carga desde ruta Unicode propietaria y rechaza probe/JSON truncados o
raíces ausentes. La CLI deja un check repetible de cinco casos: argv/rutas/URI
Unicode nativas, traversal, raíz ausente, junction fuera del mount y UTF-16 inválido.
[Resultados de rutas](datos/rutas.md), herramienta tools/perf/check_asset_paths.py.
Formatos previos consumidos y **104 outputs byte-idénticos** después del cooker;
decoder externo más estricto sobre datos inválidos, versión cooked 1 conservada.

198 archivos C++ /37,350 líneas /54 CTest; tres herramientas Python de
medida/check añadidas fuera del inventario C++. Fuentes/referencias/recursos en
assets, assets-source y art sin diferencias. Hound 131/132, v16/v17 y protocolo22
intactos; Meshy no ejecutado. Renderer Diligent/Vulkan intacto salvo el puntero
prestado del probe necesario al quitar ownership compartido de Factory.

No se promete cero asignaciones de carga ni cumplimiento C++ global. El 118 queda
preparado para revisar propiedad async/residencia; no se inicia. Jolt conserva
su bloqueo de herencia/listeners del 120/128, que la excepción STL no resuelve.
