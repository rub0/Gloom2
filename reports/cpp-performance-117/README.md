# Hito 117: avance del catálogo y evaluación de bind

**Iniciado, parcial; no cerrado.** 5 de octubre de 2026. Base `f38a4a8`.
Ponytail full. Se conservan dependencias, renderer, protocolo 22, formatos y arte.
No push ni inicio del 118. Setup Hound 131 conservado, sin llamadas Meshy.

## Avance implementado y comprobado

AssetCatalog sustituye su unordered_map por un Array ordenado de ID/puntero.
Actualiza el índice al insertar, nunca al consultar. Búsqueda binaria, registros
propietarios en asignaciones individuales; find conserva direcciones al crecer
y upsert reemplaza el contenido sin mover el registro. Copy deshabilitado;
move/destrucción transfieren o liberan cada registro una sola vez. Esto resuelve
el almacenamiento del catálogo sin exigir defaults inválidos a VirtualPath.
AssetRecord y sus paths/dependencies mantienen aún su representación legacy.
El puntero al registro caduca al destruir su catálogo o sustituirlo mediante
move assignment; las vistas a campos internos caducan al hacer upsert.
Como antes, mutar el catálogo exige sincronización externa con sus lectores.

El recorrido de dependencias usa índices, marcas y una pila explícita; evita
recursión, lambdas y un segundo mapa. Produce orden dependency-first determinista.
Ciclos o IDs ausentes devuelven false, mensaje estático y salida vacía. Los
registros inválidos/duplicados son precondiciones por assert en Debug, sin
abort manual ni checks de OOM. AssetId/AssetType mantienen ancho y valores.

Bind matrices, parents y cola pasan a Array propio, reservado antes de evaluar.
La salida pertenece al caller y queda vacía al fallar. Se conservan las reglas
de ciclos/múltiples padres, matrices finitas, pesos/joints y pose de bind.
Un bitset detecta joints duplicados sin copiar/ordenar sus IDs; se limpia solo
para los joints visitados antes de validar la siguiente skin. No cambia skinning.
Las vistas de matrices caducan al reevaluar o destruir su salida propietaria.

Pruebas existentes ampliadas: cadena de 4.096 registros, estabilidad de punteros,
upsert, move sobre destino no vacío, ciclos, dependencias ausentes, cuatro
precondiciones de catálogo en procesos hijos Debug, joints repetidos y
reutilización de matrices después de un error. Todos los callers de las dos
firmas modificadas están adaptados; no se añade un framework ni un CTest.

## Resultado medido y límites

[Tablas completas, reproducción y ámbito](medidas.md). Cinco pasadas normales
separadas del contador. Con 1.024 registros: total propio medido 246.007 →
221.200 bytes (-10,08 %); order 116.200 → 39.100 ns. Lifecycle queda dentro
del ruido. Las consultas no asignan, antes ni después, pero **find aislado es
más lento**: 3,116 → 16,333 ns. Sus callers actuales son carga/preparación/recarga;
se deja registrado el coste y no se promete una mejora de lookup.

Bind de 256 joints: 41.000 → 37.900 ns; cuatro asignaciones por validación frente
a 18, con menor cantidad de bytes solicitados. Los tamaños 8/53 tienen mayor
dispersión; no se les atribuye una ganancia de tiempo concluyente.

Builds completos Release/Debug y formato correctos. **7/7 CTest focalizados en
ambos**, Release 7,03 s /Debug 53,46 s. Factory/Hound normal sin regresión
integrada atribuible frente a la serie archivada del 116; Hound p99<4 ms,
máximo<5 ms, cero >5 ms, 345,26 MiB y controles de contenido conservados.
No se atribuyen FPS globales ni se recertifica la partida humana de ocho.
Los resultados de la última suite completa siguen siendo los del 116.
198 archivos C++ propios /54 CTest; diagnóstico del runtime OFF.

## Condición pendiente y continuación

La API instalada fastgltf **0.9.0**, core.hpp:891/898/905, exige
`std::filesystem::path` en loadGltf/loadGltfJson/loadGltfBinary. FromBytes exige
`const std::byte*`: leer por C no elimina la conversión obligatoria. El importer
actual usa FromPath y loadGltf en gltf_importer.cpp:239/312, sin cambios en este avance.
[Evidencia previa](../cpp-performance-112/compatibilidad.md).

AGENTS.md prohíbe headers y facilities STL propios salvo initializer_list para
Span. Se ha pedido decidir si se permiten únicamente los tipos/operaciones que
exige fastgltf en el adaptador. **No hay autorización registrada**; el trabajo no
introduce un puente STL nuevo ni esconde el código en vendor. No concede la
excepción de listeners Jolt del 120. Los campos legacy de AssetRecord, VFS,
ImportedScene y APIs de formatos siguen pendientes; no se certifica cumplimiento
literal del pipeline ni flags finales sin excepciones/RTTI.

Continuar el mismo 117, en este orden:

1. Resolver la frontera fastgltf expresamente. Mantener formatos/dependencias;
   si se concede una excepción, enumerar sus tipos y limitarla al adaptador.
2. Añadir texto/vista, bytes y resultados con su consumidor; migrar VFS/rutas
   nativas preservando Unicode y bloqueo de escapes. Convertir metadatos del
   catálogo; conservar el índice y su contrato de punteros ya comprobado.
3. Migrar importación/rig/mesh/material/texture, codecs, cooker, Factory y sus
   consumidores/pruebas según la ficha. Conservar BC5/BC7, fallback y recursos.
4. Ejecutar pruebas de formatos históricos/corrupción, Unicode/traversal y
   ownership; aplicar/verificar los flags finales y medir cook/load completo.
5. Solo entonces cerrar ficha/estado y crear el commit de hito terminado.

Este commit es un **checkpoint del avance**, no el cierre del hito:
`rtk git log -1 --oneline --grep='^hito 117:'`. Informe, ficha, índice e inventario
actualizados. El 116 continúa siendo el último hito C++ cerrado.
