# Hito 93 — H03, rostro y anatomía visible de Hound

17 de septiembre de 2026. Solo H03 del índice. Entrada:
`art/characters/hound/v10/hound-mesh-v10.blend`, hito 92, commit `6c2fe7f`.
Workspace limpio al empezar. [Vistas, fuente, glTF/BIN y vídeo](../../docs/art/hound/mesh-v11/README.md).

## Resultado y decisiones

Rostro humano neutro más severo: órbitas y párpados, pómulos con depresión
inferior, puente nasal/alas, labios comprimidos y mentón más plano.
Ojos pequeños en almendra y línea de boca ajustados a la superficie,
sin las cuñas salientes anteriores. La capucha mantiene exactamente su abertura.
Relieve continuo de deltoides, bíceps, tríceps y antebrazo, sin añadir volúmenes
superpuestos, venas, ruido superficial ni cambiar las proporciones del conjunto.

Cuatro componentes reconstruidos (cabeza, dos ojos y línea de boca) y dos
brazos ajustados. Los otros **90 componentes permanecen exactos**: posiciones,
caras, materiales de cara, sombreado y pesos; incluye cuello, capucha, clavículas,
manos H01 y piernas H02. En los brazos se conservan topología y todos los pesos;
el extremo bajo Z=1,085 m queda exacto. Los 53 huesos, jerarquía, bind pose,
claves e interpolaciones/handles de la acción diagnóstica permanecen iguales.

35.490 vértices y 70.624 triángulos: +15.064 triángulos, +27,1 % sobre v10.
96 componentes, 87 rígidos, siete materiales/primitivas, un mesh/skin y máximo
dos influencias. La densidad adicional se concentra en el rostro para resolver
rasgos y proyección de ojos/labios; es **autoría, no presupuesto de producción**.
Sin cambios C++, nuevas dependencias ni trabajo por frame añadido al motor.
H06/H07 deben medir y reducir esta geometría antes de la integración.

## Validación realizada

- Fuente final reabierta: seis superficies intervenidas conexas, cerradas,
  manifold, orientadas, volumen positivo y Euler 2; sin caras degeneradas
  ni autointersecciones no adyacentes en reposo. Pesos normalizados y piezas
  rígidas vinculadas a sus huesos originales.
- 61 muestras de la acción original: coordenadas finitas, caras no colapsadas
  y retorno a reposo. Error máximo de distancias rígidas: 0,000000682 m.
- Ojos: vértices anteriores a 1,10–1,35 mm de la cara en Y; boca a 0,80 mm.
  Las tapas posteriores quedan incrustadas 1 mm deliberadamente. La triangulación
  facial queda fijada antes de proyectar los detalles, evitando discrepancias
  de diagonales al unir/exportar. No es una certificación de distancia continua.
- 25 poses locales: reposo y 25/50/75/100 % de cada caso de la tabla.
  Cuatro pares contra capucha —cabeza, ojo izquierdo, ojo derecho, boca—
  en cada pose: **100 evaluaciones sin cruces de superficies**.
  Cabeza sin autointersecciones en las 25 poses.
- Otros dos pares registrados por pose: cabeza/cuello y capucha/cuello.
  Sus superficies cerradas se solapan en el ensamblaje: 221–338 pares de caras
  cabeza/cuello y 118–126 capucha/cuello. El segundo coincide exactamente con
  v10; el primero tiene otra teselación y su recuento no mide profundidad.
  No se presenta esta unión como una superficie soldada o libre de solapes.
- Brazos: volumen firmado entre **96,825 % y 100 %** del reposo.
  Sin autointersecciones en reposo, codo a 21,25°/42,5° y alcance al 25/50/75 %.
  Las poses más exigentes mantienen pinzamientos heredados; tabla inferior.
- glTF reimportado en 13 poses: error máximo **0,000002068570 m**.
  Un mesh/skin, siete primitivas/materiales, 53 huesos y una acción diagnóstica;
  43 nombres Legacy conservados. Sin cámaras ni acción de revisión exportadas.
- Regresión v10 del verificador roundtrip compartido: pasa con el mismo máximo.
- Cooker: escena `12113470922200449552`, cero dependencias externas.
  Visor Diligent/Vulkan: **7/7 piezas visibles, siete batches**, salida 0;
  captura inspeccionada. Prueba estática, no reproducción animada en el motor.
- CTest Release `gloom.assets`/`gloom.gpu_assets`: **2/2**.
  Debug `gloom.animation_vfx`: **1/1**. Binarios existentes, sin cambios C++.
- 16 PNG de revisión y fotogramas clave del vídeo inspeccionados.
  Vídeo H.264 completo decodificado: **241 frames, 1000×1000, 30 fps, 8,033333 s**.
  Luz de estudio neutra Workbench, sin usar emisión ni bloom para mostrar los ojos.
  Vistas de combate a 432 y 288 píxeles de altura: el acento es tenue en la más
  lejana; no garantiza lectura subpíxel ni sustituye la presentación final de H10.
