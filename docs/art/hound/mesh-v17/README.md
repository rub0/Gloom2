# Hound v17 — H07, malla de producción

2 de octubre de 2026 · Hito 107. Escultura v16 conservada byte a byte.
39.504 triángulos TPS /13.834 FPS, 106 componentes, siete materiales TPS/cuatro FPS.
LOD1/2 automáticos: 19.747 /8.247 triángulos. Rig, pesos, UV0 y materiales siguen
siendo provisionales para las fichas posteriores; no son clips ni mapas finales.

[Fuente](../../../../art/characters/hound/v17/hound-production-v17.blend) ·
[glTF](../../../../assets/characters/hound_rig/v17/hound-rig.gltf) ·
[BIN](../../../../assets/characters/hound_rig/v17/hound-rig.bin) ·
[Manifiesto](../../../../art/characters/hound/v17/production-reference.json) ·
[Informe, validación y límites](../../../../reports/hound-production-107/README.md).

Escena `Hound_Mesh_v17`, malla `H17_DeformMesh`, rig `Hound17_Rig`,
acción `Hound17_joint_check`, colección `HOUND_v17_EXPORT`.

Imágenes reales regenerables, excluidas de Git por el encargo:

![V16 arriba, v17 abajo, mismas cámaras](../../../../.cache/hound-production-v17/renders/before-after.png)

[Rostro](../../../../.cache/hound-production-v17/renders/face.png) ·
[Garras](../../../../.cache/hound-production-v17/renders/claws-palm.png) ·
[Torso](../../../../.cache/hound-production-v17/renders/torso.png) ·
[Grebas/botas](../../../../.cache/hound-production-v17/renders/boots.png) ·
[Espalda](../../../../.cache/hound-production-v17/renders/back-assembly.png).

![Siluetas a 200/100/50 px](../../../../.cache/hound-production-v17/silhouettes.png)

![Los tres LODs cocinados](../../../../.cache/hound-production-v17/lod-review/lod-turnaround.png)

Los LODs se muestran ampliados para revisar diferencias; LOD2 no se usa a esa
distancia. Selección 30/80 por radio de cada primitiva, no del cuerpo entero.
Máximo superficial proyectado muestreado al cambiar LOD: 1,911 px en capucha
mirando abajo; transiciones nativas y límites en el informe.

Transiciones nativas (antes/después), cámara centrada en el material:
[Capucha antes](../../../../.cache/hound-production-v17/lod-review/transition-5-30--1-look-down.png) ·
[después](../../../../.cache/hound-production-v17/lod-review/transition-5-30-1-look-down.png) ·
[Filos antes](../../../../.cache/hound-production-v17/lod-review/transition-4-80--1-fist.png) ·
[después](../../../../.cache/hound-production-v17/lod-review/transition-4-80-1-fist.png) ·
[Ojos antes](../../../../.cache/hound-production-v17/lod-review/transition-6-30--1-rest.png) ·
[después](../../../../.cache/hound-production-v17/lod-review/transition-6-30-1-rest.png).

[Alcance en LODs](../../../../.cache/hound-production-v17/lod-review/reach.png) ·
[Mirar abajo](../../../../.cache/hound-production-v17/lod-review/look-down.png) ·
[Brazos FPS](../../../../.cache/hound-production-v17/lod-review/fps-reach.png) ·
[Vídeo de revisión](../../../../.cache/hound-production-v17/renders/global-check.mp4).

![Fotogramas inspeccionados](../../../../.cache/hound-production-v17/video-keyframes.png)

[Visor Vulkan estático](../../../../.cache/hound-production-v17/gloom-preview.png) ·
[Runtime, pose A](../../../../.cache/hound-production-v17/v17-lod0-audio-b-a.png) ·
[Runtime, pose B](../../../../.cache/hound-production-v17/v17-lod0-audio-b-b.png).
Las capturas del runtime llevan mapas de carga diagnósticos; la apariencia de
esos mapas no es una entrega artística. Cuatro pasadas: 305,90–309,88 FPS a
1080p, máximo 3,554 ms; el informe conserva una pasada previa con p99 de 4,242 ms.
Las capturas se toman durante calentamiento; el HUD incluye el arranque y no
equivale a la distribución de los 360 frames medidos después.

[Inventario completo](../../../../reports/hound-production-107/inventario.md) ·
[Mediciones sin descartar la previa](../../../../reports/hound-production-107/mediciones.md).
H08 no iniciada. Commit local del hito 107; sin push.
