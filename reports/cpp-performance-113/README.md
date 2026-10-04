# Hito 113: poses persistentes por actor

4 de octubre de 2026. Entrada `268db47042853bc4405a68766366dad81831045d` (112).
Ponytail full; renderer Diligent/Vulkan conservado, sin dependencias nuevas ni cambios de shaders, contenido,
protocolo, referencias o umbrales. Commit de cierre local, sin push.

## Resultado

La preparación del rig valida jerarquía, clips, matrices y pesos una vez en la
frontera del importer. Conserva padres, orden, TRS de reposo, claves, nombres,
inverse bind y cajas de joints en almacenamiento propio. El sampling, blend,
worlds, IK y skinning escriben en buffers del animador. No construyen vectores,
colas de jerarquía ni propietarios compartidos por frame.

Cada actor posee scratch y dos juegos de `SkinPose` por nodo/enlace. Las primitivas
hermanas observan la misma paleta mediante punteros. La identidad temporal incluye
actor, vida única del rig y nodo; el animador comprueba también generación y
dirección. Cambio de actor, rig, generación, enlace, muerte, respawn y discontinuidad
cortan el historial correspondiente. Un nodo que deja de usar skin vacía ambas
paletas al enlazar el nuevo rig.

**Asignaciones medidas en poses: 210 → 0 llamadas/frame Factory y 710 → 0 Hound;
93.728 → 0 y 419.528 → 0 bytes solicitados/frame.** Las siete pasadas instrumentadas
tienen 360 frames medidos después de 120 de calentamiento. Bounds también marca
cero. Se mantienen actividad, geometría, máscaras TPS/FPS, audio y uploads.
Las cifras cuentan `new/new[]` ordinarios del ejecutable, no todo el heap ni RAM viva.

Factory: mediana poses + bounds **0.1380 → 0.0810 ms (41.3 % menos)**; mediana del frame medio 2.3590 → 2.3410 ms.

Hound: mediana poses + bounds **0.6360 → 0.5805 ms (8.7 % menos)**; mediana del frame medio 3.3440 → 3.2765 ms.

Medidas normales y distribuciones completas en [medidas](medidas.md). Comparar
**poses + bounds**: el skinning pasó de la etapa bounds al final de la evaluación
del animador. El tiempo de frame está dominado por otros costes; no se atribuye
una mejora general de FPS ni se certifica el presupuesto H06 global.

## Propiedad y caducidad

| Datos | Propietario y límite |
| --- | --- |
| Rig preparado | `ResidentScene`; inmutable durante las evaluaciones. Reemplazar/destruir después del último consumidor CPU. Preparación rechazada deja intacto el rig anterior. |
| Local, mezcla, IK, worlds, reposo/muerte anterior | `CharacterAnimator` del actor. Reservar al enlazar; ningún scratch compartido entre actores concurrentes. |
| `CharacterAnimationFrame` y sus spans | Observan buffers del animador. Caducan en su siguiente update, bind, reset o destrucción; copiar explícitamente si se necesita conservar worlds/local. |
| Skin actual/anterior | Dos slots del animador. La paleta de N permanece inmutable durante N+1 y sus consumidores CPU; N+2 puede reutilizarla. Bind/destrucción exige consumidores terminados. |
| `RenderInstance` | Observa las paletas; actor + identidad de rig + nodo + mesh/view-model comprueban el enlace anterior. Los vectores de presentación conservan sus propietarios existentes. |
| GPU | `bind_skin` copia matrices actuales, anteriores y normales de inmediato a buffers Diligent con DISCARD. La CPU no prolonga la vida de un puntero para proteger la GPU; fences, retiro y drain existentes se conservan. |

La adaptación de `bind_skin` sustituye el acceso `.get()` por el puntero prestado;
no cambia la técnica de render, el cálculo de matrices ni el almacenamiento GPU.

No se retienen spans de initializer lists. Todos los parámetros Span permanecen
por valor; estructuras grandes por referencia. `Matrix4` tiene exactamente 16
floats/64 bytes, conservando payload cocinado y layout GPU. `CombatantView` se
extrae con campos y arrays propios para desacoplar animación de simulación/red.
`Array` incorpora transferencia por movimiento para los buffers anidados.

Los buffers cambian solicitudes transitorias por capacidad persistente. Para N
nodos y J joints sumados entre enlaces, el payload inicial por animador es
400N + 256J bytes, más el objeto y overhead del allocator. Crecimiento puede
retener capacidad mayor; no se anuncia ahorro de memoria residente del proceso.

## Comprobaciones y alcance

[Validación](validacion.md): builds completos Release/Debug; focal final **9/9 en ambas**; suites **49/53 y
50/53**, con los mismos cuatro/tres fallos del 112. Pruebas finales de almacenamiento,
bounds y enlaces; dos actores concurrentes, crecimiento con control positivo,
reemplazo en la misma dirección y cambio de skin. Cuatro rigs reales, incluidos
Hound v16/v17, evalúan 2 × 1.000 veces sin nuevas asignaciones después de preparar.

La comparación numérica abarca **3.495.720 floats** del 112 y del código nuevo:
clips directos idénticos; cortes idénticos; diferencia máxima 0,000164 en normales
de Shadow y aproximadamente 0,0000041 en anclajes. No es equivalencia bit a bit
del TRS procedural: se usa matemática C en lugar de overloads STL.

**1.200 frames animados y siete vistas estáticas pasan el comparador común** con
calentamiento y fase temporal iguales. Las primeras comparaciones sin alinear
fallaron; se conservan sus resultados y el diagnóstico en validacion.md. La
alineación pertenece solo a ejecutables de validación aislados; no cambia el juego,
renderer ni referencias versionadas.

La ruta de poses usa tipos propios y carece de STL/shared_ptr, excepciones, RTTI,
auto ordinario, lambdas y mapas en sus operaciones migradas. Sus fuentes compilan
sin excepciones/RTTI. Esto **no certifica todos los headers transitivos o tests
mixtos**: Scene/math/GPU y sus headers legacy siguen en 119; importer y
`valid_bind_rigs` en 117; residencia en 118; partículas en 115 y red/simulación en
122. Los bloques heredados de animation_vfx/network se conservan para sus dueños
y revisión transversal 128, sin suprimir pruebas ni conceder excepciones nuevas.
El test skin_bounds desactiva C4530 de chrono transitivo del importer bajo MSVC;
no habilita excepciones en su fuente. Nuevas fuentes tienen responsable 113.

Los bloqueos expresos de fastgltf/Jolt en 117/120/128 permanecen pendientes de
decisión humana. No afectan a este resultado. El 114 y Hound H08 siguen sin iniciar.

Verificar el cierre: `rtk git log -1 --oneline --grep='^hito 113:'` y `rtk git status`.
