# Traspaso de Gloom

Actualizado: 10 de septiembre de 2026, hito 82 de aprobación de Hound v02.
El usuario aprueba el diseño 3D v02 con «apruebo el diseño». Conservar esa fuente,
la guía 0.1 y el boceto 2D 01. Siguiente paso artístico: preparar la malla y un rig
de prueba en una versión nueva; validar deformación y agarres antes del acabado final.
Sigue pendiente revisar la partida en Release, medir combate/HUD/audio y reducir memoria residente.
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
- Protocolo actual **22**; replica habilidad primaria/secundaria, cooldown, estado,
  factor de Flash y racha de bajas. Documentos históricos que indican protocolos 15–21 describen entregas
  previas.

## Último hito cerrado: 82, diseño de Hound v02 aprobado

El usuario acepta el diseño presentado en el hito 81 con «apruebo el diseño».
La [v02 y sus detalles](art/hound/blockout-v02/README.md) quedan como referencia
aprobada de silueta y volúmenes: hombros libres, armadura clavicular, capucha
más cerrada y manos orientadas con placa dorsal completa en pico.

Registro documental únicamente: no se modifica el modelo ni se inicia el rig.
El siguiente hito debe conservar v02, trabajar en otra versión y preparar la
malla para un rig de prueba. Comprobar caminar, apuntar y Bite, además de
deformaciones, penetraciones y agarres. Evaluar el rig existente antes de
decidir reutilizarlo. UVs/materiales finales e integración jugable siguen pendientes.

Validación de diff, enlaces locales y alcance exclusivamente documental;
no se repiten pruebas del motor para esta aceptación. Informe:
`reports/hound-approval-82/README.md`. Commit local, sin push.

## Hito 81, Hound v02 — clavículas, capucha y manos

Fuente: `art/characters/hound/v02/hound-blockout-v02.blend`.
[Vistas y detalles](art/hound/blockout-v02/README.md). La versión 01 se conserva
sin cambios. El usuario pidió liberar los hombros, acercar la armadura a la
clavícula, tapar más la cara y corregir las manos con una placa dorsal completa en pico.

Copas de hombro retiradas y deltoides descubiertos; filos anclados más medialmente.
Capucha adelantada y abertura más baja/estrecha. Palmas hacia los muslos,
pulgares anteriores, dedos que flexionan hacia la palma y cobertura dorsal
desde muñeca a nudillos con punta distal. Son cambios de volumen, no acabado.
113 piezas, 7.626 triángulos y 7 materiales, con altura de 1,80 m.

Validación de .blend y glTF, 67 piezas ajenas conservadas, reconstrucción del
BIN idéntica, regresión de v01, cooker/visor Vulkan con 113/113 piezas y
CTest **2/2**. Vistas y primeros planos inspeccionados. Sin rig, texturas finales,
pruebas de deformación/agarres ni sustitución del personaje jugable.
Informe: `reports/hound-blockout-81/README.md`. Hito con commit local, sin push.
La valoración artística de v02 queda aprobada en el hito 82; el acabado final
y la animación todavía requieren sus propias revisiones.

## Hito 80, volumen básico de Hound

Fuente editable: `art/characters/hound/v01/hound-blockout-v01.blend`.
[Frente/perfil/espalda y ficha](art/hound/blockout-v01/README.md), más tres cuartos.
105 piezas, 6.974 triángulos evaluados y 7 materiales planos. Altura 1,80 m,
igual al Archangel actual; ancho con filos 1,144 m y fondo 0,382 m.

Capucha, ojos naranja, torso por placas, brazos desnudos, hombreras/guanteletes
afilados y piernas. Rostro/manos son placeholders de volumen; espalda interpretada
provisionalmente. El facetado no cambia la dirección artística a low-poly.
No hay rig, texturas o UVs finales; no cambia el Hound jugable ni el gameplay.
La lámina de vistas proviene de la malla real, no de generación 2D.

MCP nativo de Blender ya disponible y usado directamente. Fuente .blend abierta
y validada en background; glTF validado y geometría reconstruida con BIN idéntico.
Cooker y visor Vulkan correctos (105/105 piezas), capturas revisadas y CTest **2/2**.
105 batches corresponden a piezas editables: optimizar después de aceptar formas.
No se ha validado deformación, agarres de armas, partida ni presupuesto de combate.
Informe: `reports/hound-blockout-80/README.md`.

Push solicitado y confirmado al repositorio público `rub0/Gloom2/main`:
siete commits publicados hasta `988437d` (hito 79), hash remoto comprobado.
El hito 80 se cierra con commit local; no extender automáticamente aquel push.
Las siguientes revisiones deben conservar v01 y usar una nueva versión.

## Hito 79, puente local Blender–Gloom

Blender portable 4.5.13 LTS y Blender MCP 1.9.1 instalados en `.cache`, con Python
aislado, perfil propio, telemetría desactivada, proveedores externos apagados y
modo seguro del MCP activo. `.codex/config.toml` registra cuatro herramientas
locales. No se abren puertos a la red ni se usan servicios 3D de pago.

