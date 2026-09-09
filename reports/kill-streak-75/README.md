# Hito 75 — rachas de bajas y audio

## Alcance

Se migró el aviso sonoro de racha de Gloom Legacy. La racha consecutiva se
calcula en la autoridad, se reinicia al morir y se replica en el snapshot.
Siguiendo `Scoreboard::showSpreeMessage`, el aviso suena en 3, 6, 9, …, 27
bajas consecutivas. El sonido Legacy es un único `feedback/bell.mp3`; los
mensajes de texto de `spreeMsg.swf` quedan fuera de este hito porque la HUD
actual no tiene todavía ese panel de anuncios.

## Cambios

- `ScoreComponent::current_spree` y `CombatantView::current_spree`.
- Incremento solo para bajas atribuidas a otro combatiente; reinicio por muerte
  de jugador o ambiental.
- Nuevo `audio::Cue::spree`, con `feedback/bell.mp3` importado y cocinado por
  `tools/legacy/audit_audio.py`.
- El evento se genera desde snapshots autoritativos y solo se reproduce para el
  jugador local.
- Protocolo de gameplay actualizado de 21 a 22.

## Validación

```text
Audio audit: 89 files, 47 imported, 3 missing references, 1 duplicate groups
gloom_audio_tests: passed
gloom.vertical_slice_network: passed
gloom.network_protocol: passed
gloom.audio_network: passed
gloom.audio_no_device: passed
```

La prueba focalizada `gloom.audio` cubre umbral, no-umbral, reinicio de racha,
roundtrip de red y deduplicación de presentación.
