# Hito 87 — capucha, cuello y encaje clavicular de Hound

14 de septiembre de 2026. El usuario indica «adelante» tras el cierre de v06.
Se completa el pase anunciado, conservando todas las fuentes anteriores.
Cierre local, sin push. Hito 86: `5e494ea`; último remoto comprobado: `a5302d1`.

## Resultado acotado

[Ficha, comparaciones y vídeo](../../docs/art/hound/mesh-v07/README.md).
Fuente en `art/characters/hound/v07/hound-mesh-v07.blend`; glTF/BIN en
`assets/characters/hound_rig/v07/`. Se sustituyen `Hood_shell`,
`Hood_opening_rim` y `Neck` por dos superficies, con 48 muestras por contorno
de capucha y 32 por sección de cuello:

| Superficie | Vértices | Caras | Construcción |
| --- | ---: | ---: | --- |
| Capucha | 1.106 | 1.152 | 1.056 quads y 96 triángulos; exterior, labio y forro conexos |
| Cuello | 320 | 290 | 288 quads y dos tapas; diez secciones, separado de cabeza y torso |

Capucha: polilíneas frontales exactas, incluyendo abertura a Y = −0,194 m y
contorno exterior a Y = −0,170 m. Perfil posterior suavizado y lateral inferior
recogido para alojarse detrás de los filos originales. Cuello: transiciones
de volumen y relieves suaves. No se fusiona con la cabeza o el torso.

Las dos bases claviculares conservan topología, materiales, pesos y coordenadas
Y/Z; solo cambia X donde |X| < 0,185 m. La compresión medial separa el metal de
la capucha sin mover sus extremos exteriores ni ninguno de los seis filos.
Los otros 92 componentes conservan coordenadas, caras, sombreado y pesos.

Total: 14.026 vértices, 27.672 triángulos (+548, 2,02 %), un mesh/skin,
siete primitivas/materiales, 96 componentes y 87 rígidos. No aumentan los batches
de material. El BIN mide 1.312.520 bytes y el glTF 82.513; no son un benchmark
de ejecución ni un presupuesto artístico final. Densidad y LODs siguen pendientes.

Los 53 huesos conservan matrices, longitudes, jerarquía y deformación. Las curvas
del clip se copian sin cambiar claves/interpolación a `Hound07_joint_check`.
Pesos nuevos provisionales: capucha Spine2→Head entre Z 1,54 y 1,70 m; cuello
Spine2→Neck entre 1,435 y 1,505 m, y Neck→Head entre 1,505 y 1,625 m.
Máximo dos influencias normalizadas. No hay controles nuevos ni rig definitivo.

## Validación

- Fuente final reabierta con Blender 4.5.13 LTS. Ambas superficies conexas,
  manifold y orientadas, Euler 2 y volúmenes positivos; ninguna cara degenerada
  ni cruce detectado entre caras no adyacentes en reposo.
- Conservación exacta de 92 componentes, fórmula del recorte de las dos bases,
  colores/materiales, 53 huesos y claves del clip. Altura 1,80 m.
- 48 puntos de abertura y 48 de contorno frontal comprobados contra las
  polilíneas aprobadas; diferencia menor de 0,0000001 m.
- BVH de superficies en reposo: v06 tiene 40 pares de caras capucha/base por
  lado, v07 cero. Las seis cuchillas tampoco intersectan capucha/cuello.
- 61 muestras del clip: coordenadas finitas, caras sin colapsar, vuelta a reposo
  y error máximo de distancias rígidas 0,000000687 m.
- Cuatro poses extra, aislando temporalmente la acción: giro de Head ±35°,
  e inclinación de Neck/Head ±10°/±20°. Desplazamiento efectivo de la capucha
  mayor de 25 mm; área mínima global 0,000002317 m². Renders inspeccionados.
- La primera transición de pesos produjo 46 pares capucha/clavículas en cada
  giro y dos pares internos al inclinar hacia delante. Mantener la zona baja
  con el torso y retrasar la transición a Head corrige esos casos: cero cruces
  capucha/clavículas y cero autointersecciones no adyacentes en las cuatro poses.
  Esos ceros quedan como regresión ejecutable; no se guardan las poses extra.
- glTF reimportado y comparado bidireccionalmente en 13 poses: error máximo
  0,000002036 m. Un skin, 53 huesos/43 nombres Legacy, siete primitivas,
  un clip LINEAR/STEP desde cero y ninguna textura externa.
- Regresión de la fuente v06 con el verificador compartido: pasa sus 61 muestras
  y cuatro poses previas. No se regeneran ni modifican sus archivos.
- Cooker de la exportación final: escena `8697359741167235809`, cero dependencias.
- Visor Diligent/Vulkan: 7/7 piezas, siete batches, salida 0 y captura estática
  revisada. No se prueba la reproducción del clip dentro del motor.
- CTest Release assets/gpu_assets 2/2 y Debug animation_vfx 1/1, usando los
  ejecutables existentes; sin rebuild de C++, suite completa o partida manual.
- Seis PNG inspeccionados. Vídeo H.264 640×800 a 30 fps: 181 frames decodificados,
  6,033333 s de contenedor; poses seleccionadas revisadas.
- Sintaxis Python, límite de 160 caracteres, enlaces locales y diff comprobados.

Los conteos BVH son pares de caras que se cruzan, no profundidad, distancia mínima
o certificado de ausencia de penetraciones. No se cubren todos los contactos
del cuerpo, capucha/cara, posiciones intermedias o poses de combate. El skinning
no simula tela; el acabado y la anatomía siguen requiriendo revisión artística.

## Reproducción y herramientas

Crear las carpetas v07 y `.cache/hound-mesh-v07/cooked` antes de ejecutar.
Con v06 abierta, `refine_hound_mesh_v07.py` crea otra escena y rechaza una v07
ya existente. El script de revisión guarda fuente, seis PNG y glTF/BIN. No
regenerar sobre cambios manuales posteriores sin adaptar los destinos.

```powershell
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v06/hound-mesh-v06.blend --python-exit-code 1 --python tools/art/refine_hound_mesh_v07.py --python tools/art/review_hound_mesh_v06.py -- v07
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v07/hound-mesh-v07.blend --python-exit-code 1 --python tools/art/verify_hound_mesh_v06.py --python tools/art/verify_hound_rig_roundtrip_v03.py -- v07 --render-stress
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v07/hound-mesh-v07.blend --python-exit-code 1 --python tools/art/animate_hound_rig_review_v03.py -- v07
rtk proxy build/windows-vs/Release/gloom_asset_cooker.exe assets .cache/hound-mesh-v07/cooked game:/characters/hound_rig/v07/hound-rig.gltf cache:/hound-rig.gasset
```

Siguiendo ponytail se reutilizan revisión/verificación v06 y roundtrip/vídeo v03,
con argumento de versión; sus valores predeterminados no cambian. Blender MCP
se utiliza con modo seguro activo y la CLI local para regeneración y pruebas.
No se añaden dependencias, servicios de pago ni código del motor.
Las capturas de esfuerzo y los logs quedan en `.cache`; se versionan fuente,
glTF/BIN, medios de revisión, scripts y documentación, no las copias `.blend1`.

## Siguiente pase

Contactos del conjunto y agarres reales; después anatomía/acabado y topología
facial específica. Conservar v02–v07 y seguir con el rig diagnóstico hasta
estabilizar la malla. Sin UVs/texturas finales, controles de producción,
retargeting, caminar/Bite finales ni sustitución del personaje jugable.
Este cierre técnico no equivale a aprobación artística nueva de v07.
