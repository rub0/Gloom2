# Hito 67 — Recogibles de Factory

6 de septiembre de 2026. Implementación sobre el workspace existente sin commit;
se conservaron los cambios de los hitos anteriores.

## Reglas e integración

- `FactoryScene` carga los 14 arquetipos en las 73 ubicaciones auditadas del
  manifiesto, con recompensa, arma, radio y respawn resueltos por ubicación.
  La huella de Factory incorpora identidades, posiciones y reglas de objetos.
- `LegacyPickups` concede la recompensa una sola vez por disponibilidad. Si
  coinciden contactos, gana la entidad con menor ID. Un objeto se consume
  incluso con vida, escudo o munición al máximo, como `SpawnItemManager`.
- Vida: +17/+5 hasta 250. Escudo: +100/+50 hasta 150. Armas: Sniper +10,
  ShotGun +15, MiniGun +150, IronHellGoat +10. Munición: +5/+15/+30/+5,
  hasta 10/60/200/30. La munición sin arma se guarda y no concede propiedad;
  recoger un arma teniendo Soul Reaper la selecciona automáticamente.
- Respawn: 55 s armadura grande, 120 s modificadores, 20 s munición y 25 s
  resto. Se reinicia en la posición original. El estado de partida permanece
  en la autoridad al reconectar.
- Efectos: 15 s, +200 % de daño = ×3. Reducción del 200 % saturada al 100 %,
  mínimo de un tick en cooldowns originalmente positivos. Refresco sin apilar;
  expiración independiente y limpieza al morir. El código original podía
  generar cooldowns negativos y restauraba los temporizadores cruzados.
- Soul Reaper: rayo de 37,5 unidades contra objetos, reserva exclusiva,
  movimiento hacia el jugador (.005 × milisegundos² × .15 por tick), cancelación
  al soltar, cambiar de arma o morir. La recompensa llega al contacto, permite
  interceptación y comienza entonces el respawn. Como el filtro `eITEM` original,
  el rayo de selección y el recorrido del tirón no prueban paredes.
- Protocolo 18: disponibilidad, posición, fase, propietario del tirón y ticks
  de respawn por identidad; temporizadores de efectos en cada combatiente.
  Payload acotado y validado, incluyendo fases, propietario, tiempos, finitud,
  tamaño y topes de munición. El bloque de objetos se omite en el mapa compacto.
- Presentación: las mallas originales usan `source_node` para seguir el snapshot.
  MiniGun utiliza su malla residente del arsenal; los dos modificadores tienen
  marcadores de color. El HUD muestra la duración restante de cada efecto.

## Discrepancia de datos resuelta

`factory_scene.json` contiene 74 registros con `reward`: el adicional es
`SniperAmmo1`, de tipo genérico `Ammo` y sin arquetipo recuperado. La auditoría
`audit_gameplay.py` excluye ese tipo y fija 73 ubicaciones en los tres mapas.
Este hito respeta esas 73 y conserva el objeto genérico como presentación previa.
No se ha alterado el JSON ni ampliado el alcance a un decimoquinto arquetipo.

Fuentes contrastadas en `D:/Projects/Gloom-Legacy/Src/Logic/Entity/Components`:
`SpawnItemManager.cpp`, `WeaponsManager.cpp`, `WeaponAmmo.cpp`, `SoulReaper.cpp`,
`SoulReaperAmmo.cpp`, `PullingMovement.cpp`; datos de `archetypes.txt` y del
manifiesto local, además de `assets/gameplay/legacy_rules.json`.

## Validación

Motor recompilado antes de enlazar ejecutables. **9/9 pruebas focalizadas aprobadas**: recogibles,
arsenal, simulación, red, protocolo, transporte real GNS, movimiento, servidor
y smoke Vulkan. El registro final se guarda en `validation.log`.

La nueva prueba de recogibles recorre todas las ubicaciones, recompensa/topes,
contención, respawn exacto, reserva de munición, efectos extremos e independientes,
reserva/cancelación/contacto del tirón y rechazo de snapshots inválidos.
La prueba de red hace disputar `Orb4` a dos clientes reales del adaptador,
mantiene al segundo sin snapshot inicial hasta después de recogerlo, reconecta
y comprueba que el respawn termina disponible para ambos. No modifica posiciones
ni concede recompensas mediante accesos especiales de prueba.

Revisión visual Vulkan reproducible:

```powershell
build/windows-vs/Debug/gloom.exe --pickup-review reports/pickups-2026-09-06/captures
```

Capturas inspeccionadas: [disponible](captures/pickup-available.png),
[tirón](captures/pickup-pulling.png), [recogido](captures/pickup-collected.png),
[reaparecido](captures/pickup-respawned.png). El escudo azul se desplaza, desaparece
completamente y vuelve a su posición. Esta inspección usa snapshots de revisión;
la concesión y sincronización temporal se prueban por separado en la autoridad/red.

No se repitió la suite completa ni una sesión manual con dos clientes gráficos.
Audio, habilidades adicionales, despliegue externo y optimización general no
forman parte de esta entrega. No hay un hito 68 definido en el roadmap actual.
