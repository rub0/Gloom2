# Hito 94 — H04, ropa y ensamblaje de Hound

20 de septiembre de 2026. Únicamente H04 del índice; entrada
`art/characters/hound/v11/hound-mesh-v11.blend`, commit `de11e03`.
Workspace limpio al comenzar. [Galería, fuente y vídeo](../../docs/art/hound/mesh-v12/README.md).

## Resultado y construcción

Pantalón con pliegues amplios de tensión hacia la entrepierna y compresión
sobre rodillas. Faja hueca de pared radial de 3 mm y pliegues diagonales.
Los dos paños dejan de ser placas gruesas: pared de 3 mm en Y, raíz insertada
bajo la faja y caída separada del muslo. Sus pesos provisionales se muestrean
del pantalón; el borde libre tiene holgura para las poses comprobadas.

El torso es un chaleco continuo, suavizado y con respaldo elevado bajo las
láminas centrales. Pecho, flancos, abdomen, escápulas y cadena dorsal conservan
**todas sus caras exteriores**; solo se sustituyen los perímetros posteriores
profundos por retornos de 4 mm. Los solapes de las láminas cubren sus uniones.
Dos tiras mediales de soporte unen el chaleco con pecho/escápulas y las bases
claviculares. No se añaden remaches ni se cubren los deltoides.

La inspección de axilas demostró que las bases de clavícula y de los filos
medio/exterior entraban en ambos brazos. Se recorta su asiento contra una
envolvente convexa local del hombro en cinco grados del alcance diagnóstico.
Es una operación de autoría aplicada: no se exportan booleanos ni colliders.
Se conservan puntas y planos exteriores no afectados por ese encaje.

Ocho piezas reconstruidas: dos paños y seis asientos claviculares. Quince
ajustadas: pantalón, faja, chaleco y doce placas del torso. Dos soportes nuevos.
Los otros **73 componentes son exactos** en posiciones, caras, materiales,
sombreado y pesos, incluidas cara, ojos, boca, capucha, cuello, brazos, manos,
guardas de cadera/muslo y piezas H02. El forro del pantalón bajo Z=0,61 m
también es exacto. No cambian huesos, jerarquía, bind pose, claves ni handles.

36.466 vértices / 72.568 triángulos: +976 / +1.944 (+2,75 % de triángulos).
98 componentes, 87 rígidos, siete materiales, un mesh/skin, 53 huesos,
máximo dos influencias. Densidad de autoría, pendiente del presupuesto H06/H07.
Sin C++, dependencias nuevas, simulación ni trabajo por frame añadido al motor.

## Comprobaciones

- Fuente final reabierta. Las 25 superficies intervenidas son conexas, cerradas,
  manifold, orientadas, con volumen positivo y sin autointersecciones no
  adyacentes en reposo. Euler 0 en la faja hueca y 2 en las demás.
  Espesores de retornos/paños y extremos expuestos de filos comprobados.
- Pesos normalizados; rigidez de las piezas registrada. Los dos paños pasan a
  deformables y los dos soportes nuevos son rígidos, manteniendo 87 rígidos.
- 61 muestras del clip original: coordenadas finitas, caras no colapsadas y
  retorno al reposo. Error máximo de distancias rígidas: 0,000000682 m.
  Área mínima evaluada: 6,4264e-10 m².
- 37 poses adicionales: reposo y 25/50/75/100 % de nueve casos.
  Pantalón, faja y paños sin autointersecciones en todas ellas.
  Cada caso empieza con los huesos restaurados, sin acumular poses.
- 444 pares de componentes por pose: **16.428 evaluaciones locales** en v12.
  Se registra también v11 para distinguir problemas heredados de cambios H04.
- Roundtrip glTF en 13 poses: máximo **0,000002068570 m**.
  Conserva 43 nombres Legacy, un mesh/skin, siete primitivas/materiales y
  una acción diagnóstica. Sin cámaras ni animación temporal de revisión.
  Regresión v11 del verificador compartido: pasa con el mismo máximo.
- Cooker: escena `2138900802409148367`, cero dependencias externas.
  Visor Vulkan estático: **7/7 piezas visibles y siete batches**, salida 0.
  Captura inspeccionada; no demuestra reproducción animada en el motor.
- CTest Release `gloom.assets` / `gloom.gpu_assets`: **2/2**.
  Debug `gloom.animation_vfx`: **1/1**. Binarios existentes, sin cambios C++.
- 22 PNG y fotogramas clave del vídeo inspeccionados. Vídeo H.264 decodificado
  completo: **361 frames, 1000×1000, 30 fps, 12,033333 s**.
- Sintaxis, líneas, enlaces, diff y exclusiones del commit revisados.
  Cachés, JSON de diagnóstico, logs y capturas auxiliares no se versionan.

| Caso al 100 % | Rotaciones en ejes del personaje, convertidas a cada hueso |
| --- | --- |
| Giro de torso, ambos lados | Spine ±10° Z + Spine1 ±10° Z |
| Alcance | Ambos UpperArm −50° X; Forearm −20° X |
| Paso, ambos lados | Thigh adelantado −25° X, Calf 30°; retrasado +15°, Calf 15° |
| Flexión de cadera, ambos lados | Thigh −40° X, Calf 50° |
| Giro de cabeza, ambos lados | Head ±35° Z |

