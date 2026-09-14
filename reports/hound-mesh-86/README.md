# Hito 86 — continuidad de cintura, ropa y rostro de Hound

Trabajo del 12 de septiembre de 2026; cierre retomado el 14 de septiembre.
El usuario indicó «adelante» tras v05 y después «continua por donde lo dejaste».
Se completa el pase anunciado de cintura, ropa y rostro. Cierre local, sin push.
El hito 85 sigue en `56d3fc3`; último remoto comprobado `a5302d1` (hitos 83–84).

## Resultado acotado

[Ficha, comparativas y vídeo](../../docs/art/hound/mesh-v06/README.md).
Fuente `art/characters/hound/v06/hound-mesh-v06.blend`; exportación glTF/BIN
en `assets/characters/hound_rig/v06/`. Las versiones anteriores no cambian.

Se reemplazan once componentes: pelvis, dos piernas, faja y siete bloques de
cabeza/mandíbula/nariz/mejillas/cejas. Se retiran 1.428 vértices y se añaden
4.779, repartidos en tres superficies nuevas:

| Superficie | Vértices | Caras | Construcción |
| --- | ---: | ---: | --- |
| Pantalones | 1.163 | 1.131 | 1.128 quads y tres tapas, cadera/entrepierna/piernas conexas |
| Faja | 1.248 | 1.248 | Quads, envoltura de 3 mm, interior hueco |
| Cabeza neutra | 2.368 | 2.306 | 2.304 quads y dos tapas; nariz, mejillas, cejas y mandíbula continuas |

Pantalón: dos bucles de cadera comparten una costura topológica descendente de
entrepierna, y sus mitades exteriores se unen a la cintura. Las secciones de
pierna se remuestrean sobre las anteriores y transicionan hacia esa unión;
pliegues de hasta 2,5 mm en muslo. Se mantiene tejido oscuro bajo las grebas.
Faja: envoltura independiente con pliegues suaves, sin tapas macizas sobre la pelvis.
Cabeza: superficie de anillos y relieves de facciones, sin bloques internos superpuestos.
No representa retopología facial definitiva para ojos, boca o mandíbula animada.

Los otros 94 componentes conservan coordenadas, caras, índices/colores de material
y pesos. Incluyen capucha completa, ojos, boca provisional, todas las placas,
brazos v04 y manos v05. Los 53 huesos conservan matrices de reposo, longitudes,
padres y deformación. Las curvas del clip se copian sin cambiar tiempos, valores
o interpolación a `Hound06_joint_check`.

Ahora se registran 87 componentes rígidos: los siete bloques faciales se sustituyen
por una cabeza, sin retirar armadura. Total: 13.752 vértices, 27.124 triángulos,
un mesh/skin y siete primitivas. +6.738 triángulos (33,1 %); el incremento es de
autoría, no presupuesto aprobado ni benchmark. Revisar reducción de densidad
en zonas ocultas y LODs antes de producción; no aumentan las primitivas de material.

## Validación

- Fuente final reabierta en Blender 4.5.13 LTS. Tres superficies cerradas,
  conexas, manifold y orientadas; Euler 2 para pantalón/cabeza y 0 para la faja
  hueca. Volúmenes positivos, sin caras degeneradas de reposo.
- Conservación exacta de 94 componentes, materiales, 53 huesos y claves del clip.
- Pesos normalizados, máximo dos influencias; 87 componentes rígidos con peso
  unitario a su hueso. Cabeza limitada al volumen bajo la capucha; altura 1,80 m.
- 61 muestras: coordenadas finitas, sin caras colapsadas, retorno a reposo;
  error máximo de distancias rígidas 0,000000687 m.
- Cuatro poses extra: flexión independiente de ambas caderas/rodillas, apertura
  de piernas y flexión/torsión de torso/cabeza. Se aísla temporalmente la acción
  para que el render no la restablezca; se verifica desplazamiento efectivo
  mayor de 25 mm en el componente probado. No se guardan claves ni poses nuevas.
  Área mínima observada 0,000002317 m²; las cuatro capturas corregidas se inspeccionan.
- glTF reimportado, 13 poses en ambas direcciones: máximo 0,000002800 m.
  Un skin/53 huesos/43 nombres Legacy, un clip LINEAR/STEP desde cero, siete
  primitivas y ninguna dependencia externa o textura.
- Regresión v05 de fuente y roundtrip: pasa, máximo 0,000002800 m.
- Cooker: escena `5379349391829840380`, cero dependencias externas.
- Visor Diligent/Vulkan: 7/7 piezas, siete batches y salida 0. Captura de reposo
  inspeccionada; no valida la reproducción del clip en el motor.
- CTest Release assets/gpu_assets 2/2 y Debug animation_vfx 1/1. Ejecutables
  existentes, sin rebuild de C++, suite completa, benchmark o partida manual.
- Seis PNG revisados. Vídeo H.264 640×800 decodificado completo: 181 imágenes
  a 30 fps, 6,033333 s de contenedor; poses seleccionadas inspeccionadas.
- Sintaxis Python, líneas hasta 160 caracteres, enlaces locales y diff comprobados.
- Al retomar el 14 de septiembre se revalidan fuente/roundtrip y las tres pruebas
  CTest sobre los archivos existentes, sin regenerar el modelo. Revisión mínima
  de scripts: se mantienen Blender y los verificadores locales, sin dependencias
  nuevas ni refactorización de las versiones anteriores.

Son pruebas numéricas y revisiones de poses, no certificación de autointersecciones,
contactos, anatomía final o agarres. Los ajustes de protectores y cuello/capucha
siguen pendientes. No hay simulación de tela, rig facial, UVs/texturas, animaciones
de producción, retargeting o sustitución del personaje jugable.

## Reproducción

Crear antes las carpetas v06. Con v05 abierta, ejecutar `refine_hound_mesh_v06.py`;
se niega a modificar una escena v06 existente. `review_hound_mesh_v06.py` guarda
fuente, seis PNG y glTF/BIN. No ejecutar esos guardados sobre cambios manuales
posteriores sin adaptar las rutas. Blender MCP se usó con modo seguro activo;
regeneración y verificaciones también mediante la CLI local, sin servicios de pago.

```powershell
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v06/hound-mesh-v06.blend --python-exit-code 1 --python tools/art/verify_hound_mesh_v06.py --python tools/art/verify_hound_rig_roundtrip_v03.py -- v06 --render-stress
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v06/hound-mesh-v06.blend --python-exit-code 1 --python tools/art/animate_hound_rig_review_v03.py -- v06
rtk proxy build/windows-vs/Release/gloom_asset_cooker.exe assets .cache/hound-mesh-v06/cooked game:/characters/hound_rig/v06/hound-rig.gltf cache:/hound-rig.gasset
```

Los scripts compartidos conservan v03 por defecto y aceptan v04/v05/v06.
`--render-stress` es opcional y escribe solo capturas en `.cache/hound-mesh-v06/`;
no cambia la fuente o el clip. Logs e imágenes de integración permanecen en caché;
fuentes, exportación, medios de revisión, scripts y documentación se versionan.

## Siguiente pase

Capucha/cuello y ajustes de placas; después revisar contactos y agarres reales,
anatomía y topología facial específica. Conservar v02–v06. Mantener el rig
diagnóstico hasta estabilizar la malla: este cierre no es aprobación artística
de v06 ni de la malla/rig definitivos.