Arranque: `tools/art/start_blender.ps1`. Flujo, reinstalación y límites de seguridad:
[BLENDER_WORKFLOW.md](BLENDER_WORKFLOW.md). La sesión iniciada antes del registro
puede necesitar recargar MCP para mostrar sus herramientas; se ha validado la
conexión mediante un cliente stdio real, no mediante herramientas nativas ya cargadas.

Fixture independiente: 3 mallas, 424 triángulos, 3 materiales y 1 PNG externo.
Exportar **glTF separado**, no GLB con texturas embebidas: el cooker actual las
rechaza. Metros, normales, UV0, conversión a Y-up y dependencias verificadas.
Visor con captura opcional y salida tras 32 frames con la escena residente.

Validación: build Release del visor/cooker y dos tests; CTest **2/2**, cliente MCP,
verificador de fixture, cocción y render Vulkan con **3/3 piezas visibles**.
Capturas Blender y Gloom inspeccionadas. No se repite la suite completa ni se
afirma validación de rigs/animaciones desde Blender. Informe:
`reports/blender-bridge-79/README.md`. No cambia el personaje jugable ni gameplay.

La primera fuente Hound fuera de cache y las vistas se entregan en el hito 80.
No hacer retopología, UVs, rig o texturas finales hasta la validación del volumen.

## Hito 78, guía artística y boceto de Hound — aprobados

Entregada la [guía 0.1](DIRECCION_ARTISTICA.md) y la lámina
`docs/art/hound/hound-concept-v01.png`: cuerpo completo, cabeza y guanteletes.
Referencia elegida por el usuario: `D:\Descargas\Gloom\concept_hound2.jpg`,
con copia intacta en `docs/art/characters/hound-original-concept.jpg`.
Hound es humanoide bípedo: capucha, piel pálida, ojos naranja y armadura afilada.
El diseño actual usa el cuerpo de Archangel, no un rig cuadrúpedo recuperado.

Identidad desde los concepts originales; UT3 orienta peso y materiales y el toque
de Overwatch 2 orienta claridad. El usuario acepta guía y lámina con «asi esta
perfecto, continua con lo siguiente». Conservar el peso de la armadura y la lectura
ligeramente pétrea de esta imagen. La aceptación no aprueba vistas aún no resueltas.

Validación documental, integridad de las tres referencias copiadas y revisión
visual de la lámina. Prompt y procedencia guardados; no corresponde compilar ni
repetir pruebas del motor para esta entrega de documentación y concept 2D.
Informe: `reports/art-direction-78/README.md`. El hito 78 no entregó modelo ni
instalación; el puente Blender/MCP posterior está validado en el hito 79.

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

## Hito 77, presentación visual de recogibles

El objetivo aprobado es que los objetos del mapa inviten a recogerlos mediante
flotación, giro y halo aditivo al estilo Quake. La lógica de disponibilidad,
recompensa, respawn y red del hito 67 no se modifica. La presentación será local
y estará anclada a la posición original del objeto para no introducir estado de
red ni deriva física.

Los 73 recogibles flotan hasta ±8 cm y giran a 0,72 rad/s con fases distribuidas.
Escudo, armas, munición y modificadores llevan un halo radial aditivo coloreado;
vida flota y gira sin brillo. Aparición y respawn interpolan en 0,45 s desde 35 %
de escala y opacidad cero. La presentación es local, no modifica autoridad ni
protocolo, no añade luces dinámicas ni sombras y el halo evita la copia de escena
de las partículas.

Builds Debug/Release, **48/48 pruebas** y cinco capturas Vulkan inspeccionadas.
El énfasis por proximidad y el destello de recogida continúan como pulido opcional;
queda pendiente la confirmación subjetiva en una partida humana. Informe:
`reports/pickup-presentation-2026-09-09/README.md`.

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
alcance técnico diferida. El siguiente hito definido es la primera pasada de
rendimiento descrita abajo.

## Último hito cerrado: 71, primera pasada de rendimiento

El renderer agrupaba batches de visibilidad pero los ignoraba al dibujar; ahora
los draws opacos comparten pipeline, recursos y bindings por mesh/material. Los
transparentes conservan su orden por distancia y existe compatibilidad para
snapshots sin batches. La iluminación ya no crea una `vector` por cada celda de
la rejilla 16×9×24: usa conteo plano, prefijo y relleno en dos pasadas. Las
paletas de skinning reservan su capacidad antes de insertar matrices.

Corrección del hito 74: los 108 FPS publicados medían solo el tramo de render de
una escena simplificada, no Factory jugable ni el frame completo. El resultado
histórico de la suite fue 47/47, pero no sirve como benchmark comparable con
Factory. Informe: `reports/performance-2026-09-09/README.md`.

Esta referencia no sustituye la medición de la partida humana a 1920×1080 ni
explica por sí sola los 13 FPS reportados. El siguiente frente es perfilar
skinning/presentación, asignaciones por frame y sombras en esa configuración.

## Hito 73, perfil 1080p y reservas de presentación

