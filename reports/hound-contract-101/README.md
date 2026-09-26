# Hito 101 — diagnóstico y contrato Hound H06

26 de septiembre de 2026. Pruebas y medidas del 23 de septiembre.
**H06 iniciada y bloqueada; H07 no iniciada.** Este es un subhito de diagnóstico,
no el cierre satisfactorio del presupuesto de producción.

Objetivo confirmado: **200 FPS a 1920×1080, hasta ocho combatientes, en el
Ryzen 7 3700X y GTX 1070 actuales**. Se mantienen los 5 ms por fotograma como
objetivo del juego completo. La resolución de texturas no se ha fijado.

Entrega para H07–H13: [contrato y decisiones pendientes](../../docs/art/hound/H06-contrato-presupuesto.md).
[Resultados numéricos conservados en Git](results.md).

## Hallazgos y alcance

- Catorce muestras pequeñas, creadas en Blender, exportadas a glTF/PNG,
  cocinadas y capturadas en Vulkan. Color sRGB frente a factor lineal,
  ORM, AO, emisión, UV0/UV1 y alpha tienen respuesta GPU comprobada.
- Diez de doce comparaciones de imagen pasan. Dos fallan: las normales con
  variación verde. El UASTC expandido devuelve `(R,R,R,G)`; la ruta BC5/shader
  consume RG. Las fuentes conservan el verde correcto. El defecto no es una
  decisión artística ni se resuelve retocando Hound. No se corrigió el motor.
- Fixture de dos huesos y dos pesos normalizados: constraint global horneada
  a 31 claves LINEAR, movimiento no constante y cocción aceptada. No se afirma
  reproducción animada GPU o equivalencia de cada pose.
- Siete rechazos comprobados: morph, semánticas de imagen mezcladas, UV2,
  UV ausente, imagen embebida, JOINTS/WEIGHTS_2 y CUBICSPLINE válido.
  Tangentes glTF invertidas dejan el payload cocinado idéntico: se regeneran.
- V16: 80.152 triángulos, 50.392 vértices cocinados, siete materiales,
  53 huesos, cero imágenes propias. Geometría subida calculada con LODs/brazos:
  22.340.272 bytes. Ocho copias estáticas en visor; no es un benchmark animado.
- Factory original a 1080p: 144,17 y 144,19 FPS, ~6,94 ms. A 720p: 530,77 FPS.
  La presentación de imagen puede regular la cadencia; no se ha demostrado
  que la GPU esté saturada. No atribuir estas cifras a Hound v16.

Presupuesto LOD0/LODs/materiales/mapas pendiente. La medición actual no cubre
ocho personajes animados ni HUD/audio/combate completo, y el normal map falla.
La decisión pendiente es autorizar un trabajo técnico separado: corregir
la conservación de X/Y de normales y preparar/medir ocho presentaciones
animadas a 1080p, aislando la regulación de Present. Su alcance y alternativa
sin normal map están detallados al final del contrato. No faltan ya datos del
usuario sobre FPS, resolución, equipo o cantidad de combatientes.

No se modificaron motor, gameplay, arte ni exportaciones Hound. Hashes de
fuente v16, glTF y BIN coinciden con su manifiesto. La matriz de estados,
nombres de huesos y sockets incluye las cinco armas actuales. El manifiesto
histórico de H05 conserva su flag de aprobación original; no se reescribe.

## Reproducción

Desde `D:/Projects/Gloom`, con el Blender y Python ya instalados del proyecto.
Las muestras y cocciones se generan en `.cache/hound-h06/`; nunca ejecutar
el generador sobre una escena de artista. NumPy y Pillow están en el entorno
existente; el diagnóstico usa la biblioteca KTX instalada por vcpkg.

```powershell
rtk proxy D:/Dev/CMake/bin/cmake.exe --build build/windows-vs --config Release --target gloom gloom_asset_cooker gloom_scene_viewer gloom_material_render_tests --parallel 8
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background --factory-startup --python-exit-code 1 --python tools/art/create_hound_h06_probe.py
rtk proxy .cache/legacy-tools/Scripts/python.exe tools/art/verify_hound_h06_probe.py
```

El último comando guarda capturas/resultados y **devuelve 1 por `normal` y
`normal_y` con el motor actual**. Es el fallo real que bloquea el contrato,
no un error que deba ocultarse. Continuar el diagnóstico separado:

```powershell
rtk proxy .cache/legacy-tools/Scripts/python.exe tools/art/verify_hound_h06_boundaries.py
rtk proxy build/windows-vs/Release/gloom_asset_cooker.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-h06/cooked game:/characters/hound_rig/v16/hound-rig.gltf cache:/hound-v16.gasset
rtk proxy build/windows-vs/Release/gloom_scene_viewer.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-h06/cooked game:/characters/hound_rig/v16/hound-rig.gltf cache:/hound-v16.gasset 8 D:/Projects/Gloom/.cache/hound-h06/hound-eight.ppm
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-1080p > .cache/hound-h06/factory-1080p.log
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-1080p > .cache/hound-h06/factory-1080p-repeat.log
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-720p > .cache/hound-h06/factory-720p.log
rtk proxy .cache/legacy-tools/Scripts/python.exe tools/art/measure_hound_h06.py
```

Ejecutar las pruebas GPU secuencialmente. El perfil espera recursos, calienta
120 frames y mide 360; simulación fija a 60 Hz, resolución nativa, sin VSync
ni resolución dinámica. Incluye actualización/presentación/envío/Present;
excluye arranque, poll_events, HUD/audio y capturas. Consultas GPU finales
no equivalen a media ni a tiempo completo del frame. Las mediciones del
informe son observaciones del 23/09; repetir puede producir otras cifras.

El visor de recursos usa 1280×720 y cámara fija/FOV 45°. Los tamaños proyectados
se calculan con su cámara y grid exactos. Varias copias se recortan en pantalla;
el recuento no prueba ocho rivales completamente visibles ni skinning activo.
La geometría, su proyección y la residencia geométrica se derivan del
payload real y del puente GPU, no de una cifra genérica de Blender.

## Comprobaciones y limitaciones

```powershell
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Release --output-on-failure -R '^gloom[.](assets|gpu_assets|material_render)$'
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Debug --output-on-failure -R '^gloom[.](animation_vfx|legacy_arsenal|character_restoration)$'
```

Las seis pruebas existentes pasan. La primera ejecución de `material_render`
falló porque faltaba su ejecutable Release; se compiló y pasó. Esas pruebas
no detectaban la pérdida del canal verde en la ruta Blender→cooker→GPU;
la fixture nueva sí la detecta. No se ejecutó una suite completa.
Builds de los cuatro ejecutables existentes correctos; capturas inspeccionadas.

Fuentes revisadas: importador/animación, cooker/texturas, mesh processing,
puente/residencia GPU, shader de materiales, animador de personajes,
presentación FPS/TPS, modo de rendimiento y ARSENAL. Sin cambios en C++.

Los PNG/JSON/PPM y logs son artefactos regenerables ignorados por `.gitignore`.
Este informe, `results.md`, el contrato y las herramientas sí se versionan;
no se modifica `.gitignore` ni se fuerzan binarios al commit.

Push solicitado realizado hasta `2027548` (hito 100). Al retomar el 26/09,
`main` y GitHub contienen también `db1d98e`, actualización independiente de
AGENTS; se conserva. El subhito 101 se registra en un commit local, sin otro
push ni inicio de H07. Resolver su hash con
`git log --oneline --grep='^hito 101:'`.
