# Arsenal original — hito 66

El juego recupera los cinco `WeaponType` de Gloom: Soul Reaper, Sniper,
ShotGun, MiniGun e IronHellGoat. Las constantes proceden de `archetypes.txt` y
las transiciones de los pares `Weapon`/`WeaponAmmo` del checkout legado fijado.
La simulación corre a 60 Hz y convierte magnitudes espaciales con el factor 0,15.

| Arma | Principal | Secundario | Munición |
|---|---|---|---|
| Soul Reaper | rayo de 80, 0,5 s, sin gasto | inicia el tirón del objeto apuntado | 1 lógica |
| Sniper | rayo de 70, 1,5 s | rayo expansivo de 30, 0,3 s | 10; gasto 1/2 |
| ShotGun | doce proyectiles de 9 | devuelve los proyectiles vivos | 60; gasto 1 |
| MiniGun | rayo de 5 cada 0,1 s; reduce dispersión | carga 50 cartuchos en 10 s y descarga daño acumulado | 200 |
| IronHellGoat | carga una bola de 50–100 durante 2 s | redirige las bolas vivas | 30 |

La bola interpola radio 2–5, velocidad 0,15–0,035 y explosión 10–30 en unidades
antiguas. ShotGun e IronHellGoat conservan proyectiles autoritativos; sus
posiciones se replican en un bloque acotado de 32 y se presentan a ambos clientes.
El slice tiene dos combatientes, de modo que conserva el evento expansivo de
Sniper pero no puede demostrar un tercer blanco encadenado.

Cada `WeaponComponent` contiene propiedad, munición por arma, arma activa,
cadencia y carga. La autoridad permite adquirir, recargar hasta el máximo y
seleccionar solo armas poseídas. `1`–`5` selecciona, botón izquierdo usa el
principal, botón derecho el secundario y `Q` activa la habilidad de clase.

El protocolo 16 añade `weapon_command`, secuenciado y ligado a la entidad de la
sesión. Los snapshots validan propiedad, topes, carga, proyectiles y valores
finitos; una reanudación recibe el inventario completo. El HUD muestra arma,
munición, espera y carga.

Los `.mesh` originales de las cuatro armas que faltaban se importan offline y
se cocinan junto al Soul Reaper. MiniGun usa la pose base de su rig antiguo. Las
cinco escenas permanecen residentes y FPS/TPS eligen de forma independiente por
el arma replicada.

Los recogibles, el tirón concreto del Soul Reaper y el respawn están integrados
en el hito 67, con protocolo 18. La munición recogida antes de adquirir un arma
se conserva como reserva limitada; no concede su propiedad. Los modificadores
temporales afectan a los ataques emitidos y la cadencia del arsenal.
`gloom.legacy_arsenal` cubre las
cinco máquinas de estado, inventario, consumo, límites, carga, retorno, guiado,
proyectiles, round trip de red y daño después del vuelo.

## Corrección jugable — hito 69

Las velocidades de IronHellGoat proceden del desplazamiento Legacy por
milisegundo, no por tick: tras aplicar la escala 0,15 son 22,5–5,25 m/s. La
conversión anterior usaba 60 en vez de 1000 y dejaba el vuelo 16,67 veces más
lento. La bola nace delante de la cápsula, barre su radio contra la geometría,
se destruye en el primer contacto y aplica daño de explosión con caída radial.

El impacto replica si es explosivo y activa una única presentación coordinada:
sonido `fireball_hit`, destello de explosión y estela de fuego. No se superponen
dos clips de explosión para el mismo contacto. Los proyectiles magnéticos también
respetan desde este hito los sólidos del blockout y la malla de Factory.
