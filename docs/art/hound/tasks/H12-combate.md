# H12 — apuntado, armas, habilidades y reacciones

## Entrada y alcance
Leer [contexto](CONTEXTO.md) e [índice](README.md); requiere H11 y H06/H08.
Leer solo lo pertinente de [ARSENAL](../../../ARSENAL.md),
[ANIMATION_VFX](../../../ANIMATION_VFX.md) y `src/gameplay/character_animation.cpp`.
Hound es humanoide: Bite no autoriza inventar una boca canina o una nueva mecánica.

Preparar apuntado, disparo/recoil, equipar/cambio de arma según estados existentes,
Bite, Berserker y Guard cuando el runtime los presente; daño, muerte y respawn.
Decidir qué retiene la capa procedural y qué necesita clip, sin duplicar movimientos.
Validar todos los agarres del arsenal vigente, apoyo y cañón, más visibilidad FPS.
Sincronizar gesto/VFX con eventos existentes; documentar eventos/tiempos visuales.

## Fuera de alcance
Daño/cooldowns/alcance, autoridad/red, nuevas habilidades o mecánica de recarga
inexistente. No alterar hitboxes/cámara para esconder contactos o manos incorrectas.
No rehacer texturas ni rig sin una dependencia concreta.

## Comprobaciones y entrega
Revisar combinaciones con locomoción, pitch extremo, cambio de arma, interrupción
por daño/muerte/respawn y repeticiones de evento. Comprobar que arma/manos/socket
se mantienen unidos y que cuerpo/capucha no atraviesan cámara en FPS.
Roundtrip, reproducción animada del motor, bounds y pruebas de animación/red
pertinentes a cambios de presentación. Registrar secuencias cubiertas y limitaciones.
Fuente, clips/poses/capas, tabla de eventos/estados/agarres y vídeo FPS/TPS.

## Criterio de cierre
Acciones legibles y coherentes a velocidad real, sin desincronizar eventos
autoritativos ni duplicar efectos. Presentar animaciones para revisión; conservar
materiales y locomoción existentes. Aplicar cierre común para H13.
