# Hito 88 — feedback de daño y VFX de armas

Fecha: 15 de septiembre de 2026.

## Resultado

- Una pérdida de vida o escudo activa durante 1,5 s una viñeta roja. Cuatro
  cuadriláteros interpolan alfa desde los bordes físicos hasta una abertura
  central transparente; también cubren las bandas laterales ultrawide.
- Un arco rojo alrededor de la mirilla indica la dirección del rival respecto a
  la orientación local. La captura de aceptación usa una fuente delante-derecha.
- Las partículas del fogonazo tienen un propietario separado y se trasladan con
  el socket animado del arma mientras siguen vivas. La búsqueda se desactiva al
  expirar la ráfaga.
- IronHellGoat renueva `rocket_smoke` en cada posición del proyectil. La receta
  produce 55 volutas por segundo durante 1,1 s, con crecimiento y ascenso suave.
- `explosion_review`, ya usada por el impacto autoritativo del cohete, aumenta de
  8 a 28 partículas y gana velocidad, dispersión, tamaño, brillo y duración.

No cambian protocolo 22, daño, radio, velocidad, colisiones, autoridad ni audio.

## Validación

- Build Debug: `gloom`, `gloom_ui_tests` y `gloom_animation_vfx_tests`.
- CTest focalizado: `gloom.ui`, `gloom.animation_vfx` y
  `gloom.animation_network`, 3/3.
- La prueba VFX comprueba la tasa de 55 partículas/s de humo, las 28 partículas
  de explosión, el traslado del fogonazo y la ausencia de duplicados.
- `gloom --effects-review .cache/damage-vfx-88/effects-final-visible`: 90 frames
  a 30 FPS, 489 partículas nacidas, 377 expiradas y cero descartadas. El fixture
  mueve `rocket_smoke` durante 1,6 s y conserva la explosión en el segundo 2;
  se inspeccionaron los frames 30, 45 y 66.
- `gloom --ui-review .cache/damage-vfx-88/ui-final`: 18 estados en 1280×720,
  1920×1080 y 2560×1080. Se inspeccionó la página 17 a 1280×720 y el encuadre
  ultrawide; el centro permanece intacto y el degradado alcanza los bordes.

## Pendiente humano

La captura confirma composición y dirección, pero una partida humana debe
validar subjetivamente si 1,5 s y la intensidad elegida resultan cómodos durante
combate continuo.
