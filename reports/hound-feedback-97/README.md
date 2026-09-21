# Hito 97 — feedback artístico H05 aplicado en Hound v14

21 de septiembre de 2026. F01–F06 modelados según la especificación del hito 96.
**Candidata entregada para aceptación visual explícita; H05 sigue en revisión.**
No se ejecuta H06, no se hace push ni se cambia la presentación jugable.

## Resultado y entrada siguiente

[Fuente Blender](../../art/characters/hound/v14/hound-mesh-v14.blend) ·
[glTF](../../assets/characters/hound_rig/v14/hound-rig.gltf) ·
[BIN](../../assets/characters/hound_rig/v14/hound-rig.bin) ·
[Manifiesto SHA-256](../../art/characters/hound/v14/sculpture-reference.json) ·
[Galería real](../../docs/art/hound/mesh-v14/README.md)

Escena `Hound_Mesh_v14`, malla `H14_DeformMesh`, rig `Hound14_Rig`,
acción `Hound14_joint_check`, colección `HOUND_v14_EXPORT`.
Entrada: commit `ece90a7`, fuente v13 con nombres internos v12.
V13 conserva exactamente los tres archivos y hashes del hito 95; las fuentes anteriores permanecen intactas.
Siguiente acción: aceptación o feedback sobre v14. H05 no está hecha ni habilita H07.
No ejecutar otra ficha sin encargo.

## Cambios de forma

| Feedback | Resultado |
| --- | --- |
| F01 | Tres filos escalonados por antebrazo. Raíz distal elevada para liberar extensión de muñeca. |
| F02 | Diez terminaciones acorazadas curvas y puntiagudas. Cinco dedos, pulgar, guante y placa dorsal completa conservados. |
| F03 | Greba prolongada, empeine/espinilla continuos y rodillera algo más alta. Placas articuladas, sin soldar pie y pantorrilla. |
| F04 | Un filo ascendente independiente por lado, dos en total, arraigados en espalda. |
| F05 | Asiento clavicular más bajo y medial, filo central desde clavícula y peto achatado. |
| F06 | Láminas abdominales curvas/desiguales, entrantes y solapes hacia flancos. Extremo superior liberado del brazo. |

32 componentes reconstruidos, cuatro ajustados, ocho añadidos y **62 ajenos exactos**
en coordenadas, caras, asignaciones de material y pesos. Incluye rostro, capucha,
brazos, guantes, paños y pantalón. Los 53 huesos conservan nombres, jerarquía,
bind pose, longitudes, claves e interpolación/handles diagnósticos.

38.162 vértices frente a 36.466 (+1.696; +4,65 %);
75.928 triángulos frente a 72.568 (+3.360; +4,63 %).
106 componentes frente a 98; 95 rígidos frente a 87.
Siete materiales/primitivas, una malla/skin, una acción y máximo dos influencias.
Son cifras de autoría, sin presupuesto H06, retopología, LODs ni UVs finales.

## Evidencia y pruebas

37 PNG: concept/v13/v14, cuatro vistas de ambas mallas con idéntica cámara,
escala y luz, seis detalles F01–F06, garras dorsal/palmar/lateral, poses,
siluetas a 200/100/50 px y arma. El concept está identificado como pintura.
No se retoca la geometría en 2D. Vídeo real: 331 frames, 900×1000, 30 fps,
11,033 s; decodificado completo y 13 fotogramas seleccionados inspeccionados.

- Fuente final reabierta: 106 componentes conexos, cerrados, con orientación consistente
  y volumen positivo. Pesos positivos/normalizados y conservación exacta verificada.
- 61 muestras diagnósticas: áreas positivas, valores finitos y rigidez.
  Error máximo de distancia: 0,000000715 m.
- Auditoría global: 33 poses independientes × 5.565 pares = **183.645 evaluaciones**.
  Reposo, alcance, giros, pasos, rodillas, dedos, muñecas, cuello, codos y combinación.
- Sin autointersecciones nuevas. Solo 37 parejas de caras por brazo en codo a 85°,
  iguales al límite heredado.
- Cero cruces brazo/asiento clavicular, brazo/filo central, brazo/flanco,
  filo posterior/capucha y tercer pincho/placa dorsal en las muestras.
  Se corrigieron contactos nuevos de muñeca y alcance antes de entregar.
- Roundtrip glTF: 13 poses, error máximo **0,000002069 m**. 43 nombres Legacy
  incluidos en los 53 huesos; sin imágenes ni texturas externas.
