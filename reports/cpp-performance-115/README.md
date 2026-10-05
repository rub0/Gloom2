# Hito 115: partículas y efectos con salida persistente

5 de octubre de 2026. Entrada `892463aaac62fec4de40fd48669b942af657c01d` (114).
Ponytail full; alcance 115. Cierre local, sin push ni inicio del 116.

## Resultado y contratos

ParticleSystem escribe en el Array de la escena del llamador, conservando su
prefijo: desaparecen el vector devuelto por render y su concatenación posterior.
Main posee ese almacenamiento, reserva el margen existente de escena más las
2.048 partículas y lo conserva hasta acabar visibilidad/end_frame. Materiales
por Span por valor, prestados solo durante render y sin solapar la salida.
No se añade una cola ni otro buffer de partículas en el renderer.

Recetas, partículas y emisores usan tipos propios y Array. Nombres de receta
con un máximo existente de 64 bytes, en char[65]. El emisor conserva el índice
resuelto al crearse; refresh busca su ID y no vuelve a recorrer recetas.
Los bursts infrecuentes conservan la búsqueda acotada por nombre. CombatEffects
conserva sus dos estados persistentes; sustituye array STL y lambdas por datos
y funciones directas. No había una cola dinámica que necesitase reemplazo.

Se mantienen 2.048 partículas por defecto (capacidad configurable 1–16.384),
64 emisores, límites de recetas, hash/semillas, serial, orden y contadores
spawned/expired/dropped. Nacimientos cronológicos, desempate por ID con redondeo
a nanosegundos y expiración antes de cada nacimiento; compacción estable.
Cancel/translate usan el owner de toda la vida de entidad; refresh ausente
elimina el emisor en el siguiente advance. Clear no reinicia las métricas.

Los errores de archivo/JSON/esquema, versión, límites y nombres duplicados se
comprueban al cargar y devuelven un mensaje; el resultado anterior queda intacto.
Lectura C y simdjson ya instalado, mediante su API sin excepciones; sin parser
nuevo ni dependencia. Se rechazan nombres con NUL embebido al pasar a texto C.
Timestep, identidad de emisor y spans solapados son precondiciones con assert,
sin excepciones o abort manual para errores de programación.

Array añade append por Span y push_back por referencia, con entradas no solapadas.
Vaciar valores con destructor trivial no reescribe toda la reserva; propietarios
no triviales se limpian inmediatamente y regrowth siempre restaura defaults.
La primera implementación mostró una regresión en estrés por esa reescritura:
se corrigió en Array, conservando las pruebas de recursos del 110. También se
evitan movimientos sobre el mismo elemento y redondeos repetidos del emisor.
La longitud usa sqrt con intermediarios double: cubre el rango finito de float
y conserva el resultado numérico dentro de la tolerancia medida.

Renderer Diligent/Vulkan, materiales, billboards/ribbons, sombras, blending,
soft distance, distorsión, recursos, shaders, referencias y umbrales conservados.
No se introducen partículas GPU ni se cambia la política de vida GPU.

## Medidas y validación

[Datos completos y reproducción](medidas.md). Series de tiempos sin diagnóstico;
asignaciones medidas aparte. Control histórico: 1.000 renders asignan 1.000 veces;
actual: **cero**, con capacidades 1, 5 y 2.048. La prueba permanente combina
1.000 update/render a capacidad completa y 1.000 eventos de combate, sin new/bytes;
el crecimiento fuera de reserva sigue asignando y su control positivo lo detecta.
No son cero heap global ni bytes vivos; cold load y reserva quedan fuera.

Comparación numérica con 114: **49.080 registros, 583.108 valores**, máximo absoluto
**0,00000048**; identidades, orden, métricas y eventos coinciden. Incluye 30/60/144 Hz,
capacidades 1/5/2.048, ráfagas/emisores, translate/cancel/clear y combate.
Estrés sin diagnóstico: mediana de cinco pasadas **48,8 → 37,2 µs** por
update/render de 2.048 partículas. La medida de efectos tiene dispersión y se
registra sin extrapolarla a FPS. El estrés no sustituye la serie integrada.

Juego: partículas **1 → 0 new por frame** en las siete pasadas Factory/Hound.
Poses, bounds y jobs siguen a cero. Serie normal independiente, sin mejora global
de FPS atribuida; ambas versiones observan cero Hound >5 ms y p99 <4 ms.
Residencia 345,26 MiB y controles de contenido conservados.

**Builds completos y formato correctos. Release 50/54, Debug 51/54**: únicamente
los tres fallos visuales heredados y GNS 25 solo en Release, por las mismas causas.
Storage, prueba nativa, animation_vfx, combat, material_render, Vulkan y Hound pasan.
La prueba nativa incluye datos externos inválidos y tres precondiciones por assert
en procesos hijos Debug; gloom.storage verifica recursos al reducir/regrow Array.

**1.200 capturas alineadas pasan**, 41 eventos y métricas iguales, cero dropped.
La comparación normal inicial tiene diferencias de fase de TAA/carga; se conserva
y se alinea solo en ejecutables de revisión, como en 113/114. Referencias y
umbrales intactos. Máximo por canal completo 1/255 en 640×360; pares de efectos
inspeccionados. No se declaran corregidos los comparadores visuales heredados.

198 archivos propios /54 CTest; prueba nueva bajo 115. Diagnóstico de runtime
restaurado OFF y juego recompilado. Siguiente 116 no iniciado. Commit local de
cierre: `rtk git log -1 --oneline --grep='^hito 115:'`; sin push.

No hay uso directo de STL, shared ownership, auto ordinario, lambdas,
excepciones o RTTI en los cuatro archivos del módulo y su prueba autónoma.
Scene/RenderInstance y headers de animación aún tienen datos legacy de sus dueños
117/119; las pruebas mixtas de importer/red conservan sus bloques 117/122.
No se declara cumplimiento global. /wd4530 se limita a consumidores migrados
de esos headers/simdjson; no activa desenredo ni certifica los módulos pendientes.
Los bloqueos fastgltf/Jolt de 117/120/128 siguen pendientes de decisión expresa.
