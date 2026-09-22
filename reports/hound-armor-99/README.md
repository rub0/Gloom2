# Hito 99 — armadura envolvente H05/v16

22 de septiembre de 2026. El usuario pide armadura exterior delante, detrás
y en los laterales, admitiendo malla interior visible en algunas zonas.
**H05 sigue en revisión; esta entrega requiere aceptación visual.**
La respuesta «mucho mejor!» se registra como aceptación específica de la caída
de capucha v15, que aquí se conserva exacta. No aprueba toda la escultura.

## Resultado y entrada siguiente

[Fuente Blender](../../art/characters/hound/v16/hound-mesh-v16.blend) ·
[glTF](../../assets/characters/hound_rig/v16/hound-rig.gltf) ·
[BIN](../../assets/characters/hound_rig/v16/hound-rig.bin) ·
[Manifiesto SHA-256](../../art/characters/hound/v16/sculpture-reference.json) ·
[Galería y vídeo](../../docs/art/hound/mesh-v16/README.md)

Entrada: v15, commit `84a47d9`. Se parte de esa fuente acumulada.
Escena `Hound_Mesh_v16`, malla `H16_DeformMesh`, rig `Hound16_Rig`,
acción `Hound16_joint_check`, colección `HOUND_v16_EXPORT`.
V13, v14 y v15 conservan sus fuentes, exportaciones y hashes.
La siguiente revisión parte de v16. H06 no se ha iniciado; sin push ni integración jugable.

La carencia principal estaba en el torso: placas dorsales pequeñas y costados
ampliamente expuestos. Se reconstruyen **11 piezas**: dos placas escapulares,
tres lumbares y seis bandas laterales. La espalda gana cobertura ancha; las
bandas rodean ambos costados y enlazan visualmente con pecho, abdomen y espalda.
El peto y los abdominales centrales conservan las formas del feedback anterior.

Se mantienen aberturas bajo las axilas y separaciones entre placas para
articulación, junto al tejido de cintura. Los brazos superiores y hombros
siguen descubiertos como en la dirección aprobada. Antebrazos y grebas ya
tenían carcasas envolventes: se revisan en las vistas completas y se conservan.
No se extiende metal sobre todo el pantalón ni se altera la capucha.

**95 componentes ajenos exactos** en coordenadas, caras, materiales y pesos.
Los 53 huesos conservan jerarquía, bind pose, longitudes y claves/interpolación.
Un peso provisional sí cambia dentro de las piezas reconstruidas:
`Back_spine_1` pasa de `Bip001 Spine` a `Bip001 Spine1`, coincidiendo con
`Rib_lamella_2`. Corrige el cruce entre esa placa y los costados al girar,
sin cambiar huesos ni poses. No es un rig de producción.

40.274 vértices / 80.152 triángulos frente a 38.085 / 75.774 de v15:
+2.189 / +4.378 (aprox. +5,7 %).
106 componentes, 95 rígidos, siete materiales/primitivas, una malla/skin,
una acción y máximo dos influencias. Son cifras de autoría, sin presupuesto H06.

## Evidencia y pruebas

- Renders reales de frente, perfil, espalda, tres cuartos, detalles, poses y arma;
  comparativa v15/v16 a igual cámara/escala. La lámina aislada de torso retira
  temporalmente brazos, capucha y piezas circundantes para mostrar la cobertura;
  las vistas completas conservan todo el modelo.
- Siluetas frontal/lateral/posterior a 200, 100 y 50 px.
- Vídeo de 331 frames, 900×1000, 30 fps, 11,033 s, decodificado completo;
  doce fotogramas representativos inspeccionados. Acción de revisión temporal,
  no guardada en la maestra. El material sigue siendo provisional.
- Fuente final reabierta: todos los componentes conexos, cerrados, orientados y
  con volumen positivo. Pesos positivos/normalizados, conservación comprobada.
- 61 muestras diagnósticas: coordenadas finitas y áreas positivas.
  Error máximo de distancia rígida 0,000000715 m.
- 33 poses globales × 5.565 pares = **183.645 evaluaciones por versión**.
  Comparación con v15: contactos de piezas ajenas y apoyo de arma idénticos.
  Sin autointersecciones nuevas. Las once placas reconstruidas no cruzan
  brazos, cabeza, cuello, capucha ni manos en las muestras.
