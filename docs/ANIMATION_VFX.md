# Animación y VFX: hito 63

Implementación y aceptación en [el informe reproducible](../reports/animation-vfx-2026-09-03/README.md).
El contrato técnico está en [ADR 0064](architecture/0064-animation-and-combat-effects.md),
que corresponde al hito **63**. La interfaz del hito 64 queda pendiente.

## Contenido y movimiento

Archangel/Hound reproduce los cuatro clips originales de Ogre: `forward`
(0,666667 s), `idle` (0,5 s), `jump` (0,333333 s) y `strafe_right` (0,666667 s).
Cada clip exporta 43 pistas de hueso como 129 canales TRS. Se conservan tiempos,
unidades, ejes, pesos y jerarquías. La traslación de Ogre se suma al bind local;
la rotación se compone `bind * delta` y la escala se multiplica. La pelvis visual
no desplaza la cápsula ni conduce la autoridad mediante root motion.

Retroceso y strafe izquierdo reutilizan los ciclos originales al revés. La
mezcla de velocidad, apuntado de torso/cuello, IK de dos huesos, respiración,
aterrizaje, recoil, daño, equipar y caída de muerte son capas nuevas. Shadow
**no tenía clips**: su balanceo de torso/cola y las respuestas de combate son
procedurales. Se respeta su rebase y raíz invertida del hito 62. La caída se
aplica en el espacio normalizado de 1,8 m, no alrededor del origen lejano del rig.

Soul Reaper conserva su malla y materiales. Mano, apoyo y boca de fuego salen
de la pose evaluada. El arma usa un socket de agarre normalizado; la boca de
fuego queda delante del extremo recuperado del cañón. Los brazos FPS reutilizan
triángulos completos controlados por brazo/mano de los dos modelos. Una pose
de hombros específica deja el corte de la geometría fuera de cámara. No hay
mecánica de recarga ni cambio a otra arma: no se han inventado para animarlas.
Equipar se muestra al iniciar, cambiar identidad o reaparecer.

## Pipeline

La escena cocinada es versión **5**. glTF acepta animación TRS `LINEAR` y `STEP`;
rechaza `CUBICSPLINE`, morph targets, targets con matrices, tiempos repetidos,
valores no finitos, cuaterniones inválidos y referencias fuera del rig. Hay
límites de 256 clips/huesos GPU y un millón de claves agregadas. Se conservan
las ocho influencias; Shadow utiliza cinco en algunos vértices.

CPU evalúa, interpola cuaterniones por el arco corto y mezcla poses. GPU deforma
posición, normal, tangente y posición anterior; las sombras usan la misma pose.
Los bounds se calculan sobre los vértices deformados. La paleta GPU está separada
de las constantes por objeto y solo se actualiza para mallas animadas. La ruta
estática y la revisión de bind pose del hito 62 siguen disponibles.

Teleport, muerte, respawn, cambio de identidad y reconexión invalidan el historial
de pose/transformación. El arma hereda el corte del cuerpo. La cámara conserva
su control de cortes temporal existente. La animación no modifica cápsulas,
rayos de disparo, daño (80), cooldown (0,5 s) ni respawn (4 s).

## Partículas y eventos

`assets/effects/recipes.json` define 14 recetas. Hay simulación balística CPU
analítica, semillas estables, ráfagas, emisores continuos, color/tamaño sobre
vida, billboards y cintas orientadas por movimiento. Capacidad: 2.048 partículas
y 64 emisores; se descarta el nacimiento nuevo al saturarse. Los nacimientos se
ordenan por tiempo e ID antes de aplicar capacidad, para conservar el resultado
a 30/60/144 Hz. La vida máxima admitida es 10 s. Cancelar propietario elimina
emisor y partículas; no renovar un emisor lo detiene y sus partículas expiran.

Se auditan 349 scripts/materiales y 358 fuentes con hash; se recuperan siete
texturas. Son adaptaciones acotadas de Particle Universe, sin Ogre en runtime:
energía cian, humo/cola y estelas rojas, fogonazo, impacto, daño, escudo,
aterrizaje, aparición, muerte, lava y calor. IronHellGoat renueva un emisor de
humo en cada posición replicada del cohete y su impacto usa la explosión jugable,
reforzada a 28 partículas. La sangre continúa limitada a la revisión.

