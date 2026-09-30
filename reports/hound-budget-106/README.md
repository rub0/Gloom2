# Hito 106 — presupuesto H06 y residencia completa

30 de septiembre de 2026 · Continuación solicitada del presupuesto y lo pendiente
antes de H07. Base `3dcb36d` (preparación 105), workspace limpio al comenzar.
**H06 queda cerrada como entrada técnica de producción. H07 preparada, sin iniciar.**
Se conserva el objetivo de **200 FPS / 5 ms, 1920×1080 nativo, hasta ocho
combatientes en Ryzen 7 3700X / GTX 1070**. No hay push.

El [contrato H06](../../docs/art/hound/H06-contrato-presupuesto.md#presupuesto-vigente--entrada-h07-hito-106)
fija ≤40.000 triángulos TPS, ≤18.000 FPS, siete materiales TPS/cuatro FPS,
mapas hasta 2K y reserva de memoria. La carga elegida mide **296,77–298,76 FPS**,
p99 máximo **3,914 ms**, máximo **4,503 ms** y **0/1.440 frames >5 ms**, con
audio nulo y real. [Dieciséis pasadas y distribuciones completas](mediciones.md).
Es un presupuesto medido para producir; los recursos finales deben volver a
validarse en sus etapas. No certifica todavía una partida humana/autoritativa de ocho.

## Residencia: causa y corrección

El backend ya consultaba `TextureCompressionBC` y el coordinador sabía cargar
BC5/BC7, pero nunca solicitaba esa capacidad al crear el dispositivo.
`DeviceFeatures.TextureCompressionBC` de Diligent parte de **DISABLED**, incluso
en la GTX 1070. Por eso se cargaban los KTX como RGBA8 y se agotaban los 512 MiB.
Se solicita ahora **OPTIONAL**: el camino existente BC5/BC7 se activa cuando
el dispositivo lo permite, manteniendo RGBA8 como respaldo. No cambia el cooker,
el formato de assets ni el límite de residencia. No se rediseña el gestor LRU
ni la duplicación existente de buffers de LODs.

| Control de siete Hound v16 + FPS | RGBA8 forzado | BC activo |
| --- | ---: | ---: |
| Assets residentes | 511,73 MiB | 183,54 MiB |
| Evicciones acumuladas al terminar | 30 | 0 |
| Intentos de dibujar una malla ausente durante 360 frames | 21.276 | 0 |
| Texturas requeridas ausentes durante la medida | 0 | 0 |
| Draws / sombras en el último frame | 527 / 406 | 584 / 436 |
| Primitivas Hound dentro del frustum | 53/53 | 53/53 |

**Rectificación del 104:** sus 30 evicciones iniciales dejaban partes de Factory
sin dibujar. Cero evicciones durante la medida y 53 skins en el frustum no
demostraban que toda la escena estuviera residente. Se conservan sus datos como
históricos y su arreglo de constantes/Present; sus tiempos no certifican la
escena completa. La regresión `gloom.hound_runtime` exige ahora cero mallas y
texturas ausentes. El control RGBA8 reproduce el defecto con el código final.

Los contadores se incrementan donde el renderer omite una malla o sustituye
una textura requerida; cuentan intentos por pasada, no assets únicos.
Las trazas optativas de uploads/liberaciones permiten auditar bytes por recurso.
No hay uploads ni impresión por frame en el intervalo medido. No se retiran
los otros personajes, armas ni partes de Factory para crear margen.

## Carga diagnóstica reproducible

`tools/art/prepare_hound_budget.py` comprueba los hashes de v16 y escribe solo
en `.cache/hound-budget-106/source/h06-budget/` y el contenido cocinado de build.
La ruta del fixture es `game:/h06-budget/hound.gltf`, cocinada como
`cache:/h06-budget/hound.gasset`. El log del juego imprime esa ruta y el LOD
forzado para evitar confundirla con v16 o con una futura malla H07.

- Geometría/rig/pesos de v16; UVs planas de diagnóstico. No son UVs/bake de producción.
- Ocho conjuntos diferentes de cuatro mapas 2048² (color, normal, ORM, emisión),
  con mips completos. Siete se muestrean entre los siete materiales de todos
  los cuerpos; el octavo es reserva residente en un material sin geometría.
  **No son ocho geometrías distintas ni ocho skins asignadas una por actor.**
- Cuatro copias remuestreadas de `Hound16_joint_check`, nombradas `idle`,
  `forward`, `strafe_right`, `jump`: 159 canales ×181 claves ×4 = 115.116 claves.
  Conservan 53 huesos y máximo dos influencias. El animador de locomoción
  existente evalúa mezcla/IK; no se entregan clips jugables ni saltos validados.
- Siete TPS con las cinco armas existentes (máscara 31), sombras y animación
  (máscara 127); FPS local con Soul Reaper, disparo, HUD y presentación de audio.
  Los agarres siguen siendo diagnósticos, sin offsets definitivos por arma.
- Se fuerza cada LOD TPS por separado manteniendo los mismos buffers y brazos
  FPS completos residentes. Esto aísla coste de dibujo sin atribuir al cambio
  una reducción de memoria que aún no se ha implementado.

| Contenido cocinado | V16 de control | Fixture con mapas/clips |
| --- | ---: | ---: |
| Primitivas / materiales | 7 / 7 | 7 / 8 (uno de reserva) |
| Vértices TPS cocinados | 50.392 | 53.626 |
| Triángulos TPS LOD0 / LOD1 / LOD2 | 80.152 / 40.073 / 16.023 | 80.152 / 40.073 / 16.026 |
| Triángulos FPS | 18.108 | 18.108 |
| Bytes geométricos con copias de LODs/FPS | 22.340.272 | 23.620.444 |
| Imágenes Hound | 0 | 32 |
| Assets de escena residentes | 183,54 MiB | 355,43 MiB |

El layout GPU de 104 bytes/vértice, índices uint32 y las copias de vertex
buffers de cada LOD/FPS están incluidos en los bytes geométricos. Los splits
de UV/tangentes explican la diferencia de vértices del fixture. Contabilidad
del escenario con fixture: **38,728 MiB de mallas +316,705 MiB de texturas**.
Los 32 mapas Hound ocupan **170,668 MiB**; el resto de texturas **146,037 MiB**
incluye recursos originales/UI. Son bytes de assets del motor, **no VRAM total**:
no incluyen targets, heaps, alineación del driver ni toda su memoria interna.

## Medición y decisión

Windows, controlador 32.0.15.8129, escritorio 144 Hz, Release en ventana,
render y salida 1920×1080, IMMEDIATE, dos imágenes, VSync off, sin resolución
dinámica. Se conserva la política de Present del 104. Simulación fija de 60 Hz,
independiente de los FPS de presentación. Audio real: SDL3 **Headphones
(High Definition Audio Device)**, sin fallback nulo.

Cada proceso: carga completa, 120 frames de calentamiento y 360 medidos CPU;
359 consultas GPU del render completo, sin Present y recogidas con retraso.
CPU desde poll/events hasta terminar Present, incluidos HUD/audio/disparo.
Las capturas se hacen fuera del intervalo; el HUD al principio puede reflejar
el tiempo de carga y no es la fuente de los FPS del informe. Serie final
ejecutada después de los builds, sin tests ni otros builds simultáneos.

| Perfil, cuatro pasadas cada uno | FPS medios | Peor p99 CPU | Máximo CPU | Frames >5 ms |
| --- | ---: | ---: | ---: | ---: |
| V16 sin mapas propios | 264,86–279,57 | 4,346 ms | 9,715 ms | 1/1.440 |
| Fixture TPS 80.152 | 256,38–264,69 | 4,556 ms | 6,521 ms | 1/1.440 |
| **Fixture TPS 40.073, elegido** | **296,77–298,76** | **3,914 ms** | **4,503 ms** | **0/1.440** |
| Fixture TPS 16.026 | 326,31–327,60 | 3,604 ms | 3,915 ms | 0/1.440 |

Todas: 53/53 primitivas skinned, siete TPS animados, nueve mapeos de paleta,
cero recursos ausentes y cero evicciones tanto en carga como en medida.
Fixture: 619 draws/464 de sombras, 355,43 MiB. En el perfil elegido, poses
0,533–0,566 ms y skin/bounds 0,090–0,108 ms. La carga de clips ya cuenta dentro
del tiempo completo. Se conservan **los dos picos >5 ms** de los perfiles de
80.152 triángulos. Sus máximos GPU son menores de 5 ms; sin traza no se les
atribuye una causa de sistema ni se descartan como ruido.

Se fijan 40.000 TPS/18.000 FPS redondeando hacia abajo el perfil elegido;
54.000 vértices/24 MiB geométricos son techos de asignación sobre lo medido,
sujetos a volver a probar el candidato. Pool Hound de mapas ≤192 MiB y objetivo
de escena ≤384 MiB reservan margen frente a 512 MiB, sin afirmar haber medido
ya el techo exacto de 384. El margen de trabajo es p99 ≤4 ms y máximo ≤5 ms
en las cuatro pasadas, con integridad de contenido comprobada.

Se usa la ruta de LODs existente (50 %/20 %, error 0,01; distancia/radio 30/80),
con revisión de silueta/deformación obligatoria en H07. No se inventa un formato
para LODs glTF de autoría. Las verificaciones finales de mapas, clips, pesos,
efectos, agarres y ocho jugadores autoritativos pertenecen a H08–H13; exigir esos
assets antes de fijar un límite para H07 crearía una dependencia circular.

## Validación

- Release recompilado y **10/10 CTest**: render_scene, gpu_assets, visibility,
  animation_vfx, skin_bounds, material_render, assets, vertical_slice_smoke,
  vulkan_sync y hound_runtime. Debug recompilado y **5/5**: gpu_assets,
  skin_bounds, assets, vulkan_sync y hound_runtime, incluida validación Vulkan.
  Tras ajustar los tipos enteros de las trazas de carga, nueva compilación y
  regresión final **Release 3/3** (material_render, vulkan_sync, hound_runtime)
  y **Debug 2/2** (vulkan_sync, hound_runtime); sin cambiar trabajo por frame.
- **14 fixtures /12 comparaciones de materiales** pasan en GPU BC, incluido
  sRGB/lineal, ORM/AO, emisión, UVs, alpha y normales.
- **144 comprobaciones numéricas de canales/mips y nueve GPU de normales**
  pasan con BC: X/Y, UV reflejadas y costuras. Control adicional del visor con
  `--rgba8`: factor, normal X y normal Y; diferencia interior BC/RGBA8 0,0000,
  efectos de normales 24,0052/81,4920 niveles frente al factor (>5 exigido).
- `--hound-budget-lod=3` rechazado antes de iniciar el benchmark, exit 1.
  La herramienta de medida comprueba ruta BC, visibilidad, poses, armas,
  dispositivo de audio e integridad, y conserva también los tiempos fallidos.
- Capturas de los perfiles inspeccionadas; la geometría recuperada de Factory
  concuerda con los contadores. UVs/mapas de prueba no son apariencia aceptada.
- Revisión del cambio sin nuevas asignaciones por frame, parámetros Span por
  valor conservados, herramientas Python compilables y diff sin errores de espacios.

Fuente/glTF/BIN v16 conservan el manifiesto de la escultura aprobada:

| Archivo | SHA-256 |
| --- | --- |
| `art/characters/hound/v16/hound-mesh-v16.blend` | `e0692f36a7edd0d5546ce1f67b40857f9641d02f998fffae6de60d946c3a0911` |
| `assets/characters/hound_rig/v16/hound-rig.gltf` | `754d4b93a8141e05f8fccf67332fe1df1c4ac1ed76804547852b4c5f7668c1ab` |
| `assets/characters/hound_rig/v16/hound-rig.bin` | `d0f805dd6978bd07ff93b142780b729d2ee9a916e407ffa5f22e4712fc3eb4d7` |

## Reproducción

Desde `D:/Projects/Gloom`, con el contenido original cocinado y el entorno
Python existente (NumPy/Pillow), compilar antes de medir:

```powershell
rtk proxy D:/Dev/CMake/bin/cmake.exe --build build/windows-vs --config Release --target gloom gloom_scene_viewer gloom_asset_cooker gloom_gpu_asset_tests gloom_asset_tests gloom_material_render_tests gloom_skin_bounds_tests --parallel 8
rtk proxy build/windows-vs/Release/gloom_asset_cooker.exe D:/Projects/Gloom/assets D:/Projects/Gloom/build/windows-vs/content game:/characters/hound_rig/v16/hound-rig.gltf cache:/characters/hound_rig/v16/hound-rig.gasset
rtk proxy .cache/legacy-tools/Scripts/python.exe tools/art/prepare_hound_budget.py
rtk proxy .cache/legacy-tools/Scripts/python.exe tools/art/measure_hound_budget.py
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-hound-eight-1080p --present=immediate --rgba8
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Release -j 1 --output-on-failure -R 'gloom\.(render_scene|gpu_assets|visibility|animation_vfx|skin_bounds|material_render|assets|vertical_slice_smoke|vulkan_sync|hound_runtime)$'
rtk proxy D:/Dev/CMake/bin/cmake.exe --build build/windows-vs --config Debug --target gloom gloom_gpu_asset_tests gloom_asset_tests gloom_skin_bounds_tests --parallel 8
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Debug -j 1 --output-on-failure -R 'gloom\.(gpu_assets|skin_bounds|assets|vulkan_sync|hound_runtime)$'
rtk proxy .cache/legacy-tools/Scripts/python.exe tools/art/verify_hound_h06_probe.py
rtk proxy .cache/legacy-tools/Scripts/python.exe tools/art/verify_normal_maps.py
```

Las dos últimas herramientas reutilizan las fixtures del
[101](../hound-contract-101/README.md#reproducción) y del
[103](../normal-maps-103/README.md#reproducción), regenerables según sus recetas.
Para una pasada aislada, añadir `--hound-budget-lod=1` al benchmark de siete
Hound; para audio real, `--audio-device`. La herramienta completa hace las 16
pasadas en serie y guarda logs, capturas y `measurements.json` en
`.cache/hound-budget-106/`; no se versionan esos artefactos regenerables.
Control RGBA8: `.cache/hito106-rgba8-control.log`. Builds y CTest:
`.cache/hito106-final-{release,debug}-build.log`, `.cache/hito106-{release,debug}-tests.log`.
Última comprobación: `.cache/hito106-logging-{release,debug}-build.log` y
`.cache/hito106-final-{release,debug}-smoke.log`.

Informe, estado, roadmap, contrato e índice H06/H07 actualizados; el informe
104 incluye la rectificación de integridad. **No se crea una malla H07, ni mapas,
clips o rig de producción; v16 sigue siendo la maestra aprobada.** Commit local
del hito 106; resolver con `git log -1 --oneline --grep='^hito 106:'`.
