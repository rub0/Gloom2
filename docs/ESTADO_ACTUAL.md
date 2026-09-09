# Traspaso de Gloom

Actualizado: 9 de septiembre de 2026, tras cerrar técnicamente el hito 70.
Siguiente paso: revisión jugable de las seis habilidades con el usuario.
Actualizar este documento al cerrar cada hito; guardar el detalle en informes
y crear el commit de cierre según `AGENTS.md`.

## Estado y cuidado del workspace

- Proyecto: `D:\Projects\Gloom`. Original: `D:\Projects\Gloom-Legacy`.
- El commit de cierre del 68 consolida también la definición previa de su alcance
  y todos los recursos originales que necesita esta versión. Consultar `git log -1`
  para su hash.
  Revisar `git status` antes de editar y conservar cualquier cambio nuevo;
  no usar reset, checkout ni clean para limpiar. Commit local y push son acciones
  distintas: no asumir que el último hito ya está subido a GitHub.
- Instrucción permanente del usuario: **crear un commit al terminar cada hito**,
  después de validar y actualizar documentación. Informar del hash al cerrar.
- Hitos 63–64: animación, skinning, FPS/TPS, partículas e interfaz implementados.
  65–66: física original y arsenal implementados. **67: recogibles implementados**.
- El usuario confirmó que las últimas correcciones de movimiento funcionan.
  No reabrir esa física ni los hitos anteriores salvo regresión concreta.

## Decisiones aceptadas que deben conservarse

- Momento con retención terrestre `pow(0.8, dt/0.016)` al soltar y aérea
  `pow(0.98, dt/0.016)`. No poner a cero la velocidad al soltar o aterrizar.
- Salto normal: 90 % del impulso original. Esquiva sin reducción: doble WASD
  o doble Espacio en 450 ms, con dirección; una esquiva aérea por salto.
- Jolt: recuperar apoyo con `RefreshContacts` antes de `ExtendedUpdate`.
  Las superficies transitables no restan momento horizontal; paredes/techo
  cancelan impulso hacia el obstáculo. Sin empuje descendente constante en suelo.
- Vida/escudo: barras verticales exteriores al marco de armas, símbolos
  originales. Cooldown en círculo lateral. FPS medidos y XYZ arriba a la izquierda.
- Lava letal al contacto incluso con escudo. Relleno ambiente 0,65 y luces +20 %.
- Protocolo actual **21**; replica habilidad primaria/secundaria, cooldown, estado
  activo y factor de Flash. Documentos históricos que indican protocolos 15–20 describen entregas
  previas.

## Hito 67, recogibles de Factory

14 arquetipos en 73 ubicaciones auditadas: recompensas únicas en la autoridad,
topes, reserva de munición sin arma, adquisición, respawn y tirón del Soul Reaper.
Disponibilidad/posición/respawn y efectos se replican en snapshots completos;
primer snapshot tardío y reconexión conservan el estado. Las mallas siguen ese estado.

Decisiones: consumir incluso al tope; seleccionar el arma recogida si se llevaba
Soul Reaper. Efectos de 15 s, refresco sin apilar: daño ×3, reducción saturada
al 100 % con mínimo de un tick. Expiración independiente y limpieza al morir.
Soltar el tirón, cambiar de arma o morir devuelve el objeto al origen sin premio.
`SniperAmmo1` genérico `Ammo` queda visual: está fuera de los 14 arquetipos y
73 ubicaciones que cuenta la auditoría (hay 74 registros visuales con recompensa).

Validación: **9/9 pruebas focalizadas** y cuatro capturas Vulkan inspeccionadas.
Informe: `reports/pickups-2026-09-06/README.md`. No se repitió la suite completa
ni una sesión manual con dos clientes gráficos. La revisión jugable del usuario
está pendiente; el cierre técnico no implica esa confirmación manual.

Si se revisan recogibles, leer solo la sección 67 de
[GAMEPLAY_MIGRATION.md](GAMEPLAY_MIGRATION.md) y las partes pertinentes del informe.

## Hito 68, audio original

Backend SDL3 desacoplado con fallback nulo, mezclador a 48 kHz, buses, límite de
voces, loops y lifecycle seguro. Música compartida entre menú/partida; pausa atenúa
música y silencia efectos/ambiente. Listener en cámara local, primera persona
estéreo y fuentes remotas/impactos/recogibles/nueve ambientes de Factory en 3D.
Volúmenes configurables con `GLOOM_AUDIO_MASTER`, `GLOOM_AUDIO_MUSIC` y
`GLOOM_AUDIO_EFFECTS` en `[0,1]`.