- Sintaxis, longitud de líneas, enlaces locales y diff comprobados.
  Cachés, JSON de diagnóstico, logs y captura del visor fuera del commit.

Rotaciones sobre los ejes del personaje, convertidas al espacio de cada hueso.
El porcentaje escala todos los ángulos del caso, sin modificar la acción fuente:

| Caso al 100 % | Huesos / ángulos |
| --- | --- |
| Giro izquierda/derecha | Head: ±35° sobre Z |
| Mirar arriba/abajo | Neck: ∓10° y Head: ∓20° sobre X |
| Codo | Forearm: −85° sobre X, ambos brazos |
| Alcance | UpperArm: −70° y Forearm: −25° sobre X, ambos brazos |

Pinzamientos de caras no adyacentes **por brazo**, medidos con igual topología
y poses en v10/v11; el verificador exige que H03 no los aumente:

| Pose | v10 | v11 |
| --- | ---: | ---: |
| Codo al 75 % (63,75°) | 25 | 24 |
| Codo al 100 % (85°) | 37 | 37 |
| Alcance al 100 % | 21 | 21 |

## Límites y traspaso

H03 completa su pase facial/anatómico como propuesta; **H05 conserva la
aceptación artística global**. H04 no iniciada. No se rediseña capucha/clavículas,
no hay rig facial, mandíbula animada, expresiones, texturas, UVs, LODs o integración.
El cuello conserva su ensamblaje anterior; H04 debe valorar su transición
dentro del conjunto y H08 resolver los pinzamientos del rig provisional.
H06/H08 deben decidir si alguna animación realmente necesita mandíbula;
Bite no implica añadir hocico, colmillos o mordida facial.

BVH detecta cruces en los pares y muestras nombrados; no certifica contención,
distancia continua ni contactos globales con armadura/armas. No se han validado
agarres nuevos ni locomoción final. Siguen vigentes los límites de H01/H02.

Siguiente fuente acumulada: `art/characters/hound/v11/hound-mesh-v11.blend`.
Exportación: `assets/characters/hound_rig/v11/hound-rig.gltf` y `hound-rig.bin`.
Escena `Hound_Mesh_v11`, malla `H11_DeformMesh`, rig `Hound11_Rig`,
acción `Hound11_joint_check`, colección `HOUND_v11_EXPORT`.
Evidencia: `docs/art/hound/mesh-v11/README.md`.
Commit local del hito 93; resolver con `git log --oneline --grep='^hito 93:'`.
Sin push ni comienzo de la siguiente ficha.

## Reproducción

Comandos ejecutados con prefijo `rtk`. Blender CLI local, sin servicios externos
ni puertos nuevos. El sandbox no pudo inicializar procesos; se usaron ejecuciones
revisadas fuera de él. Logs y recursos cocinados en `.cache/hound-mesh-v11/`.
El generador y el guardado final rechazan una fuente v11 existente: reconstruir
en una copia sin salida v11, conservando cualquier edición manual.

```powershell
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v10/hound-mesh-v10.blend --python-exit-code 1 --python tools/art/refine_hound_mesh_v11.py
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background .cache/hound-mesh-v11/hound-draft.blend --python-exit-code 1 --python tools/art/review_hound_mesh_v11.py -- --final --export-only
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v11/hound-mesh-v11.blend --python-exit-code 1 --python tools/art/review_hound_mesh_v11.py
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v11/hound-mesh-v11.blend --python-exit-code 1 --python tools/art/verify_hound_mesh_v11.py --python tools/art/verify_hound_rig_roundtrip_v03.py -- v11
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v10/hound-mesh-v10.blend --python-exit-code 1 --python tools/art/verify_hound_rig_roundtrip_v03.py -- v10
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v11/hound-mesh-v11.blend --python-exit-code 1 --python tools/art/animate_hound_anatomy_v11.py
rtk proxy powershell -NoProfile -Command 'New-Item -ItemType Directory -Force .cache/hound-mesh-v11/cooked | Out-Null'
rtk proxy build/windows-vs/Release/gloom_asset_cooker.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-mesh-v11/cooked game:/characters/hound_rig/v11/hound-rig.gltf cache:/hound-rig.gasset
rtk proxy build/windows-vs/Release/gloom_scene_viewer.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-mesh-v11/cooked game:/characters/hound_rig/v11/hound-rig.gltf cache:/hound-rig.gasset 1 D:/Projects/Gloom/.cache/hound-mesh-v11/gloom-preview.ppm
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Release --output-on-failure -R '^gloom[.](assets|gpu_assets)$'
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Debug --output-on-failure -R '^gloom[.]animation_vfx$'
```
