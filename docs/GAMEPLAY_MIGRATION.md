# Migración del gameplay original

Inicio: 4 de septiembre de 2026, tras el hito 64. La petición abarca física
del personaje, armas y objetos recogibles. Se organiza en tres entregas con
aceptación independiente; el arsenal original no se sustituye por cinco
variantes de Soul Reaper.

## Fuente y alcance

La auditoría reproducible está en `assets/gameplay/legacy_rules.json`. Recupera
las tablas literales de `archetypes.txt`, Factory standalone y sus mapas cliente
y servidor, sin ejecutar Lua. Registra los hashes de 28 fuentes de datos/código,
cinco armas, catorce tipos de recogible y **73 ubicaciones en cada mapa**.
Las posiciones siguen en unidades originales, con conversión explícita ×0,15.

```powershell
python tools/legacy/audit_gameplay.py --legacy-root D:/Projects/Gloom-Legacy --output assets/gameplay/legacy_rules.json --verify
```

## 65 — Física del personaje

Estado: integración en validación. `AvatarController.cpp` devuelve desplazamiento
por tick, no velocidad en metros/segundo. `GameState.cpp` y `Map.cpp` fijan ticks
de **16 ms**. La conversión a velocidad es momentum ×0,15/0,016. La simulación
moderna conserva sus 60 Hz; el factor de retención se eleva a `dt/0,016` para
mantener la respuesta de aceleración y frenado.

| Perfil | Velocidad límite terrestre | Impulso de salto normal (−10 %) | Esquiva original XYZ |
|---|---:|---:|---|
| Hound | 12,1875 m/s | 15,1875 m/s | 2,5 / 2 / 2,5 |
| Archangel | 7,5 m/s | 11,8125 m/s | 1,7 / 1,9 / 1,7 |
| Shadow | 8,4375 m/s | 12,65625 m/s | 2 / 2 / 2 |

Se recuperan aceleración y giro terrestres, retención aérea 0,98 por tick,
control aéreo, gravedad, velocidad vertical límite de seis veces el momentum
máximo, salto y esquiva desde suelo. `frictionCoef=14` existe en los datos pero
**no se usa en el cálculo original**: al soltar movimiento se multiplica por 0,8.
La revisión de escaleras recupera ese frenado gradual: se aplica
`pow(0.8, dt / 0.016)` al soltar las teclas, también al aterrizar de una esquiva.
La retención aérea sigue siendo `pow(0.98, dt / 0.016)`. El impulso del salto
normal se reduce un 10 %; el impulso de la esquiva conserva sus valores.

WASD mueve, Espacio salta y dos pulsaciones de la misma tecla WASD en menos
de 300 ms esquivan. También se admite doble Espacio en 450 ms mientras se
indica una dirección: el primer toque salta y el segundo permite una sola
esquiva aérea antes de volver al suelo. La esquiva usa la dirección de movimiento actual; al mantener
varias teclas puede ser diagonal. El original aplicaba el impulso de la última
tecla por separado. Las repeticiones automáticas del teclado no cuentan como
segunda pulsación. Los menús y la pérdida de foco limpian la detección.
Salto y esquiva sobreviven a frames de render sin tick de simulación; se consumen
una sola vez en el siguiente tick y se descartan cuando la interfaz bloquea el juego.

La cápsula tiene radio 0,45 y cilindro de altura 0,9 m (total 1,8 m). Se elimina
el empuje vertical artificial de contacto. La consulta CharacterVirtual proyecta
la velocidad contra las normales de contactos reales: corta el impulso hacia
el techo y conserva la componente tangencial. Las superficies transitables
resuelven el desplazamiento sin restar momento horizontal almacenado. Antes de
cada consulta se recuperan los contactos de apoyo para que Jolt siga el suelo
al bajar escaleras y permita saltar. Se elimina también el empuje descendente
constante sobre suelo, evitando deriva en pendientes al terminar de frenar.
Jolt resuelve las colisiones:
no se afirma identidad bit a bit con el controlador PhysX antiguo. El escalón
se convierte a 0,075 m; el margen de contacto explícito de Legacy (0,5) también
se convierte a 0,075 m. El límite de pendiente usa `acos(0,707)`, aproximadamente
45 grados: comentar una asignación no desactiva el valor por defecto de PhysX.
Los valores por defecto se contrastaron con la
[referencia de NVIDIA](https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/apireference/3.3.4/files/classPxControllerDesc.html).

Autoridad y predicción usan la misma función pura y perfiles elegidos por el
servidor. El bit de esquiva viaja en los comandos redundantes; se consume una
vez y se rechazan bits reservados. **Protocolo 17** (incluye disponibilidad de esquiva aérea en snapshots completos
y deltas) y nueva huella de Factory
impiden mezclar predicción antigua y nueva. El laboratorio de blockout conserva
su movimiento de pruebas; la restauración se aplica al Factory jugable.

## 66 — Arsenal original

Completado en [ARSENAL.md](ARSENAL.md) y ADR 0067: inventario, cambio, munición,
primarios/secundarios, cooldowns, carga, proyectiles autoritativos replicados,
presentación FPS/TPS original y HUD. Los valores se contrastan con arquetipos y
componentes; no se inventa un sistema de cargadores.

| Arma | Comportamiento a migrar | Reserva máxima |
|---|---|---:|
| Soul Reaper | Ataque cercano y comportamiento secundario original | Sin consumo normal |
| Sniper | Rayo primario, interacción con quemadura y secundario expansivo | 10 |
| ShotGun | 12 proyectiles magnéticos, quemadura y retorno secundario | 60 |
| MiniGun | Disparo sostenido, dispersión y carga secundaria | 200 |
| Iron Hell Goat | Bola cargable: gasto, radio, velocidad y explosión variables | 30 |

La autoridad conserva daño/munición, reconexión, un límite de 32 proyectiles y
las cinco geometrías recuperadas. El tirón de objetos del Soul Reaper queda
tipado y se conecta a las entidades concretas en el hito 67.

## 67 — Objetos de Factory

Implementado: 14 arquetipos en las 73 ubicaciones auditadas, con adquisición,
reservas de munición incluso sin poseer el arma, vida/escudo limitados y respawn.
La recogida consume el objeto incluso al alcanzar el tope. Al adquirir un arma
desde Soul Reaper se selecciona automáticamente, como en `WeaponsManager`.

Los modificadores duran 900 ticks (15 segundos): +200 % de daño equivale a ×3;
el reductor del 200 % se satura al 100 %, con un mínimo de un tick para una
espera originalmente positiva. Repetir el efecto refresca su duración, sin
apilarlo; ambos temporizadores expiran independientemente y se limpian al morir.
El tirón reserva la identidad original, mueve el recogible mientras se mantiene
el secundario y lo devuelve al origen al cancelar. La recompensa y el respawn
comienzan al recogerlo; otro jugador puede interceptarlo.

Protocolo 18: snapshots completos de disponibilidad, posición del tirón,
respawn y duración de modificadores, incluyendo primer snapshot tardío y
reconexión. Las mallas se ocultan/mueven exclusivamente con ese estado.
`SniperAmmo1` de tipo genérico `Ammo` no pertenece a los 14 arquetipos auditados:
conserva su presentación previa y no añade una ubicación jugable número 74.

Validación y decisiones en el [informe del hito 67](../reports/pickups-2026-09-06/README.md).
Audio y nuevas habilidades de clase permanecen fuera del alcance.

La validación de escaleras y momento está en
[el informe de revisión](../reports/stairs-momentum-2026-09-05/README.md).
