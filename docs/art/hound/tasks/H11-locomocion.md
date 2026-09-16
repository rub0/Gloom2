# H11 — locomoción y transiciones

## Entrada y alcance
Leer [contexto](CONTEXTO.md) e [índice](README.md); requiere H08 y contrato H06.
Si H09/H10 ya terminaron, partir de esa fuente acumulada preservando UVs/materiales.
Consultar [ANIMATION_VFX](../../../ANIMATION_VFX.md) y
`src/gameplay/character_animation.cpp` para estados y mezcla actuales.

Inventariar los clips recuperados (idle, forward, jump, strafe_right) como
posible base, no garantía de calidad. Retargetear considerando bind/ejes/
proporciones, o crear movimiento nuevo si la adaptación no sirve.
Cubrir reposo, avance/retroceso, desplazamiento lateral y saltar/caer/aterrizar
con las transiciones que exija H06. Revisar esquivas existentes sin cambiar su física.
Movimiento de humanoide pesado pero ágil, compatible con la velocidad real.

## Fuera de alcance
Bite/armas/reacciones de H12, modificar velocidad/autoridad/root motion jugable,
texturas y animaciones extra sin estado consumidor. No copiar claves Legacy a ciegas.

## Comprobaciones y entrega
Ciclos sin saltos, apoyos/pies sin deslizamientos graves, arcos y cambios de
dirección fluidos, contactos de capucha/armadura, bounds en movimiento.
Validar glTF y reproducción **animada** en el motor, mezclas y cortes temporales.
Comparar pasos temporales 30/60/144 Hz como estabilidad, no como benchmark de FPS.
Entregar clips con nombres/duración/frecuencia/loop, tabla de estados y vídeo,
fuente acumulada y exportación. Cierre común.

## Criterio de cierre
Locomoción funcional y de calidad revisable a velocidad de juego, sin que
el clip diagnóstico sustituya caminar. Registrar capas procedurales retenidas
y pendientes; toda integración de prueba debe ser acotada, no reemplazo definitivo.