En paso/flexión se articula también el HipGuard correspondiente con el mismo
ángulo de muslo. Es una prueba explícita del hueso existente, no automatización
del rig ni promesa de que las guardas sigan solas a las piernas.

## Inventario y lectura de contactos

Se comprueban todos los pares de tres conjuntos; se eliminan duplicados:

- Cintura (11): pantalón, faja, dos paños, torso, dos guardas de cadera,
  dos placas de muslo, última placa abdominal y última placa dorsal.
- Torso (28): doce placas de pecho/flanco/abdomen/espalda, ocho piezas
  claviculares, dos soportes nuevos, torso, esternón, capucha, cuello y brazos.
- Cabeza (6): capucha, cuello, cabeza, dos ojos y boca.

| Contacto | v11, pares de caras en reposo | v12 / resultado |
| --- | ---: | --- |
| Brazo / base clavicular, por lado | 36 | 0 en las 37 poses |
| Brazo / filo medio y exterior, por lado | 19 + 29 | 0 en las 37 poses |
| Brazo / peto, por lado | 9 | 0 en las 37 poses |
| Última lámina abdominal / faja | 79 | 0 en las 37 poses |
| Capucha / clavículas y soportes | 0 con las piezas existentes | 0 en las 37 poses |
| Borde libre de cada paño / pantalón | No era un paño ajustado | 0 en las 37 poses, caras con Z de reposo <0,991 m |

El conteo de caras no mide profundidad y cambia con la teselación.
**No se exige cero en uniones construidas**: raíces de paños dentro de faja/
pantalón, articulación de guardas, láminas solapadas y respaldos bajo placas.
Los paños conservan cruces solo en la raíz cubierta; su zona libre se comprueba
por separado. Los apoyos nuevos contactan con chaleco, petos y escápulas.
Las tres láminas dorsales contactan con su respaldo en las 37 poses.

Cabeza/cuello y capucha/cuello conservan exactamente los solapes de v11:
333–339 y 120–121 pares de caras, respectivamente, en estas poses.
Las inserciones de ojos/boca, cuello/torso y brazo/torso son ensamblajes cerrados
solapados. Los renders de cuello/axila y las vistas generales no muestran
huecos ni penetraciones graves en las poses revisadas.

## Límites y traspaso

H04 termina su pase de ropa y ensamblaje como propuesta. **H05 queda pendiente
y no se inicia**; la aceptación artística global pertenece al usuario.
Se conserva la abertura frontal de capucha y la silueta de referencia.
No hay texturas/tejido microscópico, UVs, simulación de tela, rig definitivo,
animaciones de producción, nuevos agarres ni integración jugable.

El BVH detecta cruces de superficies en los pares/muestras enumerados.
No certifica contención, distancia continua, colisión global, movimientos
combinados, carrera, sentadilla profunda, torso inclinado, cuello inclinado,
armas ni agarres. Persisten los límites H01–H03 del rig provisional;
el clip heredado se comprueba estructuralmente, no como certificado global
de ausencia de penetraciones. El vídeo no resuelve apoyos de pies.

Fuente acumulada: `art/characters/hound/v12/hound-mesh-v12.blend`.
Exportación: `assets/characters/hound_rig/v12/hound-rig.gltf` y `hound-rig.bin`.
Escena `Hound_Mesh_v12`, malla `H12_DeformMesh`, rig `Hound12_Rig`,
acción `Hound12_joint_check`, colección `HOUND_v12_EXPORT`.
Evidencia: `docs/art/hound/mesh-v12/README.md`.
Commit local del hito 94; resolver con `git log --oneline --grep='^hito 94:'`.
Sin push ni comienzo de otra ficha.

## Reproducción

Blender CLI local, sin servicios externos ni nuevos puertos. El sandbox no
iniciaba procesos; se usó ejecución revisada fuera de él. MCP permanece en
modo seguro. Los generadores rechazan una fuente v12 ya existente:
reconstruir en una copia sin esa salida para conservar posibles ediciones.

Comandos ejecutados; logs y recursos cocinados en `.cache/`:

```powershell
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v11/hound-mesh-v11.blend --python-exit-code 1 --python tools/art/refine_hound_mesh_v12.py
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background .cache/hound-mesh-v12/hound-draft.blend --python-exit-code 1 --python tools/art/review_hound_mesh_v12.py -- --final
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v12/hound-mesh-v12.blend --python-exit-code 1 --python tools/art/verify_hound_mesh_v12.py --python tools/art/verify_hound_rig_roundtrip_v03.py -- v12
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v11/hound-mesh-v11.blend --python-exit-code 1 --python tools/art/verify_hound_rig_roundtrip_v03.py -- v11
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v12/hound-mesh-v12.blend --python-exit-code 1 --python tools/art/review_hound_mesh_v12.py -- --video
rtk proxy build/windows-vs/Release/gloom_asset_cooker.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-mesh-v12/cooked game:/characters/hound_rig/v12/hound-rig.gltf cache:/hound-rig.gasset
rtk proxy build/windows-vs/Release/gloom_scene_viewer.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-mesh-v12/cooked game:/characters/hound_rig/v12/hound-rig.gltf cache:/hound-rig.gasset 1 D:/Projects/Gloom/.cache/hound-mesh-v12/gloom-preview.ppm
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Release --output-on-failure -R '^gloom[.](assets|gpu_assets)$'
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Debug --output-on-failure -R '^gloom[.]animation_vfx$'
```
