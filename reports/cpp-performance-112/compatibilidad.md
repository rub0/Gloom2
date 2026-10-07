# Fronteras externas verificadas

No se han cambiado dependencias, renderer ni reglas de AGENTS. Versiones del
build instalado: Jolt **5.6.0 (port 1)**, fastgltf **0.9.0**, GameNetworkingSockets
**1.6.0**, DiligentCore commit **a255d24d365e13edeade3b02676f444a493bc883**
(baseline 2.5.6 con arreglo Vulkan upstream). Vcpkg baseline de `vcpkg.json`.

## Prueba ejecutable del 112

`gloom.dependency_contract` compila su fuente Gloom con `/GR- /EHs-c-` en
Release/Debug. Inicializa y cierra GNS y obtiene su interfaz flat; construye un
Parser fastgltf; incluye RenderDevice con `DILIGENT_C_INTERFACE=1` y los ajustes
de plataforma de Diligent; ejecuta Jolt con dos workers y cuatro pasos.
No usa STL directamente ni clases derivadas propias. Los headers de las
bibliotecas conservan sus tipos internos.

La prueba no demuestra que todas las dependencias binarias estén compiladas sin
RTTI/excepciones, ni que toda operación de parsing/render/red esté migrada.
Los flags legacy `/EHsc` y RTTI siguen en módulos propios pendientes hasta 128.
MSVC avisa que los flags por fuente sobrescriben `/EHs /EHc`; la opción efectiva
del objeto migrado es `/EHs-c-`. No desactivar excepciones a ciegas en código que
todavía contiene throw/catch.

## Jolt: filtros resueltos, listeners pendientes

[PhysicsSystem 5.6](https://github.com/jrouwe/JoltPhysics/blob/v5.6.0/Jolt/Physics/PhysicsSystem.h)
expone `SetContactListener(ContactListener*)` y `WereBodiesInContact`.
`CharacterVirtual::SetListener(CharacterContactListener*)` exige su listener
virtual. Los headers instalados confirman esos contratos; no hay overload C de
callbacks para conservar directamente los eventos de Gloom.

Los filtros propios BroadPhaseLayers, ObjectLayerPairs y ObjectVsBroadPhase pueden
sustituirse por **BroadPhaseLayerInterfaceTable, ObjectLayerPairFilterTable y
ObjectVsBroadPhaseLayerFilterTable** incorporados. La prueba comprueba las cuatro
combinaciones: static/static no colisiona; static/dynamic, dynamic/static y
dynamic/dynamic sí. Conserva IDs de capa 0 y 1; no es necesario cambiar máscaras.

La prueba añade sensor estático y esfera dinámica despierta, sin gravedad. Tres
pasos solapados y un desplazamiento fuera producen **1 entered, 2 stayed, 1 exited**
al consultar después de Update; el pool tiene dos workers. Esto prueba solo ese
caso, no equivalencia con ContactCollector/CharacterContactCollector de Gloom.

Los listeners actuales agregan subshape pairs por body pair, emiten entrada
0→1/salida de último subshape y un stayed por paso; CharacterContactCollector
retiene posición/normal y trata contactos de CharacterVirtual. Un booleano
post-step no conserva necesariamente contactos transitorios, orden de callbacks,
sleeping, retirada de cuerpos ni contactos intermedios del personaje. Borrar los
listeners o inventar eventos por polling cambiaría comportamiento sin demostrarlo.

**Bloqueo de 120 y cierre literal 128:** falta decisión expresa para permitir dos
adaptadores virtuales mínimos de listeners externos, o una alternativa demostrada
con todos esos casos y sin cambiar Jolt. No se ha concedido ninguna excepción.
Se solicitó al usuario esa decisión durante 112; sin respuesta, sigue pendiente.
Los filtros incorporados podrán usarse dentro del 120 aunque se autorice la frontera.

## fastgltf: parsing obliga a tipos STL en la frontera

**Resolución posterior, 7 de octubre de 2026:** el usuario autorizó STL para
el hito 117 («vale entonces usa la stl para esta tarea»). La excepción está
registrada en [AGENTS](../../AGENTS.md) y en la
[ficha 117](../../docs/cpp/tasks/117-assets-y-cooker.md); incluye fastgltf y los
tipos estándar útiles del pipeline. El bloqueo STL de esta frontera queda resuelto
por autorización, sin modificar dependencias. Jolt continúa pendiente. El texto
siguiente conserva el diagnóstico y la condición que existían durante el 112.

[core.hpp 0.9](https://github.com/spnda/fastgltf/blob/v0.9.0/include/fastgltf/core.hpp)
y el instalado exigen `std::filesystem::path` como directorio en loadGltf y sus
variantes. FromPath recibe ese path; FromBytes recibe `const std::byte*`; FromSpan
usa `std::span<std::byte>`. Leer bytes con API C por sí solo no elimina el argumento
filesystem del parser. Sus resultados también contienen tipos de la dependencia.

Construir Parser sin excepciones/RTTI propios pasa el probe; **no** demuestra
importación de glTF sin uso directo de STL. No se hace un parser propio ni se
oculta código Gloom en vendor para eludir la regla.

**Bloqueo del cierre STL de 117 y global 128:** falta autorización expresa para la
conversión mínima en el adaptador fastgltf, o una API alternativa compatible que
elimine esos tipos sin cambiar formato ni dependencias. El trabajo independiente
de assets/VFS puede avanzar, pero 117 no se certifica literal mientras falte esa
decisión. La pregunta enviada agrupa esta frontera y la de Jolt.

## Diligent y GNS

[RenderDevice de la revisión instalada](https://github.com/DiligentGraphics/DiligentCore/blob/a255d24d365e13edeade3b02676f444a493bc883/Graphics/GraphicsEngine/interface/RenderDevice.h)
incluye una interfaz C habilitada por `DILIGENT_C_INTERFACE=1`.
La cabecera pasa el probe con settings de plataforma del target upstream. El 119
puede usar esa frontera y tipos Gloom para datos propios conservando Diligent/Vulkan;
la prueba aún no migra todos los RefCntAutoPtr/owners del renderer.

[GNS 1.6 ofrece su cabecera flat](https://github.com/ValveSoftware/GameNetworkingSockets/blob/v1.6.0/include/steam/steamnetworkingsockets_flat.h); la inicialización, factory flat
v009 y shutdown pasan. Sus interfaces virtuales internas pertenecen a la
dependencia; Gloom no necesita heredar de ellas. No inferir equivalencia de todas
las operaciones de transporte de este smoke test: conserva las pruebas de red,
protocolo 22 y manejo del error GNS 25 heredado al migrar 122.

[NoGraphicsAPI](https://github.com/sebbbi/NoGraphicsAPI) sigue siendo referencia
de API y C++ sencillo. No se añade al build ni se trasladan sus decisiones de
renderer, ownership o validación sin comprobar los contratos de Gloom.