Auditoría reproducible de 89 archivos: 37 importados/cocinados, 3 referencias
ausentes conocidas, un grupo duplicado y `troll` excluido. PCM GAU1 preserva la
salida decodificada y valida formato, canales, frecuencia, duración y finitud.
Reglas: `docs/AUDIO.md`; inventario: `assets/audio/inventory.json`; informe:
`reports/audio-2026-09-06/README.md`.

Pasos cada 365 ms solo con desplazamiento y apoyo. Umbrales físicos convertidos:
aterrizaje -6,5625 m/s y gruñido -18,75 m/s. Disparos/cargas, impactos, explosión,
retorno/guiado, sin munición, cambio/adquisición, daño, muerte y respawn emiten
eventos autoritativos. Diario de 128 eventos; protocolo 19 envía solo el último
segundo con redundancia. Primer snapshot, repetición/reordenación y reconexión no
reproducen one-shots históricos; los bucles vigentes se reconstruyen.

Validación: **11/11 pruebas focalizadas**, auditoría/cooker y smoke SDL3 real sobre
`Headphones (High Definition Audio Device)`. Secuencia regenerable de 58 s en el
informe. Sigue pendiente que una persona confirme subjetivamente volumen, timbre
y espacialización; el cierre técnico no sustituye esa escucha.

## Último hito cerrado: 69, revisión jugable

La captura del usuario confirmó tres regresiones. IronHellGoat convertía como ticks
una velocidad Legacy por milisegundo y volaba 16,67 veces demasiado lento; sus
proyectiles no consultaban la geometría y `Jumper1` seguía explícitamente diferido.

IronHellGoat vuela ahora a 22,5–5,25 m/s según carga, nace delante de la cápsula,
barre su radio contra sólidos/triángulos y destruye/explota en el primer contacto.
Daño radial, estela y destello son autoritativos/replicados. Un impacto emite solo
`fireball_hit`, sin el segundo clip de explosión que se solapaba. `Jumper1` aplica
en autoridad y predicción su fuerza original (4,6875/32,8125/0 m/s) y reproduce
`gameplay/plasma.wav` en 3D. Protocolo **20**.

Validación: **12/12 pruebas focalizadas**, auditoría de 89 audios con 38 importados,
smoke Vulkan de 90 frames (381 partículas, 0 descartadas) y ejecutables cliente/
servidor recompilados. Informe: `reports/gameplay-fixes-2026-09-07/README.md`.
Queda pendiente la nueva confirmación jugable/subjetiva del usuario.

## Último hito cerrado: 70, habilidades de clase

Las seis habilidades del roster original disponible están integradas. Hound usa
Bite (`Q`) y Berserker (`E`); Archangel, Diamond Skin y Life Dome; Shadow,
Invisibility y Flash. Guard permanece como loadout compatible. Screamer sigue
fuera del roster porque carece de modelo y definición de contenido propia.

Decisiones del usuario: Berserker conserva fidelidad literal a la ruta ejecutable
Legacy —20 s de estado, olor y audio, sin bonus de daño/cadencia— y podrá calibrarse
después. Life Dome cura 10 puntos solo al propio Archangel, nunca al enemigo.
Diamond Skin inmuniza frente a combate durante 5 s sin impedir la lava letal.
Flash conserva alcance, orientación y línea de visión Legacy. Muerte, respawn y
cambio de selección limpian estados y loops.

La autoridad valida las activaciones; snapshots y protocolo **21** replican ambas
habilidades, cooldowns, estados y factor de Flash bajo pérdida/reordenación,
primer snapshot y reconexión. HUD Q/E, audio original y VFX FPS/TPS están
integrados. Inventario de audio: 89 auditados, 46 importados, tres ausentes
conocidos y un duplicado.

Validación: build Debug completo, **47/47 pruebas**, auditoría de audio y smoke
Vulkan/Jolt. Se inspeccionaron seis capturas deterministas generadas mediante
`gloom --ability-review DIR`. Informe y comandos:
`reports/abilities-70/README.md`.

Siguiente paso: sesión jugable humana con cada habilidad y dos clientes. Ese pase
puede abrir ajustes de Berserker o presentación, pero no queda funcionalidad del
alcance técnico diferida. No hay un hito 71 definido todavía.

El ejecutable actualizado es `build/windows-vs/Debug/gloom.exe`; el servidor
`build/windows-vs/Debug/gloom_slice_server.exe` está recompilado con protocolo 21.
Revisión de audio: `gloom --audio-review DIR [--device]`.

## Mapa mínimo de archivos

- Reglas auditadas: `assets/gameplay/legacy_rules.json` y
  `tools/legacy/audit_gameplay.py`; código fuente de referencia en el checkout
  Legacy, sobre todo `Src/Logic/Entity/Components` y `Exes/media/maps`.
- Ubicaciones/collider: `assets/legacy/factory_scene.json`,
  `src/gameplay/factory_scene.cpp`.
