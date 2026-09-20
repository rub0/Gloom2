# Hito 95 — H05, entrega para revisión global de Hound

20 de septiembre de 2026. Únicamente H05. Workspace limpio al comenzar;
entrada v12/H04, commit `0643723`. **Subhito técnico terminado; H05 en revisión.**
[Galería, vídeo y fuente candidata](../../docs/art/hound/review-h05/README.md).
Aceptación visual explícita: pendiente, sin fecha ni texto atribuido al usuario.
No se ha empezado H06 ni otra ficha. Sin push.

## Resultado

Se compararon concept original, boceto 01 y v02 aprobada con la fuente acumulada.
Frente/perfil/espalda/tres cuartos comparten cámara ortográfica y escala.
Se revisaron proporciones, perfil rectilíneo de capucha, espalda, jerarquía de
formas y anatomía visible. Se conservan los hitos visuales aprobados: hombros
libres, filos claviculares, capucha adelantada y dorso de manos en pico.
La escultura es más lisa y regular que el concept; se muestra esa diferencia
para aceptación, sin atribuirla a materiales que todavía no existen.

No se identificó un remate pequeño que exigiera alterar la forma de referencia.
Los fallos de poses/arma requieren trabajo posterior de articulación, encaje
o topología, fuera del rig diagnóstico conservado por H05.
No se propone ni ejecuta un rediseño de silueta.

Se guardan Blender y glTF/BIN v13 como **copias exactas de v12**.
El manifiesto de integridad registra origen, tamaños y SHA-256 de los tres archivos.
La copia no cambia nombres internos, huesos, bind pose, pesos ni claves.
Se comprobaron otra vez sus hashes después de renders y pruebas.
La condición de inmutable es una regla de trabajo respaldada por hashes/Git;
no se cambia el permiso de escritura del archivo. Cualquier revisión de formas
debe crear otra candidata, sin sobrescribir esta.

36.466 vértices / 72.568 triángulos; 98 componentes, 87 rígidos, siete materiales,
un mesh/skin y 53 huesos. No aumenta coste de ejecución ni cambia C++,
dependencias, gameplay o presentación jugable. Densidad de autoría, no presupuesto.

## Validación realizada

- Fuente congelada reabierta: 98 componentes conexos, manifold, cerrados,
  aristas orientadas y volumen positivo. Pesos normalizados; máximo dos influencias.
- Verificador H04 ejecutado sobre esa fuente: conservación, 61 muestras del
  clip, rigidez y 37 poses/16.428 evaluaciones locales pasan.
- Nueva auditoría global: 33 poses independientes, 98 componentes,
  **4.753 pares por pose / 156.849 evaluaciones**. Incluye todos los pares
  del cuerpo y autointersecciones no adyacentes de cada componente.
  Registra los cruces; no declara que el modelo esté libre de colisiones.
- Arma original: otros 196 pares, todos los componentes contra Soul Reaper
  en dos poses. Se conserva la escala 0,43 y el apoyo diagnóstico de H01;
  al elevar el brazo el arma sigue la mano mediante su transformación.
- Roundtrip de la exportación v13 en 13 poses: **0,000002068570 m** máximo.
  43 nombres Legacy, 53 huesos, una malla/skin, siete primitivas y un clip.
  El verificador compartido admite una ruta glTF opcional; regresión v12
  con su ruta anterior pasa con el mismo error máximo.
- Cooker v13: escena `7886512575590165434`, cero dependencias externas.
  Visor Vulkan estático: 7/7 piezas visibles, siete batches, salida 0.
  Captura inspeccionada; no prueba reproducción animada dentro del motor.
- CTest Release `gloom.assets` / `gloom.gpu_assets`: 2/2.
  Debug `gloom.animation_vfx`: 1/1. Binarios existentes; sin cambios C++.
- 26 PNG y 13 fotogramas clave inspeccionados. Vídeo H.264 completamente
  decodificado: 331 frames, 900×1000, 30 fps, 11,033333 s.
  La escala de siluetas es altura visible 200/100/50 px, no distancia de juego.
- Sintaxis, longitudes de líneas, rutas/enlaces, diff, exclusiones y hashes revisados.
  No se versionan cachés, logs, JSON de diagnóstico ni copias .blend1.

### Muestras de la auditoría global

Reposo y 50/100 % de cada uno de los 16 casos siguientes; se restauran todos
los huesos antes de cada caso y se comprueba la matriz identidad.

| Casos | Rotaciones en ejes del personaje, convertidas al hueso |
| --- | --- |
| Alcance | Ambos UpperArm −50° X, Forearm −20° X |
| Torso, izquierda/derecha | Spine ±10° Z, Spine1 ±10° Z |
| Paso, izquierda/derecha | Muslo adelantado −25° X, rodilla 30°; retrasado +15°, rodilla 15° |
| Cadera, izquierda/derecha | Muslo −40° X, rodilla 50° |
| Cabeza, izquierda/derecha | Head ±35° Z |
| Puño | Dedos 32°/38°/20°; pulgar 16°/22°, signos por mano |
| Muñeca, flexión/extensión | Hand ±25°, signos por mano |
| Mirar arriba/abajo | Neck −10°/+10° X, Head −20°/+20° X |
| Codo | Ambos Forearm −85° X |
| Combinada | Paso izquierdo y giro izquierdo de torso |

Las guardas de cadera se articulan explícitamente como los muslos.
El vídeo usa segmentos de un segundo con ida/vuelta sinusoidal para alcance,
ambos giros/pasos, dedos, muñeca, cabeza y caso combinado. No es locomoción
ni animación final; la pose extrema de codo se presenta en PNG.

## Hallazgos y límites para malla, UVs y rig

