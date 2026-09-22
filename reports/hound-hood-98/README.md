# Hito 98 — caída abierta de capucha, Hound H05/v15

22 de septiembre de 2026. Se aplica el nuevo feedback del usuario:
«no me gusta como cierra sobre el pecho, prefiero que caiga como en el concept».
**H05 sigue en revisión, pendiente de aceptación visual explícita de v15.**
H06 no se inicia. Sin push ni cambio de presentación jugable.

## Resultado y entrada siguiente

[Fuente Blender](../../art/characters/hound/v15/hound-mesh-v15.blend) ·
[glTF](../../assets/characters/hound_rig/v15/hound-rig.gltf) ·
[BIN](../../assets/characters/hound_rig/v15/hound-rig.bin) ·
[Manifiesto SHA-256](../../art/characters/hound/v15/sculpture-reference.json) ·
[Galería y vídeo](../../docs/art/hound/mesh-v15/README.md)

Entrada: v14, commit `4c1f86c`, con F01–F06 ya aplicados.
Escena `Hound_Mesh_v15`, malla `H15_DeformMesh`, rig `Hound15_Rig`,
acción `Hound15_joint_check`, colección `HOUND_v15_EXPORT`.
V13 y v14 conservan sus fuentes/exportaciones y hashes exactos.
La siguiente revisión parte de v15; no ejecutar H06 ni otra ficha sin encargo.

Se elimina el puente inferior en W que cerraba la capucha sobre el esternón.
Dos bordes descienden a los lados del cuello y terminan sobre el peto.
El recorte queda limitado al frente: corona, marco superior del rostro y
cobertura posterior de la nuca se conservan. El dobladillo une exterior y
forro con grosor cerrado; la nueva abertura no une ambos extremos por delante.
Se conserva el material provisional de tela, sin texturizado ni simulación.

Únicamente cambia `Hood_continuous`. Las otras **105 piezas** conservan
coordenadas, caras, materiales, suavizado y pesos. Los 53 huesos mantienen
jerarquía, bind pose, longitudes y claves/interpolación/handles diagnósticos.
Los pesos de los vértices de capucha conservados también proceden de v14.

38.085 vértices / 75.774 triángulos: −77 / −154 frente a v14 (aprox. −0,20 %).
106 componentes, 95 rígidos, siete materiales/primitivas, una malla/skin y una acción.
Capucha: 1.029 vértices / 1.074 polígonos. Son cifras de autoría, sin presupuesto H06.

## Evidencia y validación

- 16 PNG: concept recortado identificado como pintura, v14/v15 con igual cámara
  y escala, frente/perfil/espalda/tres cuartos, conjunto y poses de cuello/torso.
  La comparativa aísla cabeza, cuello, capucha y peto para despejar la vista;
  las vistas completas mantienen las otras piezas. No hay retoque de formas en 2D.
- Vídeo real: 211 frames, 900×1000, 30 fps, 7,033 s. Decodificado completo;
  ocho fotogramas de reposo, giros de cabeza, inclinación, alcance y torsión inspeccionados.
  La acción temporal del vídeo no se guarda en la fuente maestra.
- Fuente final reabierta. Capucha conexa, cerrada, orientada, Euler 2,
  volumen positivo 0,001944086 m³. Resto de geometría y rig exactos frente a v14.
- 61 muestras diagnósticas: coordenadas finitas, área mínima de cara
  6,426417×10⁻¹⁰ m², pesos positivos/normalizados, máximo dos influencias.
- 33 poses independientes: capucha contra las otras 105 piezas
  (**3.465 pares por versión, 6.930 comparaciones v14/v15**).
  Cero autointersecciones de capucha. No aparece contacto con una pieza que
  estuviera libre en v14 en la misma pose. Nuca posterior comprobada exacta.
- Roundtrip glTF, 13 poses: error máximo 0,000002069 m.
  La regresión v14 del verificador compartido también pasa.
- Cooker: cero dependencias externas. Visor Vulkan estático carga/renderiza v15.
  [Captura](../../docs/art/hound/mesh-v15/gloom-preview.png).
- CTest Release `gloom.assets` / `gloom.gpu_assets`: 2/2;
  Debug `gloom.animation_vfx`: 1/1. Sin C++ modificado ni recompilación necesaria.

## Contactos y límites

[Comparación por pieza y pose](contacts.md). Se cuentan cruces de superficies;
no profundidad, contención o holgura continua. La teselación afecta al recuento.

En reposo, capucha/esternón pasa de 74 parejas de caras a cero.
Persisten inserciones del forro en cuello (78), torso (122) y peto (68 por lado),
ya presentes en v14; son ensamblajes provisionales, no superficies de producción.
No hay contacto de capucha con rostro, ojos, boca, clavículas o filos posteriores
en las 33 poses. Los bordes libres no se cruzan entre sí.
El rig provisional dobla con fuerza la tela al bajar la cabeza; no se presenta
como una caída de tela animada definitiva.

Las otras piezas son exactas: siguen vigentes los límites de v14:
pinzamiento de codos a 85°, cabeza/torso al mirar abajo, inserciones bajo
armadura/guantes y cruces de láminas abdominales al girar. El apoyo Soul Reaper
heredado continúa inválido contra brazo/guantelete/clavícula y las garras.
Este pase de capucha no redefine agarre, pesos ni sockets.
[Inventario global v14](../hound-feedback-97/contacts.md) y
[evolución/arma](../hound-feedback-97/contact-changes.md).
Resolver con malla/rig de producción antes de UVs; no se inicia ese trabajo aquí.

## Reproducción

Desde la raíz mediante `rtk`. Blender:
`.cache/blender/blender-4.5.13-windows-x64/blender.exe`;
Python: `.cache/legacy-tools/Scripts/python.exe`.

1. Blender background sobre v14, `--python-exit-code 1 --python tools/art/refine_hound_hood_v15.py`:
   genera `.cache/hound-hood-v15/hound-draft.blend`. `-- --final` guarda/exporta
   únicamente en rutas v15 libres; no sobrescribe una candidata publicada.
2. Fuente final: `tools/art/verify_hound_hood_v15.py`,
   `tools/art/verify_hound_rig_roundtrip_v03.py -- v15`;
   regresión del segundo sobre v14 con `-- v14`.
3. Fuente final: `tools/art/review_hound_hood_v15.py` para imágenes y
   `-- --video` para movimiento. Python `tools/art/board_hound_hood_v15.py`
   compone láminas, comprueba el vídeo y escribe contactos.
4. Crear primero la carpeta `.cache/hound-hood-v15/cooked`.
   `build/windows-vs/Release/gloom_asset_cooker.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-hood-v15/cooked game:/characters/hound_rig/v15/hound-rig.gltf cache:/hound-rig.gasset`.
   Visor: mismos cuatro argumentos y
   `1 D:/Projects/Gloom/.cache/hound-hood-v15/gloom-preview.ppm`.
5. `D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Release --output-on-failure -R '^gloom[.](assets|gpu_assets)$'`;
   Debug con `-R '^gloom[.]animation_vfx$'`.

El primer intento del cooker falló por no existir su carpeta de montaje;
creada la carpeta, cooker y visor pasan. La revisión visual detectó una apertura
posterior no deseada en un borrador: se conservó la nuca y se repitieron
validación, exportación, vídeo y visor sobre la geometría corregida.
Los borradores y logs quedan en caché; solo se entrega la versión corregida.

Estado, índice, ficha y galería actualizados. Commit local del hito 98:
resolver con `git log --oneline --grep='^hito 98:'`. Sin push.
