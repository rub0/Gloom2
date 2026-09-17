# Hito 92 — H02, grebas, rodillas, tobillos y botas de Hound

17 de septiembre de 2026. Solo H02 del índice. Entrada:
`art/characters/hound/v09/hound-mesh-v09.blend`, hito 91, commit `8bc8060`.
Workspace limpio al empezar; durante la reanudación solo estaban los cambios de H02.
[Vistas, fuente, exportación y vídeo](../../docs/art/hound/mesh-v10/README.md).

## Resultado y decisiones

- Dos grebas huecas: pared radial nominal de 6 mm, abertura de tobillo,
  borde posterior rebajado y lengüeta frontal continua que soporta la rodillera.
- Diez placas reconstruidas como superficies cerradas de 4 mm en el eje Y:
  rodillas, frente de greba, espinillas, tobillos y punteras. Curvatura ajustada
  al soporte, bordes definidos y extremos inferiores aliviados. El espesor
  medido sobre la normal varía con la curvatura.
- Dos botas conexas con suela biselada, elevación de punta de 3 mm,
  talón y cavidad interior del tobillo. Plantas apoyadas en Z=0 en reposo.
  Se mantiene la anchura máxima de bota de 0,190 m.
- Ajuste del forro/pantalón exclusivamente por debajo de Z=0,61 m,
  conservando su topología, materiales y pesos. La zona superior queda exacta.
- 14 componentes reconstruidos y uno ajustado; los otros 81 exactos,
  incluidas manos/guanteletes H01, brazos, torso, rostro y capucha.
  Mismos 53 huesos, jerarquía, bind pose y claves diagnósticas, sin cambiar pesos.
- 27.958 vértices y 55.560 triángulos: +8.880 triángulos, +19,0 % sobre v09.
  96 componentes, 87 rígidos, un mesh/skin, siete materiales/primitivas,
  máximo dos influencias. Densidad de autoría, no presupuesto de producción.
  Sin cambios C++ ni asignaciones nuevas por frame; H06/H07
  deben medir/reducir la geometría antes de integrarla.

Las carcasas y placas están separadas por holguras pequeñas y el soporte de
rodilla forma parte de la greba. No se añaden adornos ni microdesgaste.
La suela sigue rígida en estas pruebas: no hay flexión de dedos, IK de apoyo
o animación de caminar de producción. No cambia física, cápsula o cámara.

## Validación realizada

- Fuente final reabierta. Las 15 superficies intervenidas son conexas,
  cerradas, manifold, orientadas, de volumen positivo y sin caras degeneradas.
  Euler 0 en grebas huecas, 2 en botas, placas y pantalón.
  Sin cruces entre caras no adyacentes de esas superficies en reposo.
- 81 componentes conservados exactamente: posiciones, caras, materiales,
  sombreado y pesos. Zona superior del pantalón exacta.
  Pesos normalizados; las 87 piezas rígidas siguen ligadas a un solo hueso.
  Matrices, jerarquía y claves/handles del rig iguales a v09.
- 61 muestras del clip original: coordenadas finitas, caras no colapsadas
  y retorno a reposo. Error máximo de distancias rígidas: 0,000000682 m.
- 17 poses de apoyo: reposo y cuatro intensidades (25/50/75/100 %) de cada
  caso de la tabla inferior. 105 pares por pose, 1.785 evaluaciones sin cruces.
  Por lado se comparan las siete piezas de pierna/calzado entre sí y con
  todo el pantalón (28 pares); además, 49 pares entre lados opuestos.
  Sin autointersecciones no adyacentes del pantalón en estas 17 poses.
- Plantas sin inversión; ninguna bota atraviesa el plano Z=0 en esas poses,
  tolerancia 0,000001 m. El pie levantado del paso no se fuerza contra el suelo.
  La raíz se desplaza verticalmente solo en la revisión para presentar cada apoyo.
- glTF reimportado en 13 poses: error máximo 0,000002068570 m,
  un mesh/skin, siete primitivas/materiales, 53 huesos, 43 nombres Legacy
  y una acción diagnóstica. Sin cámaras, suelo ni acción de vídeo exportados.
- Regresión v09 del verificador compartido: pasa, máximo 0,000002036017 m.
- Cooker: escena `13165526701939461301`, cero dependencias externas.
  Visor Diligent/Vulkan: 7/7 piezas visibles, siete batches, salida 0.
  Captura inspeccionada; es una prueba estática, no animación en el motor.
- CTest Release `gloom.assets`/`gloom.gpu_assets`: 2/2.
  Debug `gloom.animation_vfx`: 1/1. Binarios existentes, sin cambios C++.
- 14 PNG finales inspeccionados. Vídeo H.264 completo decodificado:
  181 frames, 900×1100, 30 fps, 6,033333 s.
  Fotogramas de reposo, flexión, paso, punta y talón revisados.
