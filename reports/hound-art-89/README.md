# Hito 89 — primera propuesta artística global de Hound v08

Trabajo del 15 de septiembre de 2026; cierre retomado el 16 de septiembre.
Tras aclarar que la escultura no estaba terminada, el usuario pidió «vale,
entonces continua» y después «continua por donde te quedaste». Se entrega
un pase artístico de formas secundarias para revisión, no la escultura final.
Cierre local, sin push. Se conserva el hito 88 ajeno (`1b9e537`) sin modificaciones.

## Resultado

[Ficha, comparativas, fuente y vídeo](../../docs/art/hound/mesh-v08/README.md).
Fuente en `art/characters/hound/v08/`, glTF/BIN en `assets/characters/hound_rig/v08/`.
Se compararon el boceto 01 aprobado, el concept original elegido y la v07.

- 33 placas reconstruidas a partir de los contornos v02, con campos curvos y
  bordes contenidos. Se conservan los espesores de referencia; la orientación
  de la mano y su extremo en pico permanecen. No se reconstruyen las bases
  claviculares ni sus filos, para conservar el ajuste validado en v07.
- Dos paños reconstruidos sin el volumen piramidal previo, ojos ajustados a
  la cara y una línea de boca cerrada: 38 componentes reconstruidos en total.
- Cinco superficies conservan topología/materiales/pesos y cambian solo relieve:
  brazos, cabeza, pantalones y faja. Desplazamientos máximos: brazos 7,953 mm,
  cabeza 5,389 mm, pantalones 7,767 mm y faja 3,500 mm.
- Otros 53 componentes conservan exactamente posiciones, caras, materiales y
  pesos. Incluyen capucha/cuello, clavículas/filos, falanges y guantes interiores.
- No cambian las matrices, longitudes o jerarquía de los 53 huesos ni las claves
  del clip diagnóstico; 87 componentes rígidos y máximo dos influencias.

Total: 20.130 vértices y 39.880 triángulos (+12.208; 44,1 %), 96 componentes,
un mesh/skin y siete materiales/primitivas. El BIN mide 1.636.744 bytes y el
glTF 83.404. El incremento corresponde a geometría de autoría, no a un presupuesto
aprobado de producción. Revisar densidad/zonas ocultas/LODs después del cierre
artístico; no aumentan las primitivas de material. No se hizo benchmark.

## Revisión e iteraciones

La primera revisión mostró bordes demasiado dominantes y pérdida de masa al
reducir el espesor. Se estrechó el acento claro y se restauraron los espesores
del diseño aprobado. Se diferenciaron los paños de las placas metálicas.

La vista posterior detectó un error de reconstrucción: `matrix_world` de las
referencias v02 archivadas permanecía sin evaluar porque sus colecciones están
excluidas. Se usa `matrix_basis` con aserción de ausencia de padre. Las placas
posteriores vuelven a su posición correcta y dejan de duplicarse en el abdomen.
El verificador compara ahora los extremos de cada placa con v07: tolerancia
de 16 mm para el cambio de campo/bisel; 18 mm en Y solo para los paños que
pierden su antigua cresta piramidal de 18 mm. No es una tolerancia de colisión.

## Validación

- Fuente final reabierta con Blender 4.5.13 LTS. Superficies nuevas/modificadas
  cerradas, conexas, manifold y orientadas; volumen positivo, sin caras
  degeneradas ni cruces detectados entre caras no adyacentes en reposo.
  Faja Euler 0; las demás superficies comprobadas Euler 2.
- 53 componentes conservados exactamente; otros cinco con pesos/topología
  iguales y desplazamiento menor de 13 mm. Extremos de placas y abertura
  aprobada de capucha comprobados. Altura 1,80 m.
- 61 muestras del clip: coordenadas finitas, caras sin colapsar, retorno a
  reposo; error máximo de distancias rígidas 0,000000687 m.
- Cuatro poses extra: giro de Head ±35° e inclinación Neck/Head ±10°/±20°.
  Cero cruces detectados capucha/clavículas y cero pares autointersectados
  no adyacentes de la capucha en esos casos. Área mínima global 0,000001200 m².
- glTF reimportado y comparado bidireccionalmente en 13 poses: máximo
  0,000002036 m. Un skin, 53 huesos/43 nombres Legacy, un clip LINEAR/STEP
  desde cero y siete primitivas. Sin texturas ni dependencias externas.
- Regresión v07 con el verificador compartido: pasa sus 61 muestras y cuatro
  poses de cuello. Sus fuentes y medios no se regeneran ni modifican.
- Cooker: escena `11889315981599309218`, cero dependencias externas.
- Visor Diligent/Vulkan: 7/7 piezas, siete batches, salida 0; captura estática
  inspeccionada. No valida reproducción del clip dentro del motor.
- CTest Release assets/gpu_assets 2/2 y Debug animation_vfx 1/1, ejecutados
  el 16 de septiembre sobre los binarios existentes. Sin rebuild de C++,
  suite completa o partida manual.
- Seis PNG revisados, cuatro poses adicionales y vídeo diagnóstico inspeccionados.
  H.264 640×800, 181 frames decodificados a 30 fps, 6,033333 s de contenedor.
- Sintaxis Python, líneas hasta 160 caracteres, enlaces locales y diff comprobados.

Los tests BVH cubren pares de caras, no profundidad de penetración ni separación
mínima. No certifican todos los contactos entre piezas, capucha/cara, agarres,
construcción física de armadura/tela, anatomía o calidad artística final.

## Reproducción

Crear las carpetas v08 y `.cache/hound-mesh-v08/cooked` antes de ejecutar.
El generador rechaza una escena v08 existente. No regenerar encima de cambios
manuales posteriores sin adaptar las rutas de salida.

```powershell
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v07/hound-mesh-v07.blend --python-exit-code 1 --python tools/art/refine_hound_mesh_v08.py --python tools/art/review_hound_mesh_v06.py -- v08
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v08/hound-mesh-v08.blend --python-exit-code 1 --python tools/art/verify_hound_mesh_v06.py --python tools/art/verify_hound_rig_roundtrip_v03.py -- v08 --render-stress
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v08/hound-mesh-v08.blend --python-exit-code 1 --python tools/art/animate_hound_rig_review_v03.py -- v08
rtk proxy build/windows-vs/Release/gloom_asset_cooker.exe assets .cache/hound-mesh-v08/cooked game:/characters/hound_rig/v08/hound-rig.gltf cache:/hound-rig.gasset
```

Siguiendo ponytail se reutilizan los scripts de revisión/verificación v06 y
roundtrip/vídeo v03 mediante argumento de versión; sus valores predeterminados
no cambian. Se usa Blender local, con inspección por MCP y generación/pruebas
por CLI, sin dependencias ni servicios nuevos. Solo los medios de revisión,
fuente, exportación, scripts y documentación se versionan; logs y copias
automáticas `.blend1` quedan fuera.

## Pendiente

Revisión artística de la propuesta v08 y remates de carcasas de guanteletes y
grebas, botas, dedos, formas faciales, uniones y ropa. Mantener v02–v08 intactas
y el rig diagnóstico. La escultura sigue abierta: no añadir una aprobación
del usuario que no se ha dado ni sustituirla por los tests numéricos.
Sin UVs/texturas finales, rig facial, controles de producción, retargeting,
caminar/Bite finales o sustitución del Hound jugable.