El render ordena transparencias de atrás hacia delante, conserva profundidad,
mezcla alfa/aditiva y HDR/bloom, y suaviza intersecciones de partículas con
profundidad. Los halos aditivos no vuelven a multiplicar la intensidad original
por su máscara alfa. El calor lee una copia de color/profundidad opacos, con
flujo normal lineal BC5, rechazo de primer plano y bordes suaves. Las copias se
hacen fuera del render target antes de la pasada transparente. Las partículas
no escriben profundidad ni vectores de movimiento; su vida corta y el historial
subyacente evitan velocidades falsas de un quad recién nacido.

Protocolo **14** añade pitch y el último disparo aceptado: secuencia, tick,
contacto, impacto y resultado. Conserva IDs 0/1/2/3. Los efectos de disparo usan
solo confirmaciones; la predicción de movimiento continúa, pero un comando
especulativo no crea un segundo fogonazo. Esta decisión añade la latencia de
confirmación a la respuesta visual. Snapshots repetidos/reordenados se ignoran;
una nueva conexión establece una línea base y no reproduce eventos viejos.
El dedicado no instancia renderer, animadores ni partículas.

El fogonazo confirmado usa un propietario de partícula separado y traslada sus
partículas mientras viven según el socket animado del arma. Al expirar deja de
buscar ese propietario, por lo que no añade un recorrido permanente por frame.

## Reproducir

```powershell
& 'D:\Dev\CMake\bin\cmake.exe' --build --preset windows-debug
& 'D:\Dev\CMake\bin\ctest.exe' --preset windows-debug --output-on-failure
.\build\windows-vs\Debug\gloom.exe --animation-review .cache\animation-review\30fps 30
.\build\windows-vs\Debug\gloom.exe --animation-review .cache\animation-review\60fps 60
.\build\windows-vs\Debug\gloom.exe --animation-review .cache\animation-review\144fps 144
.\build\windows-vs\Debug\gloom.exe --lava-review .cache\animation-review\lava
.\build\windows-vs\Debug\gloom.exe --effects-review .cache\animation-review\samples
```

Cada secuencia principal dura 40 s simulados y guarda 1.200 PPM a 30 imágenes/s.
En 144 Hz, una captura puede caer hasta un tick después del instante nominal.
Son pasos de render fijos para comparar comportamiento, no un benchmark que
afirme alcanzar esas frecuencias en tiempo real. Primeros 20 s: inspección;
últimos 20 s: luces normales de Factory. Cada bloque de 10 s muestra idle,
avance/retroceso/strafe, salto/aterrizaje, disparo/daño/escudo, muerte durante
cuatro segundos, muestras de VFX, cámara arriba/abajo y respawn. Lava tiene una
secuencia independiente de 3 s con las luces del mapa.
`--effects-review` acerca sangre, explosión y calor a una zona despejada para
inspeccionar las muestras originales durante 3 s.

`tools/review/animation_report.py` codifica los vídeos, compara frecuencias y
genera contactos, hashes y resultados JSON. `gloom.animation_network` recorre
Factory y prueba combate local y host/join sobre GNS. `gloom.animation_dedicated`
lanza el servidor real en otro proceso y dos clientes GNS independientes dentro
del proceso de prueba, con presentación CPU, reconciliación y reconnect. Las
secuencias GPU complementan esa prueba de transporte; no son capturas de esa
partida de red.

Para regenerar, usar Python con Pillow y el entorno offline Ogre documentado
en `CHARACTERS.md`. `import_effects.py` solo necesita Pillow:

```powershell
python tools/legacy/import_effects.py --legacy-root D:/Projects/Gloom-Legacy --output-root assets/effects
python tools/legacy/verify_effects.py --legacy-root D:/Projects/Gloom-Legacy --reference-root assets/effects --work-root .cache/effects-reproduction
python tools/legacy/verify_characters.py --legacy-root D:/Projects/Gloom-Legacy --reference-root assets/characters/original --work-root .cache/character-reproduction
```

Se mantienen los originales y todas las referencias de imagen de 58/61/62.
No se afirma equivalencia exacta con Ogre/Particle Universe ni captura de
movimiento de Shadow: sus acciones nuevas son autoría procedural sobre el rig
recuperado. El detalle de dedos sigue limitado por la geometría original.
