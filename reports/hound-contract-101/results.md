# Resultados H06 — subhito 101

Medidos el 23/09/2026; registrados el 26/09. Fuentes del hito 100; v16 intacta.
Estos valores persisten en Git; los JSON, capturas y logs se regeneran con [la receta](README.md).

## Controles GPU de materiales

14 capturas a 1280×720, una malla visible y un batch por captura.
Error absoluto medio por canal, escala 0–255, en ROI x=530..709/y=180..359.
Equivalencia: <3; cambio visible: >5. La ROI reduce la contaminación del borde
por jitter temporal. No es una validación colorimétrica exhaustiva o de todos los mips.

| Referencia | Variante | Esperado | Error medio ROI | Resultado |
| --- | --- | --- | ---: | --- |
| factor | color | close | 0.544064 | Pasa |
| orm | orm_factor | close | 1.059516 | Pasa |
| emission | emission_factor | close | 0.044383 | Pasa |
| factor | normal | different | 0.536204 | FALLA |
| factor | normal_x | different | 81.841626 | Pasa |
| factor | normal_y | different | 0.536204 | FALLA |
| factor | orm | different | 83.565844 | Pasa |
| orm | ao | different | 24.542973 | Pasa |
| factor | emission | different | 76.322284 | Pasa |
| uv0 | uv1 | different | 64.071481 | Pasa |
| uv0 | mask | different | 28.952521 | Pasa |
| mask | blend | different | 7.690597 | Pasa |

Diez comparaciones pasan; dos de normales fallan. La diferencia en X solo prueba
respuesta a esa entrada, no una dirección normal correcta. El shader reconstruye Z
y lee XY en RG. La expansión del UASTC de las muestras produce:

| PNG de entrada RGBA | KTX expandido RGBA |
| --- | --- |
| normal_x: (210,128,224,255) | [210, 210, 210, 128] |
| normal_y: (128,210,224,255) | [128, 128, 128, 210] |

Es un desacuerdo de canales que requiere corregir y probar la ruta de normales.
Las dos muestras independientes con variación G producen la misma captura en la
pasada de control. No se certifica el respaldo RGBA8 ni se modifica el motor.

## Importación y horneado

Constraint global horneada: dos huesos, dos influencias normalizadas, 31 claves
de rotación LINEAR no constantes; cocción correcta. No prueba reproducción GPU.
TANGENT invertido en una copia: payload cocinado exactamente idéntico al control.

| Control negativo | Resultado observado |
| --- | --- |
| reject_morph | Código 1: Morph targets and Draco-compressed primitives are not supported yet |
| reject_mixed_semantics | Código 1: One image is used with incompatible color/data/normal semantics; use separate image resources |
| reject_uv2 | Código 1: Imported glTF contains invalid cross references |
| reject_missing_uv | Código 1: Material references an absent UV channel |
| reject_embedded | Código 1: Embedded glTF images are not supported by this cooker version |
| reject_ninth_weight_set | Código 1: More than eight skin influences are unsupported |
| reject_cubic | Código 1: Only LINEAR and STEP animation are supported; bake cubic curves offline |

## Geometría cocinada v16

| Material | Vértices | LOD0 | LOD auto 1 | LOD auto 2 | Triángulos FPS | Bytes de geometría |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Hound16_Undersuit.002 | 4946 | 8358 | 4178 | 1668 | 4352 | 2280208 |
| Hound16_BurgundyCloth.002 | 2839 | 5574 | 2786 | 1113 | 0 | 999444 |
| Hound16_AshSkin.002 | 12222 | 23600 | 11800 | 4720 | 3132 | 5603376 |
| Hound16_Iron.002 | 22398 | 33014 | 16507 | 6602 | 9560 | 10105764 |
| Hound16_EdgePlanes.002 | 6258 | 6160 | 3080 | 1232 | 1064 | 2741760 |
| Hound16_Hood.002 | 1029 | 2054 | 1026 | 410 | 0 | 362928 |
| Hound16_EyeAccent.002 | 700 | 1392 | 696 | 278 | 0 | 246792 |

