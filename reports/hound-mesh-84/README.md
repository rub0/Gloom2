# Hito 84 — malla continua de hombros y brazos

10 de septiembre de 2026. Continúa el flujo acordado tras limitar v03 a rig de
prueba: trabajar la malla antes de invertir en controles, pesos y animaciones finales.
El hito 83 pendiente se cerró primero en `4aed691`. Este hito también tiene cierre
local; el push solicitado anteriormente publicó únicamente hasta `d7fc1f8`.

## Cambio acotado

[Ficha, comparación y vídeo](../../docs/art/hound/mesh-v04/README.md).
Fuente `art/characters/hound/v04/hound-mesh-v04.blend`, exportación en
`assets/characters/hound_rig/v04/`. v02/v03 conservadas sin cambios de recursos.

Los tres volúmenes superpuestos de cada brazo (deltoides, bíceps y extremidad
continua de v03) se sustituyen por una única superficie cerrada. Se muestrea
la envolvente exterior con rayos sobre la geometría anterior, usando 37 secciones
y 32 vértices por sección; dos pasadas locales suavizan las intersecciones.
No se hace un remallado global ni se altera el diseño de la armadura.

Cada brazo contiene 1.184 vértices, 1.152 quads laterales y dos tapas; todos
los bordes pertenecen a dos caras y la superficie es un único componente conexo.
Se retiran 2.208 vértices de los seis volúmenes originales y se añaden 2.368:
la malla completa queda en 8.941 vértices y 17.466 triángulos (+336 frente a v03).
Se mantienen una malla/skin y siete primitivas de material; el coste final de
combate y la reducción posterior de densidad siguen pendientes.

Conservación comprobada de vértices de los otros 103 componentes y colores
de los siete materiales. Los 53 huesos conservan matrices locales de reposo,
longitudes y padres. Las 93 piezas rígidas siguen pesadas al 100 % a un hueso.
El clip se copia como `Hound04_joint_check`; no se crean animaciones nuevas.
Los pesos nuevos de brazo son provisionales, con hasta dos influencias.

## Validación

- Fuente reabierta en Blender 4.5.13 LTS; dos brazos cerrados/conexos y Euler 2,
  sin bordes abiertos ni caras degeneradas en la malla de reposo.
- Vértices conservados de 103 componentes y resto del esqueleto verificados.
- Envolvente: distancia dirigida máxima de vértices nuevos a superficies previas
  0,0027626255 m. No mide simétricamente toda la superficie ni reemplaza revisión visual.
- 61 muestras del clip: pesos normalizados, coordenadas finitas, sin explosiones
  ni caras colapsadas, retorno a reposo, y error de distancias rígidas menor que
  0,000000687 m. No es prueba exhaustiva de intersecciones o calidad anatómica.
- glTF reimportado: 13 poses comparadas, error máximo 0,000002800 m. Una malla,
  siete primitivas, un skin con 53 huesos y clip LINEAR/STEP de inicio cero.
- Regresión del verificador sobre la fuente v03: pasa, error máximo 0,000004689 m.
- Cooker Gloom: escena `4772031101896625430`, cero dependencias externas.
- Visor Diligent/Vulkan: 7/7 piezas visibles, siete batches, salida 0. Captura
  de reposo inspeccionada; no demuestra reproducción del clip en el juego.
- CTest Release assets/gpu_assets 2/2 y Debug animation_vfx 1/1. Ejecutables
  existentes, sin cambios de C++, rebuild, suite completa o partida manual.
- Vistas de reposo/frente/codos/alcance y comparación inspeccionadas; vídeo
  H.264 640×800 decodificado, 181 imágenes a 30 fps, contenedor de 6,033333 s.
- Python compilado, líneas hasta 160 caracteres, enlaces locales y diff revisados.

## Reproducción

Con v03 abierta, ejecutar `refine_hound_mesh_v04.py`; se niega a sobrescribir
una escena v04. `review_hound_mesh_v04.py` genera fuente, glTF y cinco PNG.
Crear antes las carpetas de salida. No ejecutar guardados sobre revisiones
posteriores del artista sin adaptar primero las rutas/versiones.

```powershell
& .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v04/hound-mesh-v04.blend --python-exit-code 1 --python tools/art/verify_hound_mesh_v04.py --python tools/art/verify_hound_rig_roundtrip_v03.py -- v04
& .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v04/hound-mesh-v04.blend --python-exit-code 1 --python tools/art/animate_hound_rig_review_v03.py -- v04
& build/windows-vs/Release/gloom_asset_cooker.exe assets .cache/hound-mesh-v04/cooked game:/characters/hound_rig/v04/hound-rig.gltf cache:/hound-rig.gasset
```

El verificador de reimportación y el render de vídeo ahora aceptan `-- v04`;
sin ese argumento conservan el comportamiento v03. Logs/capturas de integración:
`.cache/hound-mesh-v04/`. Solo se versionan fuentes, exportación, medios de
revisión, scripts y documentación; no cachés de Blender ni builds.

## Próximo pase

Continuar topología/anatomía y conexiones de manos/dedos, cintura/ropa y rostro;
comprobar contactos con armaduras y agarres. El rig diagnóstico queda estable:
no avanzar aún a controles definitivos, retargeting o animaciones de producción.
No hay UVs/texturas finales ni sustitución del personaje jugable. Cierre de este
pase de brazos, no aceptación global de la malla ni del rig de producción.
