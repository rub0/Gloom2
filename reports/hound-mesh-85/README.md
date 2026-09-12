# Hito 85 — interior continuo de manos de Hound

12 de septiembre de 2026. Encargo: «haz push y continua». Se publicó primero
el cierre pendiente de los hitos 83–84 en `rub0/Gloom2 → main`; comprobación
remota exacta: `a5302d16b393d1715f36c91106826c326f2a6309`.
Este nuevo hito tiene commit local, sin extender automáticamente aquel push.

## Resultado y alcance

[Ficha, comparación y vídeo](../../docs/art/hound/mesh-v05/README.md).
Fuente `art/characters/hound/v05/hound-mesh-v05.blend`; glTF/BIN separados en
`assets/characters/hound_rig/v05/`. Las fuentes anteriores no se modifican.

Dos palmas aisladas (720 vértices) se sustituyen por dos guantes interiores
continuos (2.180 vértices). Palma, cuatro dedos y pulgar comparten topología;
no se unen ni suavizan las placas metálicas. La jaula ramificada se subdivide
una vez con Catmull-Clark. Las secciones del pulgar transportan su orientación
entre segmentos para evitar el giro artificial observado en la primera revisión.
Se reduce el grosor interior tras detectar cruces visibles con falanges en reposo.

Cada mano: 1.090 vértices, 1.088 quads, un componente cerrado y orientado.
Total: 10.401 vértices, 20.386 triángulos, un mesh/skin y siete primitivas de
material. Son 2.920 triángulos adicionales; no es presupuesto de producción
ni medición de combate. No cambia código de C++, gameplay o personaje jugable.

Se conservan los otros 103 componentes: coordenadas, caras, materiales y pesos.
Esto incluye los brazos v04 y las 93 piezas rígidas, con la placa dorsal completa
en pico intacta. Los 53 huesos mantienen matrices, longitudes, padres y propiedad
de deformación. El clip se copia como `Hound05_joint_check`, con las mismas
curvas, tiempos, valores e interpolación. Solo se pesan las manos nuevas.

## Validación realizada

- Fuente final reabierta en Blender 4.5.13 LTS: un mesh y modificador de armadura
  LBS; manos conexas, manifold, bordes orientados, Euler 2 y volumen positivo.
- Conservación de 103 componentes, 53 huesos, materiales y claves comprobada.
- Pesos normalizados con hasta dos influencias. Las 93 piezas rígidas siguen
  vinculadas al 100 % a su hueso; no se hacen pesos finos o controles avanzados.
- 61 muestras del clip: coordenadas finitas, sin caras colapsadas, vuelta al
  reposo y error máximo de distancias rígidas 0,000000687 m.
- Cuatro casos FK adicionales de pulgar: ±20 grados en dos ejes para ambas
  cadenas, sin claves nuevas. Deformación efectiva, finitud y áreas verificadas;
  no son pruebas de oposición anatómica, contacto o agarre de armas.
- Reimportación glTF: 13 poses comparadas en ambas direcciones, máximo
  0,000002800 m. Un skin de 53 huesos, 43 nombres Legacy, un clip LINEAR/STEP
  con inicio cero, siete primitivas, sin texturas o dependencias externas.
- Regresión de fuente y roundtrip v04: pasa con error máximo 0,000002800 m.
- Cooker Gloom: escena `1674987440674172715`, cero dependencias externas.
- Visor Diligent/Vulkan: 7/7 piezas, siete batches, salida 0. Captura de reposo
  inspeccionada; no se afirma reproducción del clip en el motor.
- CTest Release `gloom.assets` y `gloom.gpu_assets`: 2/2. Debug
  `gloom.animation_vfx`: 1/1. Ejecutables existentes, sin rebuild o suite completa.
- Cuatro PNG inspeccionados y vídeo H.264 640×800 decodificado: 181 imágenes,
  30 fps y 6,033333 s de contenedor. Revisados los fotogramas de cada pose clave.
- Scripts compilados, longitudes de línea, enlaces locales y diff comprobados.

Las comprobaciones no certifican ausencia de autointersecciones/penetraciones,
acabado anatómico ni agarres. La continuidad es del guante, no de todas las piezas
de la mano o el personaje. El metal conserva su aspecto básico aprobado.

## Reproducción

Crear las carpetas de v05 antes del render. Con la fuente v04 abierta, ejecutar
`refine_hound_mesh_v05.py` y después `review_hound_mesh_v05.py`. El primer script
se niega a modificar una escena v05 existente. El segundo guarda fuente, cuatro
PNG y glTF/BIN; no ejecutarlo sobre futuras revisiones manuales sin cambiar rutas.
Blender MCP se usó con su modo seguro activo; la regeneración y las pruebas se
realizaron también mediante la CLI local, sin servicios externos o de pago.

```powershell
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v05/hound-mesh-v05.blend --python-exit-code 1 --python tools/art/verify_hound_mesh_v05.py --python tools/art/verify_hound_rig_roundtrip_v03.py -- v05
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v05/hound-mesh-v05.blend --python-exit-code 1 --python tools/art/animate_hound_rig_review_v03.py -- v05
rtk proxy build/windows-vs/Release/gloom_asset_cooker.exe assets .cache/hound-mesh-v05/cooked game:/characters/hound_rig/v05/hound-rig.gltf cache:/hound-rig.gasset
```

Los scripts compartidos de roundtrip y vídeo admiten `-- v05`, manteniendo
v03 por defecto y `-- v04`. Logs/capturas derivados en `.cache/hound-mesh-v05/`;
no se versionan cachés, builds o copias `.blend1`.

## Continuación

Continuar cintura/ropa y rostro; después comprobar contactos y agarres reales.
Conservar v02/v03/v04/v05, usando otra versión para el próximo pase. El rig
sigue siendo diagnóstico: sin retargeting, caminar/Bite finales, UVs/texturas
finales ni integración del nuevo personaje en partida. Este cierre técnico
no sustituye la revisión artística del usuario ni aprueba la malla definitiva.