- Cobertura radial del torso cada 5°: a 1,11 y 1,205 m se pasa de 32/72 y
  38/72 direcciones con metal a 72/72. A 1,25 m hay metal en las cuatro
  direcciones principales. Las secciones superiores mantienen huecos de axila
  y unión central. No representa un porcentaje de superficie ni cobertura continua.
- Roundtrip glTF en 13 poses: máximo 0,000002069 m.
  Regresión del roundtrip y auditor compartidos sobre v15 correcta.
- Cooker sin dependencias externas y visor Vulkan estático correctos.
  [Captura](../../docs/art/hound/mesh-v16/gloom-preview.png).
  No certifica reproducción animada en el juego.
- CTest Release `gloom.assets` / `gloom.gpu_assets`: 2/2;
  Debug `gloom.animation_vfx`: 1/1. Sin C++ modificado ni recompilación necesaria.

## Contactos y límites

[Comparación de contactos y secciones](contacts.md).
Los recuentos BVH son parejas de caras y dependen de la teselación;
no miden profundidad, contención o holgura entre fotogramas.

Los extremos delanteros de las bandas se insertan bajo las placas abdominales:
35, 34 y 31 parejas por lado en reposo, según la unión. Las raíces de filos
posteriores y apoyos de espalda conservan inserciones constructivas.
El cruce nuevo entre placa lumbar central y bandas laterales al girar se
corrige haciendo coherente su peso provisional. Las placas lumbares vecinas
y bandas laterales vecinas quedan separadas en las 33 poses medidas.

Persiste el contacto abdomen 2/banda lateral 2 en torsión (máximo 56 parejas
por lado frente a 116 en v15), además de los contactos ajenos que permanecen
exactos: codos a 85°, cabeza/torso al bajar, láminas abdominales, inserciones
bajo guantes/armadura y apoyo Soul Reaper inválido contra brazo, guantelete,
clavícula y garras. La capucha aún se dobla con fuerza al mirar abajo.
[Limitaciones anteriores](../hound-feedback-97/README.md).
Resolver holguras, pesos y agarre con la malla/rig de producción antes de UVs.

## Reproducción

Desde la raíz, siempre mediante `rtk`. Blender:
`.cache/blender/blender-4.5.13-windows-x64/blender.exe`;
Python: `.cache/legacy-tools/Scripts/python.exe`.

1. Blender background sobre v15:
   `--python-exit-code 1 --python tools/art/refine_hound_armor_v16.py`.
   Crea `.cache/hound-armor-v16/hound-draft.blend`.
   `-- --final` guarda/exporta solo en rutas v16 libres, sin sobrescribir una entrega.
2. Sobre la fuente final: `tools/art/verify_hound_armor_v16.py`;
   `tools/art/review_hound_h05.py -- --v16 --audit`, `--stills` y `--video`.
   Roundtrip: `tools/art/verify_hound_rig_roundtrip_v03.py -- v16`.
3. Regresión sobre v15: auditor `-- --v15 --audit` y roundtrip `-- v15`.
4. Crear `.cache/hound-armor-v16/cooked`; cooker:
   `build/windows-vs/Release/gloom_asset_cooker.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-armor-v16/cooked game:/characters/hound_rig/v16/hound-rig.gltf cache:/hound-rig.gasset`.
   Visor: mismos cuatro argumentos y
   `1 D:/Projects/Gloom/.cache/hound-armor-v16/gloom-preview.ppm`.
5. Python `tools/art/board_hound_armor_v16.py`: decodifica vídeo, compone
   evidencias, verifica contactos ajenos y genera la comparación de contactos.
6. `D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Release --output-on-failure -R '^gloom[.](assets|gpu_assets)$'`;
   Debug con `-R '^gloom[.]animation_vfx$'`.

El guardado inicial quedó interrumpido por un límite de uso de la revisión
automática de permisos; no ejecutó la acción. Se retomó con autorización normal.
Fuentes intermedias, logs y cachés excluidos de Git. Estado, índice y ficha
actualizados; commit local del hito 99, resolver con
`git log --oneline --grep='^hito 99:'`. Sin push.
