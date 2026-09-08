# Hito 69 — revisión jugable de audio, IronHellGoat y jumper

Fecha de cierre técnico: 8 de septiembre de 2026.

## Evidencia recibida

Se inspeccionó la captura de 66,26 s proporcionada por el usuario. Entre 9–18 s
el personaje pisa `Jumper1` sin recibir impulso. Entre 48–63 s la bola naranja de
IronHellGoat cruza muy lentamente el pasillo, atraviesa la pared y desaparece sin
una explosión visible. El análisis de los eventos de audio confirmó que los
one-shots replicados no se repiten por snapshot; sí existían dos clips de impacto
solapados para una única explosión.

## Correcciones

- Velocidad Legacy convertida desde desplazamiento por milisegundo: 22,5 m/s sin
  carga a 5,25 m/s con carga máxima, 16,67 veces los valores erróneos anteriores.
- Nacimiento delante del jugador y barrido continuo del radio del proyectil contra
  sólidos del blockout y triángulos de Factory, evitando atravesar paredes.
- Consultas aceleradas por Jolt para evitar recorrer todos los triángulos por
  proyectil. Los perdigones también nacen delante de la cápsula y chocan con sólidos.
- Primer contacto autoritativo, daño expansivo con caída radial, destrucción,
  `fireball_hit` único, evento visual replicado, explosión y estela de fuego.
- `Jumper1` normalizado desde su posición, volumen y fuerza originales. Autoridad
  y predicción aplican el mismo impulso de 4,6875/32,8125/0 m/s. El evento usa el
  `gameplay/plasma.wav` original como fuente 3D.
- Protocolo 20 para impedir que clientes antiguos interpreten el nuevo estado de
  presentación o el nuevo identificador de audio.

## Validación

- 12/12 pruebas focalizadas: físicas, simulación, arsenal, audio, red de audio, movimiento,
  Factory, VFX, protocolo, transporte, servidor dedicado y dos clientes bajo enlace
  adverso. Incluyen regresiones específicas de velocidad, pared real, explosión
  única, round trip y jumper determinista.
- Auditoría de audio: 89 archivos, 38 importados, 3 referencias ausentes conocidas
  y 1 grupo duplicado. `plasma.wav` queda importado y cocinado como GAU1.
- Smoke Vulkan: 90 frames a 30 FPS, 381 partículas creadas, 269 expiradas, 0
  descartadas. La captura del destello de explosión fue inspeccionada.
- Ejecutables `gloom.exe` y `gloom_slice_server.exe` recompilados con protocolo 20.

La comprobación técnica no sustituye una nueva partida del usuario para valorar
subjetivamente mezcla, fuerza visual y sensación del jumper.
