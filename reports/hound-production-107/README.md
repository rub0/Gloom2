# Hito 107 — H07, malla de producción v17

2 de octubre de 2026. Entrada `ffc2d99` (H06/106), workspace inicialmente limpio.
**H07 terminada en v17; H08 y las demás fichas no se han iniciado. Sin push.**
Se conserva la escultura aprobada v16 byte a byte. No se integra el candidato
como personaje del juego ni se cambia gameplay, rig definitivo, mapas o clips finales.

[Fuente](../../art/characters/hound/v17/hound-production-v17.blend) ·
[glTF](../../assets/characters/hound_rig/v17/hound-rig.gltf) ·
[BIN](../../assets/characters/hound_rig/v17/hound-rig.bin) ·
[Manifiesto](../../art/characters/hound/v17/production-reference.json) ·
[Galería](../../docs/art/hound/mesh-v17/README.md) ·
[Inventario por pieza/material/LOD](inventario.md) · [Todas las medidas](mediciones.md).

Escena `Hound_Mesh_v17`, malla `H17_DeformMesh`, rig `Hound17_Rig`,
acción `Hound17_joint_check`, colección `HOUND_v17_EXPORT`.
La referencia para hornear sigue siendo `art/characters/hound/v16/hound-mesh-v16.blend`.

## Resultado y presupuesto

| Recurso | V17 | Límite H06 |
| --- | ---: | ---: |
| Vértices de autoría | 19.950 | — |
| Triángulos TPS LOD0 | 39.504 (−50,71 % frente a v16) | 40.000 |
| Triángulos LOD1 / LOD2 | 19.747 / 8.247 | Objetivos 50 % /20 %, error 0,01 |
| Triángulos FPS | 13.834 | 18.000 |
| Vértices TPS cocinados | 29.745; 29.757 con UVs diagnósticas del benchmark | 54.000 |
| Primitivas/materiales TPS /FPS | 7 /4 | 7 /4 |
| Huesos /influencias máximas | 53 /2 | 53 /2 |
| Geometría con copias de LOD/FPS | 12,346 MiB; 12,351 MiB en el fixture | 24 MiB |
| Mapas diagnósticos Hound | 32 mapas 2K, ocho conjuntos, 170,668 MiB | Pool 192 MiB |
| Assets residentes, carga diagnóstica completa | 345,26 MiB | Objetivo 384 MiB; techo 512 MiB |

Una malla/skin TPS, siete materiales OPAQUE y ninguna imagen final. Los brazos
se extraen con la regla vigente de pesos >95 %, sin LOD propio. No se exportan
objetos superpuestos como si fueran LODs consumidos por el motor.
La prueba de mapas usa siete conjuntos repartidos entre materiales y el octavo
como reserva, igual que el 106; no representa ocho geometrías ni ocho skins
asignadas individualmente. La UV0 de la fuente también es provisional.

## Geometría y conservación

Se mantienen los 106 componentes y sus separaciones; 95 conservan asignación
rígida a un solo hueso. No se retira ninguna superficie supuestamente oculta.
Diez superficies deformables quedan exactas: brazos, guantes, cuello, capucha,
torso, pantalón y paños. La faja pasa de 48 a 24 columnas angulares y conserva
sus trece anillos de altura, pared interior/exterior y pesos de los vértices retenidos.
Así se deja densidad para placas finas sin alterar los loops de las articulaciones.

Las piezas rígidas usan la biblioteca meshoptimizer ya instalada para el cooker,
por componente y con splits de material/normal. Retiene posiciones y normales
de referencia. La tolerancia se reduce en grebas y placas lumbares para impedir
que las caras de retorno se crucen; dedos/ojos admiten más error relativo por su
tamaño pequeño. Las ratios solicitadas son objetivos, no recuentos garantizados.
Se limpian degenerados heredados al triangular filos y placas de las manos.
Las alternativas de colapso de Blender y disolución angular se descartaron por
sombreado ondulado o cruces en placas finas; no están en la entrega.

Frente, perfil, espalda, tres cuartos, rostro, garras, torso, capucha y piernas
comparados a igual cámara/escala e inspeccionados. IoU de silueta rasterizada
v16/LOD0: **99,875 % frontal, 99,907 % lateral y 99,876 % posterior**.
Error superficial bidireccional muestreado máximo: **2,917 mm**; faja **1,707 mm**.
Se muestrean vértices y centros de triángulos, no una cota continua analítica.
El detalle que ya no necesita geometría permanece en la maestra para el futuro H09.