Se añadió `--vertical-slice-performance-1080p`, que ejecuta el escenario de
perfil a 1920×1080, mide el loop completo y expone instancias/batches. El caso
mide **32,32 FPS** en 720 frames: 30,9369 ms por loop, 29,4376 ms de
render/presentación, 6,84914 ms de visibilidad, 0,269229 ms de iluminación y
3,54779 ms GPU. Son 227 instancias sometidas, 71 visibles y 26 batches; la
memoria pico es 702,32 MB device-local y 42,67 MB host-visible. Corrección del
hito 74: esa media incluía carga y esperas de trabajos; no permitía atribuir el
coste a visibilidad, streaming o resolución. Además se ejecutaba el guion del
smoke simplificado sobre Factory y se habían omitido comprobaciones incompatibles.

La lista completa de instancias y las listas animadas reservan capacidad antes
de añadir proyectiles, personajes y efectos, reduciendo realojos y copias por
frame. Informe ampliado: `reports/performance-2026-09-09/README.md`.

## Hito 74, benchmark corregido y sombras

Factory original se mide a 720p/1080p con la misma cámara, contenido y simulación:
esperar todos los recursos solicitados, calentar 120 frames y medir otros 360,
resolución nativa fija, sin VSync ni capturas. Informe de media/p50/p95/p99/máximo,
seis etapas CPU y memoria de proceso. Fallo o cierre prematuro devuelve error.
El benchmark no incluye HUD/audio ni un recorrido de combate completo.

Se eliminó el sleep fijo de 16 ms del juego, que impedía alcanzar 100 FPS.
VSync queda desactivado por defecto; `GLOOM_VSYNC=1` lo activa en partida.
Se descartan conservadoramente las esferas fuera de cada volumen de sombras;
el descarte usa espacio de luz y contempla escala no uniforme/reflejada. Las
listas de presentación conservan capacidad entre frames; se intercambian las
listas de animación actual/anterior sin copiar toda la lista.

Ryzen 7 3700X / GTX 1070: Debug 1080p pasa de 41,273 a 32,934 ms con el benchmark
corregido (20,2 % menos). Release 1080p: **144,17 FPS**, media 6,936 ms,
p99 6,964 ms, máximo 7,111 ms. Release 720p: 546,76 FPS; ambas resoluciones
terminan con 224 instancias/85 visibles/35 batches. No atribuir el salto de
Debug a Release únicamente al cambio de código ni garantizar esos FPS en todo combate.

Validación: builds Debug/Release; siete pruebas focalizadas únicas pasan
(render_scene, visibility, factory/character_visual_review, vulkan_sync, ui_flow,
vertical_slice_smoke). Las 13 capturas de Factory/personajes coinciden píxel a
píxel con referencias. Se corrigió el límite de carga visual contado en frames
por un límite temporal después de un fallo reproducido. Sin nueva suite completa.
Memoria Release 1080p: 2085 MiB privados/1133 MiB working set; queda trabajo de
residencia y retención de recursos. Detalle: `reports/performance-74/README.md`.

Ejecutable para jugar y medir: `build/windows-vs/Release/gloom.exe`.
Comandos: `--vertical-slice-performance-720p` y `--vertical-slice-performance-1080p`.

El ejecutable actualizado es `build/windows-vs/Debug/gloom.exe`; el servidor
`build/windows-vs/Debug/gloom_slice_server.exe` está recompilado con protocolo 22.
Revisión de audio: `gloom --audio-review DIR [--device]`.

## Hito 75, rachas de bajas y audio

La autoridad mantiene `current_spree` por combatiente: aumenta únicamente al
conseguir una baja atribuida a otro jugador y se reinicia al morir, incluida la
muerte ambiental. Se replica junto a los snapshots con el protocolo **22**.
La semántica Legacy reproduce `feedback/bell.mp3` en 3, 6, 9, …, 27 bajas
consecutivas; el cliente solo lo presenta cuando el actor es el jugador local,
por lo que no hay sonidos duplicados por eventos remotos.

Validación: auditoría/cocción de audio (**47/89** importados), build Debug de
`gloom_audio_tests` y prueba focalizada de audio completa, incluyendo umbrales,
reinicio, serialización de racha y filtros local/remoto. Informe:
`reports/kill-streak-75/README.md`.

## Último hito cerrado: 76, marcador de partida

Mientras se mantiene `Tab` durante una partida aparece un marcador sobre el HUD,
sin pausar ni bloquear el control. Presenta los dos combatientes con nombre,
clase, bajas y muertes; ordena por bajas y usa menos muertes como desempate. El
jugador local queda resaltado. Los nombres se resuelven por entidad desde la sala
y la partida local usa etiquetas de reserva.

La implementación reutiliza los contadores autoritativos ya presentes en
`SliceSnapshot`, por lo que no cambia el protocolo **22**. Se añadió el estado 17
a la galería de revisión y referencias deterministas para 1280×720, 1920×1080 y
2560×1080. Informe y validación: `reports/scoreboard-76/README.md`.

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
