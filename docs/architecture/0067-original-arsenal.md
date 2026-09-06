# ADR 0067: arsenal original determinista y autoritativo

## Estado

Aceptado para el hito 66.

## Decisión

`LegacyArsenal` es una máquina de estado de tick sin dependencias de render,
SDL o transporte. Emite acciones tipadas; la autoridad resuelve rayos, crea
proyectiles y aplica daño. Inventario y munición pertenecen a `WeaponComponent`.
Protocolo 16 replica el resultado y transporta botones continuos mediante un
comando ligado al propietario de sesión.

ShotGun e IronHellGoat usan proyectiles autoritativos acotados. MiniGun y Sniper
conservan acciones diferentes aunque alcancen mediante consultas lineales. Soul
Reaper conserva la acción de tirón que el sistema de objetos del hito 67 enlaza
con un `ItemSpawn` concreto. La presentación carga las cinco geometrías y no
calcula daño, cadencia ni munición.

El límite de 32 proyectiles mantiene fijo el tamaño máximo del snapshot. La
topología de dos combatientes no permite probar la expansión de Sniper a un
tercer enemigo; la acción y el radio quedan conservados.