## Validación de fuente, deformación y transporte

- Fuente final reabierta; escala unitaria, normales finitas/unitarias, UV0 finita,
  pesos finitos normalizados y máximo dos influencias. Rig, bind pose, jerarquía
  y claves/interpolaciones exactos respecto a v16.
- **181 frames del clip** completos; área mínima de caras 6,421×10⁻¹⁰ m²,
  error máximo de distancia rígida 0,000000715 m. Los 106 componentes TPS son
  conexos, cerrados, orientados y de volumen positivo.
- Auditoría **33 poses ×5.565 pares =183.645 evaluaciones**: reposo y mitades/extremos
  de alcance, torsión, pasos, cadera, cabeza, puño, muñeca, mirada, codo y pose combinada.
  Mismos conjuntos de contactos entre piezas y autointersecciones que v16.
  No se comparan como equivalentes los recuentos de caras tras cambiar teselación.
- glTF 2.0 separado y BIN reimportados en 13 poses: error máximo **0,000002067 m**.
  Regresión v16 de fuente/auditoría/roundtrip pasa; 44 fuentes/exportaciones
  anteriores coinciden con sus blobs Git, además de los manifiestos v13–v16.
- Cooker y visor Vulkan cargan **la ruta v17**. LOD0/1/2/FPS: cero triángulos
  degenerados, normales válidas y presupuesto de vértices/materiales/memoria cumplido.
- Los índices reales del cooker se reconstruyen en glTFs diagnósticos en caché
  y se reimportan, conservando skin/clip para comparar los tres LODs y FPS.
  61 muestras del clip y 30 poses extremas adicionales; todas las 106 piezas
  sobreviven en LOD0/1/2. Las superficies LOD1/2 se comparan en ambos sentidos.
- Vídeo de revisión de **331 frames /30 FPS**, decodificado completo, y doce
  fotogramas representativos inspeccionados. Esta acción de revisión temporal
  no modifica las claves de la fuente.

## LODs y transiciones

Se conserva la ruta automática del contrato: objetivos 50 %/20 %, error 0,01,
distancia/radio por primitiva 30/80. LOD2 conserva caras adicionales de bordes
para respetar el error; no se fuerza a 7.900 triángulos. El inventario distingue
vértices referenciados de los buffers completos que realmente se suben a GPU.

Renders de los tres LODs a la misma cámara y siluetas de 200/100/50 px, además
de transiciones mixtas ±2 % alrededor de cada umbral nominal de reposo, con
perspectiva vertical 45° y salida 1920×1080, encuadrando el centro del material.
Se incluyen mirar abajo para capucha y puño para filos. El runtime selecciona
con sus bounds animados; las distancias exactas pueden variar con la pose.
Se han inspeccionado las imágenes reales. La selección de LOD es por material,
no un cambio simultáneo del cuerpo entero.

El máximo muestreado proyectado entre superficies consecutivas es **1,911 px**
en capucha al mirar abajo, y **1,610 px** en filos con puño. Se supera el control
exploratorio de 1 px; se conserva el dato. La revisión a distancia no muestra
pérdida relevante de silueta, dedos o cobertura, y se acepta un control local de
2 px muestreados. No se presenta como cero salto ni como garantía continua.
Las vistas ampliadas de LOD2 muestran pérdida de suavidad y de detalle interior;
no son su distancia de uso. No se necesita una ruta de LODs de autoría para esta
malla y estos pesos; **H08 debe repetir la revisión cuando cambie la deformación**.

## Rendimiento e integridad de la escena

La única modificación C++ permite seleccionar la entrada del benchmark con
`--hound-production-v17`; conserva la carga H06, las armas, los contadores y
el control v16. La ruta se imprime y el fixture registra hashes del glTF/BIN
v17. No se cambia el runtime del juego normal, el cooker o el gestor de memoria.

La serie completa, dos pasadas con audio nulo y dos con SDL3 real, obtiene
**305,90–309,88 FPS**, p99 máximo **3,528 ms**, máximo **3,554 ms**,
**0/1.440 frames >5 ms**. Las 53 primitivas previstas son visibles; máscara de
animación 127, cinco armas/máscara 31, nueve mapeos de paleta. Cero mallas o
texturas ausentes, cero evicciones en carga/medida; residencia **345,26 MiB**.
Se mantiene **200 FPS a 1080p con hasta ocho combatientes** en el escenario
diagnóstico de Ryzen 7 3700X/GTX 1070. No certifica ocho jugadores autoritativos.
Las dos capturas por pasada ocurren durante calentamiento (frames 60/90):
el FPS del HUD incluye arranque y no representa las 360 muestras posteriores.

