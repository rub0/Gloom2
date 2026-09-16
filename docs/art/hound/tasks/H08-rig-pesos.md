# H08 — rig, pesos y agarres de producción

## Entrada y alcance
Leer [contexto](CONTEXTO.md) e [índice](README.md); requiere H07 y H06.
Consultar `src/gameplay/character_animation.cpp` y
`include/gloom/gameplay/character_animation.hpp` solo para el contrato de poses/anclajes.

Estabilizar pivotes/ejes/bind pose y jerarquía. Añadir solo controles útiles de
autoría; separar controles de huesos exportados y hornear constraints conforme H06.
Refinar pesos y volumen en hombros/codos/muñecas, cadera/rodillas, cuello/capucha.
Transferir/validar skinning de cada LOD. No asumir que los 53 huesos actuales
deban crecer ni que coincidir en nombres haga compatible el rig Legacy.

Definir sockets y poses de agarre de las armas existentes, incluyendo apoyo de
la segunda mano donde corresponda. Probar FPS/TPS y mirada arriba/abajo.
Mandíbula/rig facial solo si H06 demuestra una necesidad y el usuario la acepta.

## Fuera de alcance
Animaciones finales, texturas, retopología extensa, nuevas armas o cambios de física.
Si la prueba exige cambiar topología, resolverlo y documentarlo antes de H09.

## Comprobaciones y entrega
Matriz de poses extremas y funcionales: peso finito/normalizado, límite de influencias,
piezas rígidas estables, sin colapsos, agarres y contactos; exportar/reimportar poses.
Probar en motor deformación y anclajes, no solo el visor estático; si falta el
modo de prueba, documentar esa dependencia, no afirmar validación.
Entregar fuente del rig, glTF de prueba, tabla de huesos/sockets, límites y vídeo.

## Criterio de cierre
Rig estable y documentado para H09/H11, con LODs y agarres comprobados.
Conservar una acción diagnóstica separada; no publicarla como locomoción.
Aplicar cierre común con entrada exacta para la siguiente tarea.