Total: 50.392 vértices cocinados / 80.152 triángulos LOD0; 53 huesos, máximo
dos influencias y cero imágenes. Geometría con LODs/brazos: 22.340.272 bytes.
Cálculo de buffers de 104 bytes/vértice + índices uint32, incluyendo copias
de VBO hechas por el puente; no lectura de VRAM física ni memoria total del juego.

Ocho cuerpos: 641.216 triángulos LOD0. Siete cuerpos + brazos FPS: 579.172.
Sin sumar armas, sombras ni otras pasadas. Geometría/texturas compartidas por
instancias idénticas. Escena cocinada: 7.021.345 bytes.

Visor estático: 54/56 primitivas visibles, 9 batches, LODs 35/12/7, 20 µs
de construcción CPU en la última muestra (sin interpretación de benchmark).
Grid de 4 m, cámara (7,5,-9) mirando a (0,1,0), FOV vertical 45°, 1280×720.
Alturas completas proyectadas en orden de copia: 141.19, 175.72, 232.36, 112.68, 133.68, 164.26, 93.73, 107.83 px.
Hay recorte de pantalla; no son ocho combatientes completos animados en Factory.

## Factory: objetivo 200 FPS/1080p/ocho

Equipo: Ryzen 7 3700X, GTX 1070, driver 32.0.15.8129; Release/Vulkan.
120 frames de calentamiento y 360 de muestra, dos combatientes y presentación
original. Sin VSync/resolución dinámica; HUD/audio/poll_events fuera del intervalo.
No mide Hound v16 ni combate completo de ocho jugadores.

### factory-1080p

```text
Factory benchmark: samples=360 mean=6.936 ms fps=144.17 p50=6.936 p95=6.955 p99=6.985 max=7.018 ms
CPU stages ms: update=0.145 begin=0.028 presentation=0.271 visibility=0.039 lighting=0.038 draw_present=6.415
Process memory: private=2080.32 MiB working_set=1131.23 MiB peak_working_set=1412.37 MiB
Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 GPU_last_passes=2.345 ms
```

### factory-1080p-repeat

```text
Factory benchmark: samples=360 mean=6.935 ms fps=144.19 p50=6.936 p95=6.953 p99=6.970 max=6.991 ms
CPU stages ms: update=0.147 begin=0.030 presentation=0.273 visibility=0.036 lighting=0.038 draw_present=6.412
Process memory: private=2084.89 MiB working_set=1130.54 MiB peak_working_set=1412.43 MiB
Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 GPU_last_passes=3.116 ms
```

### factory-720p

```text
Factory benchmark: samples=360 mean=1.884 ms fps=530.77 p50=1.820 p95=3.023 p99=3.049 max=3.092 ms
CPU stages ms: update=0.145 begin=0.032 presentation=0.266 visibility=0.034 lighting=0.037 draw_present=1.370
Process memory: private=2082.87 MiB working_set=1131.61 MiB peak_working_set=1413.05 MiB
Factory render: 1280x720 output=1280x720 instances=249 visible=96 batches=36 GPU_last_passes=1.194 ms
```

~6,94 ms a 1080p supera el objetivo de 5 ms. La cadencia estable puede indicar
regulación en presentación. Las consultas GPU finales no son una media completa;
no prueban por sí solas saturación ni margen. 720p es un control, no aceptación.

## Integridad y verificaciones

SHA-256 contrastados con el manifiesto inmutable de v16:

- art/characters/hound/v16/hound-mesh-v16.blend: e0692f36a7edd0d5546ce1f67b40857f9641d02f998fffae6de60d946c3a0911.
- assets/characters/hound_rig/v16/hound-rig.gltf: 754d4b93a8141e05f8fccf67332fe1df1c4ac1ed76804547852b4c5f7668c1ab.
- assets/characters/hound_rig/v16/hound-rig.bin: d0f805dd6978bd07ff93b142780b729d2ee9a916e407ffa5f22e4712fc3eb4d7.

Pruebas existentes: Release assets/gpu_assets/material_render y Debug
animation_vfx/legacy_arsenal/character_restoration, seis correctas.
La fixture de normales conserva código 1 para hacer visible el defecto.
Presupuesto de producción y prueba animada GPU pendientes; H07 no iniciada.