[Inventario completo: cada par con cruces y sus máximos](contacts.md).
Los conteos BVH representan pares de caras, dependen de la teselación y no miden
profundidad. No detectan toda contención ni certifican distancia continua.

| Hallazgo | Evidencia | Consecuencia |
| --- | --- | --- |
| Codo a 85° | 37 autointersecciones por brazo; cero en los demás casos/componentes | Resolver pliegue y distribución de pesos/topología en H07/H08 |
| Mirar abajo | Cabeza/torso: 114 pares, frente a cero en reposo; capucha se pliega | Revisar pivotes, holgura y pesos de cuello/capucha en H08 |
| Puño | Falange distal 2C/guante: 12 pares por lado al 50/100 % | Afinar inserciones del guante y falanges antes del agarre definitivo |
| Antebrazo bajo armadura, en reposo | Brazo/carcasa: 28 por lado; brazo/dorso de mano: 25; también placas/filos/aro | Revisar superficie interior y recortes ocultos en H07/H08 |
| Apoyo Soul Reaper, reposo | Brazo 619, placa 91, carcasa 255, aro 136, clavícula 10, filos 3+11 | Apoyo global inválido; resolver orientación, socket y agarre |
| Apoyo Soul Reaper, alcance | Brazo 616, placa 91, carcasa 255, aro 136 | Elevar el brazo no resuelve el montaje del arma |

El apoyo sigue sin cruzar guante/dedos: demuestra por qué la comprobación
local de H01 no podía certificar un agarre de todo el personaje.
No se modifica el arma original ni se exporta el montaje de revisión.

El inventario también incluye uniones constructivas solapadas: placas y
respaldos, raíces de paños/guardas, ojos/boca/cuello, falanges/guante y capucha
sobre pecho/esternón/torso. No se clasifica automáticamente todo solape como
aceptable por existir en reposo. Hay que distinguir inserciones ocultas de
cruces visibles antes de retopología/UVs; podrían exigir ajustes de encaje.
Las 33 poses no cubren carrera, sentadilla profunda, torsiones extremas,
agarre bimanual, apuntado final, todas las combinaciones ni estabilidad física.
El vídeo FK no garantiza apoyo de pies ni equilibrio dinámico.

## Traspaso exacto y puerta de aprobación

Maestra candidata para futura H07/H09:
`art/characters/hound/v13/hound-sculpture-v13.blend`.
Exportación: `assets/characters/hound_rig/v13/hound-rig.gltf` y `hound-rig.bin`.
Integridad: `art/characters/hound/v13/sculpture-reference.json`.
Evidencias: `docs/art/hound/review-h05/README.md`.

Nombres internos conservados: escena `Hound_Mesh_v12`, malla `H12_DeformMesh`,
rig `Hound12_Rig`, acción `Hound12_joint_check`, colección `HOUND_v12_EXPORT`.
Entrada anterior v12 y todas las versiones previas conservadas.

H05 está **en revisión**, no hecha. Falta aceptación explícita de las formas
por el usuario, después de ver los renders y límites. Esa decisión se registrará
con fecha y texto breve en el índice; este subhito no habilita H07.
No hay UVs, texturas, rig, sockets, clips finales ni integración jugable.
Commit local del subhito 95; resolver con `git log --oneline --grep='^hito 95:'`.

## Reproducción

Todos los procesos fueron locales. El sandbox fallaba al iniciar tanto lectura
como edición; se usó ejecución revisada fuera de él. No se abrió un servidor
ni se cambió la seguridad del MCP. Los generadores temporales no guardan
sobre la referencia. `--freeze` se niega a sobrescribir una candidata existente.

Comandos realmente ejecutados, con logs en `.cache/hound-h05/`:

```powershell
rtk proxy .cache/legacy-tools/Scripts/python.exe tools/art/review_hound_h05.py --freeze
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v13/hound-sculpture-v13.blend `
  --python-exit-code 1 --python tools/art/review_hound_h05.py -- --audit
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v13/hound-sculpture-v13.blend `
  --python-exit-code 1 --python tools/art/review_hound_h05.py -- --stills
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v13/hound-sculpture-v13.blend `
  --python-exit-code 1 --python tools/art/review_hound_h05.py -- --video
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v13/hound-sculpture-v13.blend `
  --python-exit-code 1 --python tools/art/verify_hound_mesh_v12.py --python tools/art/verify_hound_rig_roundtrip_v03.py `
  -- v12 --gltf assets/characters/hound_rig/v13/hound-rig.gltf
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v12/hound-mesh-v12.blend `
  --python-exit-code 1 --python tools/art/verify_hound_rig_roundtrip_v03.py -- v12
rtk proxy .cache/legacy-tools/Scripts/python.exe tools/art/board_hound_h05.py
rtk proxy powershell -NoProfile -Command "New-Item -ItemType Directory -Force '.cache/hound-h05/cooked' | Out-Null"
rtk proxy build/windows-vs/Release/gloom_asset_cooker.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-h05/cooked `
  game:/characters/hound_rig/v13/hound-rig.gltf cache:/hound-rig.gasset
rtk proxy build/windows-vs/Release/gloom_scene_viewer.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-h05/cooked `
  game:/characters/hound_rig/v13/hound-rig.gltf cache:/hound-rig.gasset 1 D:/Projects/Gloom/.cache/hound-h05/gloom-preview.ppm
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Release --output-on-failure -R '^gloom[.](assets|gpu_assets)$'
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Debug --output-on-failure -R '^gloom[.]animation_vfx$'
```

El primer intento de cooker falló porque no existía la carpeta de caché;
creada la carpeta, cooker y visor terminaron correctamente. La primera copia
visual de v02 omitía evaluar sus transformaciones: se corrigió y se regeneró
la lámina completa antes de entregar.