- Inventario: `include/gloom/gameplay/legacy_arsenal.hpp` y su `.cpp` en
  `src/gameplay`; ya existen `acquire`, `add_ammo`, `owns`, `ammo` y `select`.
- Recogibles: `include/gloom/gameplay/legacy_pickups.hpp` y `.cpp`;
  `tests/legacy_pickups_tests.cpp`. Revisión visual: `gloom --pickup-review DIR`.
- Simulación, daño y proyectiles: `src/gameplay/vertical_slice.cpp`;
  contratos en `include/gloom/gameplay/vertical_slice.hpp` y `components.hpp`.
- Red: `src/gameplay/vertical_slice_network.cpp`,
  `src/network/movement_replication.cpp`, `include/gloom/network/protocol.hpp`.
- Presentación: `apps/gloom/main.cpp`, `apps/gloom/game_ui.hpp`.
- Audio: contratos en `include/gloom/audio`, mezclador en `src/audio`, backend
  SDL3 en `src/backends/sdl_audio.cpp`; eventos/presentación en `src/gameplay`.
- Pipeline de audio: `tools/legacy/audit_audio.py`, contenido en `assets/audio`,
  reglas en `docs/AUDIO.md` e informe en `reports/audio-2026-09-06`.
- Regresiones: `tests/legacy_arsenal_tests.cpp`, `tests/vertical_slice_tests.cpp`,
  `tests/vertical_slice_network_tests.cpp`, `tests/legacy_movement_tests.cpp`.
- Habilidades: contratos en `slice_selection.hpp`, `vertical_slice.hpp` y
  `components.hpp`; autoridad en `vertical_slice.cpp`, red en
  `vertical_slice_network.cpp` y revisión con `gloom --ability-review DIR`.

## Validación y herramientas

- Revisión de HUD/colisiones: 43 pruebas validadas (40 en pasada completa y
  3 visuales tras inspeccionar y actualizar referencias).
  Informe: `reports/gameplay-fixes-2026-09-05/README.md`.
- Última revisión, escaleras/momento: **7/7** pruebas focalizadas, con geometría
  real de Factory, techo, reposo en pendiente, red y smoke gráfico. Confirmación
  posterior del usuario. Informe: `reports/stairs-momentum-2026-09-05/README.md`.
  No se ha repetido la suite completa después de esta última revisión.
- Hito 67: **9/9**, incluyendo recogibles, arsenal, simulación, dos clientes con
  contención/primer snapshot tardío/reconexión/respawn, protocolo, GNS, movimiento,
  servidor dedicado y smoke gráfico. Logs/capturas en el informe del hito.
- Hito 68: **11/11**, auditoría y corrupción WAV/OGG/MP3, mezclador/backend nulo,
  eventos, dos clientes bajo pérdida/reordenación, reconexión, protocolo 19,
  arsenal/recogibles/movimiento/transporte, dedicado, smoke de dispositivo y
  smoke gráfico Vulkan/Jolt.
- Hito 69: **12/12** focalizadas sobre físicas, arsenal, audio/red, Factory, movimiento,
  VFX, simulación/red/transporte, protocolo y dedicado. Auditoría 89/38 y smoke
  Vulkan de 90 frames; no se repitió la suite completa ni una partida manual.
- Hito 70: **47/47** pruebas Debug, auditoría 89/46 y smoke Vulkan con seis
  capturas inspeccionadas. Falta la aceptación jugable/subjetiva humana.
- PowerShell; CMake/CTest: `D:/Dev/CMake/bin`, preset `windows-debug`.
  Ejecutable: `build/windows-vs/Debug/gloom.exe`.
- Compilar motor antes de enlazar ejecutables. Con
  `/p:BuildProjectReferences=false` se omite reconstruir dependencias: usarlo
  únicamente cuando ya estén actualizadas, para no validar una biblioteca vieja.
- Python disponible: `.cache/legacy-tools/Scripts/python.exe` (Ogre, Pillow,
  numpy, av). PyAV puede inspeccionar/decodificar los WAV/OGG/MP3 durante la
  importación offline; el runtime no debe depender de este entorno. `python`
  puede apuntar al alias de Microsoft Store.

## Trabajo con contexto reducido

Leer este documento primero; abrir fuentes con `rg` y rangos pequeños según
la necesidad. No cargar todos los informes, JSON grandes ni historial del chat.
Redirigir builds/pruebas a logs y mostrar solo errores/resultados. Pruebas
focalizadas durante cambios; ampliar por riesgos de integración concretos y
requisitos del hito. Actualizaciones breves cuando haya avances relevantes.
Al finalizar, actualizar estado, decisiones, validación y siguiente paso aquí;
crear el commit del hito, comprobar `git status` e informar del hash.
