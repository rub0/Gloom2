# Hito 83 — base deformable y esqueleto de prueba de Hound

10 de septiembre de 2026. El usuario solicitó push y empezar malla/esqueleto.
Push confirmado: `rub0/Gloom2/main`, desde `988437d` hasta
`d7fc1f820068a9a334a27f976e3f7a3da642fc1a` (hitos 80–82).
Este hito se cierra con commit local; no se amplía aquel push a trabajo posterior.

## Resultado y decisiones

[Ficha y medios](../../docs/art/hound/rig-v03/README.md).
Fuente `art/characters/hound/v03/hound-rig-v03.blend`; v01/v02 intactas.
Una malla, siete primitivas/materiales, 8.781 vértices de autoría y 17.130 triángulos.
Cuatro extremidades cerradas con 23 anillos de 12 vértices; pesos normalizados,
máximo dos influencias. Las 93 piezas rígidas usan un solo hueso.

Rig FK de 53 huesos: 43 nombres Legacy, ocho falanges distales y dos placas de
cadera. Se auditó Archangel importándolo en una escena aislada: sus centros de
codos/hombros no coinciden con Hound. La nueva pose base adapta ejes, longitudes
y jerarquía; no se copian los cuatro clips Legacy ni se afirma compatibilidad directa.

Clip diagnóstico de 6 s, sin root motion: reposo, flexión de codos, alcance,
paso aislado, giro de torso y dedos. No es un ciclo de caminar ni un ataque Bite.
La exportación muestrea a 30 Hz y desplaza tiempos a cero. Se corrigió un desfase
de un fotograma detectado al comparar la reimportación con la fuente.

El usuario preguntó si convenía hacer rig antes de malla final. Se acordó conservar
el esqueleto como prueba y estabilizar topología antes de pesos finos/controles/
animaciones. Queda pendiente eliminar solapes deltoides-bíceps-brazo, revisar
contactos de armadura, anatomía, manos/agarres y resto de retopología. No hay UVs
ni texturas finales, sustitución jugable, retargeting o animaciones de producción.

## Comprobaciones

- Fuente reabierta en Blender 4.5.13 LTS; bind pose de 1,80 m, caras no colapsadas,
  coordenadas finitas y pesos normalizados en 61 muestras temporales.
- Cuatro extremidades cerradas, bordes usados por dos caras, 264 quads por extremidad.
- 93 piezas rígidas: error máximo de distancia interna 0,000000643 m al posar.
- glTF: una malla, siete primitivas, un skin de 53 huesos y un clip LINEAR/STEP;
  sin texturas ni objetos de referencia exportados; buffers y 43 nombres verificados.
- Reimportación glTF: 13 poses comparadas en ambos sentidos por proximidad de
  vértices (el exportador duplica vértices en bordes/materiales); error máximo
  0,000004689 m. Comprueba geometría animada, no identidad de índices/topología.
- Cooker real: escena `10191948375362532261`, cero dependencias externas.
- Visor Diligent/Vulkan: 7/7 visibles, siete batches, cierre y salida 0. Su cámara
  genérica queda lejos y muestra reposo; NO valida reproducción GPU del nuevo clip.
- CTest Release assets/gpu_assets 2/2; Debug animation_vfx 1/1, ejecutables existentes.
  Sin cambios de motor, rebuild, suite completa ni partida manual.
- Se revisaron lámina, vistas y fotogramas decodificados del vídeo H.264
  640×800, 181 imágenes, duración de contenedor 6,033333 s.

## Reproducción

Con la fuente v02 abierta, ejecutar `build_hound_rig_v03.py` en Blender. Se niega
a sobrescribir una escena v03. `review_hound_rig_v03.py` guarda/exporta y genera
vistas; `board_hound_rig_v03.py` genera la lámina y `animate_hound_rig_review_v03.py`
el vídeo. Crear antes las carpetas de salida. Usar rutas nuevas si hay ediciones
posteriores; no ejecutar un guardado sobre una versión aceptada por un artista.

```powershell
& .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v03/hound-rig-v03.blend --python-exit-code 1 --python tools/art/verify_hound_rig_v03.py --python tools/art/verify_hound_rig_roundtrip_v03.py
& build/windows-vs/Release/gloom_asset_cooker.exe assets .cache/hound-rig-v03/cooked game:/characters/hound_rig/v03/hound-rig.gltf cache:/hound-rig.gasset
& D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Release -R '^gloom\.(assets|gpu_assets)$' --output-on-failure
& D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Debug -R '^gloom\.animation_vfx$' --output-on-failure
```

Logs y capturas de integración: `.cache/hound-rig-v03/`. Código Python y diff
revisados; no cambia C++ ni el presupuesto autoritativo de gameplay. La agrupación
reduce objetos/batches de autoría, pero no demuestra un presupuesto final de combate.
MCP local en modo seguro; sin proveedores de pago ni servicios de generación 3D.
El cierre se retomó tras una interrupción por límite de uso.
