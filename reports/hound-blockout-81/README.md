# Hito 81 — Hound v02: clavículas, capucha y manos

10 de septiembre de 2026. Revisión del volumen inicial según los comentarios
del usuario. Entrega técnica terminada; valoración artística de v02 pendiente.

Actualización posterior: el usuario aprueba el diseño con «apruebo el diseño».
Aceptación registrada en el [hito 82](../hound-approval-82/README.md);
el resto de este informe conserva el estado y las pruebas de la entrega original.

## Resultado

[Vistas y detalles de v02](../../docs/art/hound/blockout-v02/README.md).
Fuente: `art/characters/hound/v02/hound-blockout-v02.blend`.
Exportación: `assets/characters/hound_blockout/v02/hound-blockout.gltf` y BIN.

Las copas de hombro se sustituyen por placas junto a clavícula/cuello y raíces
de filos más mediales. Se añaden masas de deltoides descubierto. La capucha
mantiene la corona a 1,80 m, adelanta el borde frontal y baja/estrecha su abertura.

Las manos se reconstruyen con una base anatómica común: pulgar anterior,
palma medial y dorso lateral. Los dedos flexionan hacia la palma, no hacia la
placa dorsal como en v01. La placa cubre el dorso completo, se ensancha sobre
metacarpianos y termina en pico distal. Se conserva el pulgar separado y se
añade transición a la muñeca; no se ha añadido un rig ni validado agarres.

113 mallas editables, 7.626 triángulos evaluados, 7 materiales planos, altura
1,80 m, ancho 1,091746 m y fondo 0,382126 m. La silueta más ancha ahora pertenece
a los filos de antebrazo, no a las antiguas hombreras exteriores.
Los materiales y meshes se copian de forma independiente al crear v02.

Los archivos de v01 no se modifican. La fuente v02 conserva la escena v01
como comparación, pero no incluye sus objetos en el glTF. Se retiraron de la
sesión de Blender únicamente las escenas temporales de revisión/escala del
hito 80; sus imágenes y la fuente v01 siguen guardadas.

## Validación realizada

| Comprobación | Resultado |
| --- | --- |
| Fuente v02 abierta en Blender background | 113 meshes, 4 cámaras, 7.626 triángulos; caras de área positiva, coordenadas finitas, escala/suelo correctos |
| Verificación de cambios | 67 piezas ajenas a la revisión conservan geometría, topología, color y transformaciones |
| Hombros/capucha | Copas retiradas, placas mediales, deltoides de piel y labio de capucha adelantado/bajado |
| Ambas manos | Placa exterior, cobertura de palma y pico distal; pulgares anteriores y extremos de dedos hacia la palma |
| glTF v02 | 113 meshes, 7 materiales, normales y bounds; sin textura, skin, animación ni objetos de revisión |
| Reconstrucción desde el .blend v01 | BIN de geometría idéntico byte a byte |
| Regresión v01 | Validadores .blend/glTF correctos; fuente, glTF, BIN y vistas previas sin cambios Git |
| Cooker sobre fuente v02 | Scene 17374081639196224832, cero dependencias externas |
| Visor Diligent/Vulkan | 113/113 piezas visibles, 113 batches, salida 0 y cierre normal |
| CTest Release focalizado | gloom.assets y gloom.gpu_assets: 2/2 |
| Python y diff | compileall, líneas de código hasta 160 caracteres y comprobación de whitespace |

SHA-256 del BIN reproducido:

```text
286CCF07532656CE1A5AD16AE843A08D90C8751C88D10481200453D501050DB6
```

La comparación de transformaciones usa TRS almacenadas: `matrix_world` de
una escena inactiva puede seguir en identidad al cargar el archivo. Se comparan
coordenadas locales con tolerancia 1e-7 y TRS/rotaciones con tolerancia 1e-6,
sin confundir esa caché no evaluada con una modificación del modelo.
La igualdad binaria corresponde a la geometría exportada, no al .blend ni al
JSON completo con metadatos.

Se inspeccionaron la lámina de tres vistas, el tres cuartos y los detalles de
capucha/clavículas y dorso/palma, además de la captura del visor. La lámina de
manos muestra la misma mano izquierda desde ambos lados, girando copias reales.
Se desactivaron las sombras del estudio de manos para que sus rótulos no
proyectasen letras sobre el modelo; no se pintó ni retocó la geometría renderizada.
La cámara genérica de Gloom queda lejos y prueba integración, no acabado de cerca.

No hay cambios C++/shaders/gameplay. Se reutilizan los ejecutables Release
validados, sin reconstrucción del motor ni suite completa. No se afirma
ausencia de penetraciones durante animación, compatibilidad de rig, agarre de
armas, calidad final de piel/tela o presupuesto de combate.
Los 113 batches requieren reorganización posterior a la aprobación del volumen.

## Herramientas y reproducción

- `revise_hound_blockout_v02.py`: clona v01 y aplica solo las correcciones.
- `review_hound_blockout.py` y `turnaround_hound_blockout.py`: ahora usan la
  versión de la escena activa, validan v01/v02 y mantienen las rutas separadas.
- `detail_hound_blockout_v02.py`: primeros planos de las zonas revisadas.
- `verify_hound_blockout.py [v01|v02]` y `verify_hound_blend.py`: verificadores
  compartidos, con resultados esperados específicos para cada versión.
- `verify_hound_v02_changes.py`: comprobaciones geométricas de las correcciones
  y conservación del resto del cuerpo.

```powershell
& .cache/blender-mcp-env/Scripts/python.exe tools/art/verify_hound_blockout.py v02
& .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v02/hound-blockout-v02.blend --python-exit-code 1 --python tools/art/verify_hound_blend.py --python tools/art/verify_hound_v02_changes.py
ctest --preset windows-release -R '^gloom\.(assets|gpu_assets)$' --output-on-failure
```

Para reconstruir, abrir v01, ejecutar el script de revisión v02 y luego los
scripts de vistas/exportación con v02 activa. Crear antes los directorios de
cache `.cache/hound-blockout-v02/source` y `cooked`. Las carpetas de los
entregables ya están versionadas. No volver a ejecutar guardados sobre trabajo
posterior del artista sin preparar una versión nueva.

Logs, copia girada para el visor y capturas de integración:
`.cache/hound-blockout-v02/`. Se conservan en Git el .blend, la exportación,
las siete imágenes de revisión, los scripts y la documentación.
MCP local en modo seguro; sin llamadas a generadores 3D externos ni compras.

## Siguiente paso

El usuario valida la ubicación de la armadura, cuánto tapa la capucha y la
orientación/cobertura de las manos. Incorporar nuevos ajustes en otra versión,
antes de retopología, UVs, rig, texturas y animación. No se reemplaza el personaje
jugable ni se realiza push en esta entrega.