- Sintaxis de scripts y límite de 160 caracteres comprobados.
  Enlaces locales, diff y exclusión de cachés comprobados antes del commit.

Ángulos respecto al reposo, rotaciones de revisión sobre X expresadas en el
espacio local correspondiente. Las claves originales no se sustituyen:

| Caso al 100 % | Muslo | Pantorrilla / rodilla | Pie / tobillo | Aplicación |
| --- | ---: | ---: | ---: | --- |
| Flexión | −35° | +70° | −35° | Ambas piernas |
| Paso | −38° | +62° | −24° | Izquierda; derecha en reposo |
| Punta | +5° | +12° | +8° | Ambas piernas; pie total +25° |
| Talón | −5° | +8° | −23° | Ambas piernas; pie total −20° |

## Límites que hereda la siguiente tarea

La flexión certificada en estas pruebas llega a 70°, sin torsión lateral.
Un ensayo preliminar a 85° mostró pinzamientos con el forro/rig provisional;
no se certifica esa flexión en la entrega. Arrodillarse, agacharse profundamente,
correr, torsión lateral y dedos flexionados necesitan el rig y los clips de H08/H11.

BVH comprueba cruces de superficies de los pares nombrados, no contención
volumétrica ni separación continua entre todas las muestras. No se evalúan
contactos con armas, manos, paños, placas de muslo/cadera o el resto del cuerpo.
El paso sigue siendo diagnóstico y puede deslizar: no hay locomoción final.
Sin UVs/texturas finales, presupuesto medido, integración jugable o nueva
aprobación artística global. El apoyo de Soul Reaper de H01 mantiene su límite;
no se convierte en empuñadura final.

## Reproducción

Comandos ejecutados con prefijo `rtk`. Logs, vídeo descompuesto, captura del visor
y recursos cocinados en `.cache/hound-mesh-v10/`, sin versionar.
El generador y el guardado final rechazan una fuente v10 existente.
Para reconstruir, usar una copia sin salida v10 y conservar cualquier edición manual.

```powershell
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v09/hound-mesh-v09.blend --python-exit-code 1 --python tools/art/refine_hound_mesh_v10.py
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background .cache/hound-mesh-v10/hound-draft.blend --python-exit-code 1 --python tools/art/review_hound_mesh_v10.py -- --final
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v10/hound-mesh-v10.blend --python-exit-code 1 --python tools/art/verify_hound_mesh_v10.py --python tools/art/verify_hound_rig_roundtrip_v03.py -- v10
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v10/hound-mesh-v10.blend --python-exit-code 1 --python tools/art/animate_hound_legs_v10.py
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v09/hound-mesh-v09.blend --python-exit-code 1 --python tools/art/verify_hound_rig_roundtrip_v03.py -- v09
rtk proxy powershell -NoProfile -Command 'New-Item -ItemType Directory -Force .cache/hound-mesh-v10/cooked | Out-Null'
rtk proxy build/windows-vs/Release/gloom_asset_cooker.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-mesh-v10/cooked game:/characters/hound_rig/v10/hound-rig.gltf cache:/hound-rig.gasset
rtk proxy build/windows-vs/Release/gloom_scene_viewer.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-mesh-v10/cooked game:/characters/hound_rig/v10/hound-rig.gltf cache:/hound-rig.gasset 1 D:/Projects/Gloom/.cache/hound-mesh-v10/gloom-preview.ppm
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Release --output-on-failure -R '^gloom[.](assets|gpu_assets)$'
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Debug --output-on-failure -R '^gloom[.]animation_vfx$'
```

Blender CLI local, sin servicios externos, puertos nuevos ni cambios de modo seguro.
El primer intento de cooker no tenía creado su directorio de caché; se corrigió
el destino y la ejecución posterior pasó. El sandbox no podía iniciar procesos,
por lo que las ejecuciones se revisaron fuera de él. La revisión automática se
interrumpió por límite de uso y se retomó tras el encargo «continua por donde te quedaste».
No se omitió la validación pendiente.

## Traspaso

Fuente acumulada: `art/characters/hound/v10/hound-mesh-v10.blend`.
Exportación: `assets/characters/hound_rig/v10/hound-rig.gltf` y `hound-rig.bin`.
Escena `Hound_Mesh_v10`, malla `H10_DeformMesh`, rig `Hound10_Rig`,
acción `Hound10_joint_check`, colección `HOUND_v10_EXPORT`.
Evidencia: `docs/art/hound/mesh-v10/README.md`.
Los JSON detallados se regeneran con el verificador y están excluidos por `.gitignore`.

H02 hecha dentro de la cobertura y límites descritos. H03 no iniciada.
H05 conserva la puerta de aprobación global. Commit local del hito 92;
resolver con `git log --oneline --grep='^hito 92:'`. Sin push.