**Salvedad conservada:** una pasada previa dio p99 **4,242 ms** y máximo
**4,580 ms**. Incumple el margen de trabajo de 4 ms aunque cumple 5 ms.
El lector se detuvo después por una comprobación nueva incorrecta sobre
aplazamientos de uploads durante el arranque, también presentes en el 106.
Se corrigió el lector y se ejecutaron las cuatro pasadas completas, sin borrar
la observación previa ni atribuir su tiempo a esa comprobación. Todos los
valores y distribuciones están en [mediciones.md](mediciones.md).

Release recompilado, **6/6 CTest**: assets, gpu_assets, animation_vfx,
skin_bounds, vulkan_sync y hound_runtime. Debug recompilado, **3/3**:
animation_vfx, vulkan_sync y hound_runtime, con validación Vulkan.
No se relajan las aserciones del control v16/104. Scripts compilables, revisión
de rendimiento sin trabajo/asignaciones nuevas por frame en juego normal.

## Entrada para un encargo posterior

Partir de v17 y su manifiesto; conservar v16 para bake/referencia. H08 debe
resolver codos profundos, cabeza/torso, pesos de capucha, holguras de abdomen,
inserciones de guantes y cierre palmar/pulgar/mano izquierda. El apoyo Soul Reaper
sigue siendo inválido; validar las cinco armas, offsets y sockets requerirá su
propia ficha. UVs, bakes, materiales y clips finales siguen pendientes.
La estructura de codos, cuello/capucha, abdomen y manos permite ese trabajo
sin rehacer la retopología. No se inicia ninguna de esas tareas en este hito.

## Reproducción

Desde la raíz, siempre con `rtk proxy`. Blender:
`.cache/blender/blender-4.5.13-windows-x64/blender.exe`; Python:
`.cache/legacy-tools/Scripts/python.exe`. Herramientas en `tools/art/`:

1. Sobre la fuente v16, `build_hound_production_v17.py` produce borrador en
   `.cache/hound-production-v17/`; `-- --final` solo admite rutas v17 libres.
   Usa `build/windows-vs/vcpkg_installed/x64-windows/bin/meshoptimizer.dll` existente.
2. Sobre la fuente final: `verify_hound_production_v17.py`,
   `review_hound_h05.py -- --v17 --audit`, `--stills` y `--video`;
   `verify_hound_rig_roundtrip_v03.py -- v17`. Controles v16 con sus recetas H07/99.
3. Cooker Release, argumentos: `D:/Projects/Gloom/assets`
   `D:/Projects/Gloom/.cache/hound-production-v17/cooked`
   `game:/characters/hound_rig/v17/hound-rig.gltf` `cache:/hound.gasset`.
   Crear antes el directorio cooked. Visor: mismos argumentos más
   `1 D:/Projects/Gloom/.cache/hound-production-v17/gloom-preview.ppm`.
4. Python `inspect_hound_production_v17.py`; Blender sobre v17 con
   `review_hound_lods_v17.py`. `-- --transitions-only` regenera solo transiciones.
5. Compilar `gloom` Release/Debug antes de medir. Python
   `prepare_hound_budget.py --v17`; control sin `--v17` conserva la ruta del 106.
   `inspect_hound_production_v17.py --source .cache/hound-production-v17/source/h07-v17/hound.gltf
   --cooked build/windows-vs/content/h07-v17/hound.gasset --probe` mide el fixture.
6. CTest secuencial Release con expresión
   `^gloom[.](assets|gpu_assets|animation_vfx|skin_bounds|vulkan_sync|hound_runtime)$`;
   Debug `^gloom[.](animation_vfx|vulkan_sync|hound_runtime)$`.
7. Sin builds/tests/renders simultáneos: Python `measure_hound_budget.py --v17`.
   Conserva las cuatro distribuciones incluso si falla el umbral temporal.
8. `report_hound_production_v17.py` regenera tablas, checks de contactos/hashes,
   siluetas, vídeo decodificado y capturas PNG. La pasada previa es evidencia
   histórica conservada en la tabla; su log local está en caché y no forma parte
   de las cuatro pasadas de reproducción.

Renders, vídeos, fixtures, cooked assets, logs y `.blend1` se quedan en caché.
Se versionan fuente/exportación/manifiesto, herramientas y Markdown del hito.
Commit local del hito 107; resolver con `git log -1 --oneline --grep='^hito 107:'`.
