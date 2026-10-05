# Hito 116: registro de entidades con componentes estables

Terminado el 5 de octubre de 2026. Base `2b98e83` (131 solo prepara Hound; último C++ 115).
Ponytail full, sin push ni inicio automático del 117. Setup Hound 131 conservado.

## Decisión previa a la implementación

Tipos usados: daño, transform, física de personaje, vida, escudo, movimiento,
arma, habilidad, selección, autoridad, replicación, presentación, puntuación,
movimiento cinemático y cuatro cuerpos físicos (estático/dinámico/cinemático/trigger).
Identificadores explícitos compartidos, sin RTTI ni registro automático global.

Se eligen páginas de ocho slots por tipo, indexadas por EntityId::index. La tabla
de punteros puede crecer; las páginas y los componentes vivos no se mueven.
Solo se construyen slots ocupados, mediante la construcción inline ya existente;
retirar componente/entidad destruye inmediatamente el objeto, exactamente una vez.
Clear conserva páginas y capacidad, invalida generaciones y reutiliza free list.
Punteros prestados válidos hasta remove/destroy/clear/destrucción de su registro,
sin copias o movimientos del registro. No retenerlos tras retirar su componente.

Alternativa descartada: Array contiguo de componentes obligaría a reemplazar los
doce punteros de Combatant y revisar todos los caches; además movers/destructores
de cuerpos físicos requieren cuidado. No aporta una ventaja medida que justifique
esa ampliación. Las páginas de ocho limitan desperdicio a siete slots por tipo
en la última página; coste de lookup: tabla + página + bit de presencia.
La tabla de tipos está cerrada sobre los 18 tipos reales; añadir un tipo exige
un ID explícito, no cambia el límite de entidades. No ECS arquetípico ni arenas.

## Resultado y medidas

Registro nativo sin mapas, RTTI, jerarquía virtual ni excepciones. EntityId
conserva sus ocho bytes y se pasa por valor; Span también por valor. Los doce
punteros de Combatant permanecen válidos al crecer y retirar otros componentes.
Replicación, generaciones, reutilización de índices y destrucción inmediata
exactamente una vez se conservan. No cambia renderer, recursos ni protocolo 22.

Mil ciclos de crear/componer/consultar/vaciar a 8, 64 y 1.024 entidades:
**cero llamadas new y cero bytes solicitados** después de calentar, en Debug y
Release. El crecimiento frío sí asigna y su control positivo pasa.
Cinco pasadas normales con 1.024 entidades: cuatro consultas y lecturas bajan
de 153,125 a 8,594 ns por entidad; componer/destruir, de 1.435,059 a 113,867 ns.
La serie integrada Factory/Hound no muestra regresión ni una mejora global
de FPS atribuible al registro. [Tablas, método y límites](medidas.md).

Memoria llena a 1.024, incluidos registro y heap propio medido:
1.233.067 → 786.328 bytes (**-36,23 %**). El registro pasa de 120 a 920 bytes.
Clear conserva páginas para recomponer sin asignaciones: heap vacío
785.408 frente a 213.082 bytes antes, liberado al destruir el registro.
La tabla de páginas cubre el índice máximo aunque solo se asignan páginas
ocupadas. Estas cifras no representan RSS ni el heap global del juego.

## Validación y cierre

Builds completos Debug/Release y formato correctos. Crecimiento de 2.050
entidades, 1.024 composiciones, alineación de 64 bytes, componentes inmóviles,
handles caducados, remove, clear repetido, teardown y replicación comprobados.
Precondiciones de emplace verificadas mediante hijos Debug que disparan assert.
Flags efectivos /GR- /EHs-c- en entity.cpp y entity_tests.cpp en ambos builds.

CTest completo: **Release 50/54 (107,93 s), Debug 51/54 (257,49 s)**.
Persisten los tres comparadores visuales heredados; animation_network falla
con GNS send 25 solo en Release, como antes. Entidades, física, simulación,
replicación, combate, movimiento, Vulkan y Hound pasan; no hay fallos nuevos
identificados por causa. No se cambian referencias ni umbrales.

Las **1.200 capturas alineadas** contra el 115 pasan el comparador original
y sus 172 grupos. Diferencia máxima de canal 1/255; media RGB máxima
0,001444/255 y hasta 0,429688 % de píxeles distintos. Trece pares inspeccionados;
mismos 41 eventos, 5.410 partículas creadas, 4.746 expiradas y cero descartadas.
La alineación de frame/TAA existe solo en el capturador temporal de caché.

LegacyArsenal, World/Subsystem y WorldSnapshot conservan sus fronteras
120/122/127. La captura del snapshot sigue asignando su vector fuera de la
ventana del registro; no se declara cumplimiento global de C++ sin STL.
Se amplía únicamente la prueba existente: **198 archivos C++ propios /54 CTest**.
Diagnóstico del runtime OFF; contador independiente solo en la prueba.

Estado, ficha, índice e inventario actualizados. Commit local de cierre:
`rtk git log -1 --oneline --grep='^hito 116:'`. Sin push; 117 no iniciado.
