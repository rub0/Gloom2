# Hito 70: habilidades originales del roster

Fecha: 9 de septiembre de 2026. Cierre técnico; queda pendiente la aceptación
jugable y subjetiva del usuario.

## Auditoría y decisiones

Se contrastaron los componentes Legacy `Hound.cpp`, `Archangel.cpp`,
`LifeDome.cpp`, `Shadow.cpp` y `CameraFeedbackNotifier.cpp`, los arquetipos del
servidor y los recursos de partículas/audio. El arquetipo del servidor prevalece
para duraciones y cooldowns. El roster cubierto es Hound, Archangel y Shadow;
Screamer continúa fuera porque no tiene modelo ni definición de contenido propia.

| Clase | Q | Regla restaurada | E | Regla restaurada |
| --- | --- | --- | --- | --- |
| Hound | Bite | golpe autoritativo de corto alcance ya existente | Berserker | 20 s activo y 20 s de cooldown; fidelidad literal: solo estado, audio y olor, sin modificar daño ni cadencia |
| Archangel | Diamond Skin | inmunidad a daño de combate durante 5 s; 25 s de cooldown; la lava sigue siendo letal | Life Dome | cura inmediata de 10 puntos solo al propio Archangel, nunca al enemigo; presentación 10 s y cooldown 10 s |
| Shadow | Invisibility | invisibilidad visual durante 5 s; 25 s de cooldown | Flash | radio Legacy de 150 unidades (22,5 m), exige que el objetivo mire hacia Shadow y respeta línea de visión; cooldown 1 s |

La Cúpula Legacy recorría jugadores sin resolver equipos y podía curar al rival;
por decisión expresa del usuario esta versión cura únicamente al lanzador. El
código local antiguo describía bonificaciones Berserker no conectadas, pero la
ruta ejecutable Legacy no las aplicaba; por decisión expresa se conserva esa
literalidad y se podrá calibrar después. Guard y los IDs anteriores siguen siendo
compatibles. Las habilidades secundarias se activan con `E`; `Q` conserva la
primaria.

## Autoridad, red y presentación

El servidor valida activación, cooldown, vida, distancia, orientación y geometría.
Primaria/secundaria, duración, cooldown y factor de Flash forman parte del snapshot;
primer snapshot, pérdida/reordenación y reconexión reciben el estado vigente sin
reproducir one-shots históricos. Muerte, respawn y cambio de selección limpian los
estados. El contrato cambia a protocolo **21**.

Se importaron y cocinaron ocho recursos de habilidad (`houndBite`, `houndSmell`,
`archangelShield`, `lifeDome`, `shadow`, `shadowIn`, `shadowOut` y `shadowFlash`).
La auditoría queda en 89 archivos, 46 importados, tres referencias ausentes
conocidas y un grupo duplicado. Se añadieron el loop de invisibilidad, one-shots
autoritativos, HUD Q/E, tinte/material, pantalla de Flash y recetas reproducibles
para olor y cúpula. El renderer es single-threaded en esta ruta y no añade
reservas dinámicas al tick autoritativo; los cálculos de Flash solo se ejecutan
al activar `E`.

## Reproducción y validación

```powershell
cmake --build --preset windows-debug
ctest --preset windows-debug --output-on-failure
.cache/legacy-tools/Scripts/python.exe tools/legacy/audit_audio.py D:/Projects/Gloom-Legacy --check
build/windows-vs/Debug/gloom.exe --ability-review reports/abilities-70/captures
```

`--ability-review` genera seis vistas deterministas, una por habilidad. Las PNG
inspeccionadas están en [`captures`](captures); los PPM son la salida original del
renderer. El smoke Vulkan/Jolt terminó correctamente en una GTX 1070, sin comandos
rechazados. Las pruebas cubren reglas, inmunidad y lava, autocuración sin curar al
rival, Berserker sin bonificaciones, orientación/rango/LOS de Flash, cooldowns,
limpieza, serialización, dos clientes, pérdida/reordenación y reconexión.

No se declara aceptación humana: falta jugar cada habilidad en una sesión real de
dos clientes y decidir si conviene calibrar Berserker o la presentación visual.