- Regresión: auditoría v13 (98 componentes/156.849 evaluaciones), roundtrip v12
  sobre glTF v13 y compositor v13 pasan. Regenerar las láminas v13 no produjo
  cambios en Git; se preservó el informe histórico.
- Cooker y visor Vulkan estático: carga y render correctos, sin dependencias
  externas. [Captura](../../docs/art/hound/mesh-v14/gloom-preview.png).
  No prueba reproducción animada en el juego.
- CTest Release `gloom.assets` y `gloom.gpu_assets`: 2/2;
  Debug `gloom.animation_vfx`: 1/1. No se modifica C++ ni se requiere recompilar.

## Contactos y límites pendientes

[Inventario completo](contacts.md) · [Evolución frente a v13](contact-changes.md).
Cruces de superficies BVH: no miden profundidad, contención ni holgura continua.
Los recuentos dependen de la teselación; no son una puntuación de calidad.

Retornos de peto, abdomen/flancos, greba/empeine/rodillera y soportes se solapan
como construcción de autoría. Hay inserciones ocultas de garras en guantes,
raíces de filos y capas de pierna. La raíz distal alcanza el antebrazo bajo la
carcasa (10 parejas por lado); no se presenta como superficie de producción libre de cruces.

Al girar el tronco, las láminas rígidas abdominales se cruzan: placas 1/2 hasta
50 parejas, placa 2/lámina lateral 2 hasta 116 por lado. Las holguras/pesos
provisionales requieren resolución con malla/rig de producción antes de UVs.
El abdomen sigue siendo metal; no se cambia el esqueleto para ocultar contactos.

Persisten codo a 85° y cabeza/torso (114 parejas al mirar abajo con Neck 10° +
Head 20°). Capucha/rostro no se desplazan para compensar el peto.

Soul Reaper original, escala 0,43 y apoyo diagnóstico heredado: brazo
619 parejas en apoyo/616 en alcance, frontal de guantelete 91, carcasa 255,
brazalete de codo 136 y filo clavicular exterior 11 en apoyo.
Las nuevas garras añaden contacto con la carcasa: Finger_R_0C 14, Finger_R_1C 48,
Finger_R_2C 14 y Thumb_RB 32 en ambas poses.
**El apoyo es inválido global y localmente; no es agarre ni socket aprobado.**
Arma intacta. H08 deberá resolver posición, dedos/pulgar, mano izquierda y socket.

## Reproducción

Desde la raíz, siempre mediante `rtk`.
Blender: `.cache/blender/blender-4.5.13-windows-x64/blender.exe`.
Python: `.cache/legacy-tools/Scripts/python.exe`.

1. Blender background sobre v13, `--python-exit-code 1 --python tools/art/refine_hound_mesh_v14.py`:
   genera solo `.cache/hound-mesh-v14/hound-draft.blend`.
2. Auditar borrador con `tools/art/review_hound_h05.py -- --v14 --audit`.
   `tools/art/save_hound_mesh_v14.py` guarda/exporta en rutas v14 libres;
   rechaza sobrescribir una candidata publicada.
3. Sobre fuente final: `tools/art/verify_hound_mesh_v14.py`,
   `tools/art/verify_hound_rig_roundtrip_v03.py -- v14`,
   `tools/art/review_hound_h05.py -- --v14 --audit`, `--stills` y `--video`.
4. Python: `tools/art/board_hound_h05.py --v14` y `tools/art/board_hound_feedback_v14.py`.
5. Regresión: auditor sin `--v14` sobre v13; roundtrip
   `-- v12 --gltf assets/characters/hound_rig/v13/hound-rig.gltf`; compositor sin `--v14`.
6. `build/windows-vs/Release/gloom_asset_cooker.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-mesh-v14/cooked game:/characters/hound_rig/v14/hound-rig.gltf cache:/hound-rig.gasset`.
   Visor: mismos cuatro argumentos y `1 D:/Projects/Gloom/.cache/hound-mesh-v14/gloom-preview.ppm`.
7. `D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Release --output-on-failure -R '^gloom[.](assets|gpu_assets)$'`;
   Debug con `-R '^gloom[.]animation_vfx$'`.

Scripts fuente versionados; cachés/logs/builds/copias intermedias excluidos.
Estado, índice, ficha y galerías actualizados. Commit local del hito 97:
resolver con `git log --oneline --grep='^hito 97:'`. Sin push.
