# Hito 80 — primer volumen editable de Hound

10 de septiembre de 2026. Entrega técnica de un blockout; aceptación artística
pendiente. El usuario pidió hacer push del hito previo y empezar Hound básico.

## Push solicitado

La revisión automática detuvo inicialmente el push para confirmar el destino.
Se comprobó que `rub0/Gloom2` es público y el usuario confirmó expresamente la
publicación en `main` de los siete commits pendientes hasta `988437d`.
Push completado desde `0226933` hasta
`988437d76e8f1b3775b31b58c3a4ab331ba44c7f`; hash remoto verificado con ls-remote.
Esa confirmación no se ha extendido automáticamente al nuevo hito 80.

## Recursos entregados

- `art/characters/hound/v01/hound-blockout-v01.blend`: fuente editable real,
  colección de modelo separada y cuatro cámaras; aproximadamente 1,4 MB.
- `assets/characters/hound_blockout/v01/hound-blockout.gltf` y BIN: exportación
  estática, fuera de las rutas que usa el personaje jugable.
- [Lámina de frente/perfil/espalda](../../docs/art/hound/blockout-v01/turnaround.png)
  y cuatro PNG individuales, obtenidos con Blender Workbench.
- [Ficha del volumen y revisión pendiente](../../docs/art/hound/blockout-v01/README.md).
- Cinco scripts de construcción, revisión, lámina y validación en `tools/art/`.

No se utilizó generación de imágenes, text-to-3D ni servicios 3D de pago.
Las herramientas Blender MCP ya están disponibles de forma nativa en esta
tarea; se utilizó execute_blender_code directamente, manteniendo el modo seguro.
Se conserva la guía 0.1 y no se modifica el boceto aprobado.

## Geometría y decisiones

105 piezas, 6.974 triángulos evaluados y 7 materiales planos. Ancho total
1,143714 m, altura 1,80 m y fondo 0,382126 m. Origen vertical en el suelo;
-Y frontal en Blender, +Z frontal y Y vertical al exportar a glTF.

Capucha abierta con espesor, cabeza con planos básicos, ojos pequeños naranja,
coraza/abdomen segmentados, brazos desnudos, tres filos por hombrera, guanteletes,
manos con dedos separados, cintura/pantalones y protecciones de piernas.
Se ajustó el tamaño inicial de las manos por un factor de 1,25 desde las muñecas
y se acortaron los pliegues de cintura para no convertirlos en un faldón.
La espalda usa dos placas anchas y una columna corta como interpretación nueva.

El acabado facetado es una herramienta de revisión, no una propuesta de estilo
final. La cara no es un esculpido definitivo y las manos no están listas para
empuñar armas. No se aprueba por esta entrega la anatomía oculta, la espalda,
las articulaciones, retopología, skinning ni texturas.

## Validación

| Comprobación | Resultado |
| --- | --- |
| Apertura del .blend guardado en Blender background | 105 meshes, 4 cámaras, sin rig; código 0 |
| Geometría de fuente | 6.974 triángulos, caras de área positiva, coordenadas finitas, suelo/altura correctos |
| Verificador glTF | 105 meshes, 7 materiales, normales, transforms y bounds correctos; sin textura/skin/animación |
| Comparación de escala con Archangel original | 1,79999997 m en glTF; coincide con 1,80 m del bloque |
| Reconstrucción desde configuración de fábrica | BIN geométrico idéntico byte a byte |
| Cooker de la exportación fuente | Scene 11614133332254446729, cero dependencias externas |
| Exportación de prueba para el visor | Copia temporal girada 180°; no cambia el frente de la fuente |
| Render Diligent/Vulkan | 105/105 piezas visibles, 105 batches, salida 0 y cierre normal |
| CTest Release focalizado | gloom.assets y gloom.gpu_assets, 2/2 |
| Sintaxis Python | compileall correcto |

SHA-256 del BIN reproducido:

```text
DF6CC4A4D53E4B8FFD46F944DEAF8E4F52F3A96A989DDF3EA5261B63C7A24F3D
```

La comparación binaria cubre la geometría exportada, no promete identidad
binaria del .blend ni del JSON con metadatos. Se inspeccionaron las vistas
individuales a través de la lámina y el tres cuartos, además de la captura del
visor. Su cámara fija queda lejos: esa captura prueba integración, no calidad
del acabado de cerca ni comportamiento en Factory.

La importación de Archangel crea también una Icosphere de forma de hueso en
`glTF_not_exported`; se excluyó de la medición. Contarla daba una falsa altura
de 2,8 m. El verificador independiente lee la geometría real del glTF original.
La reproducción reveló que la selección de Blender puede incluir objetos de
otras escenas: las exportaciones de revisión usan también `use_active_scene=True`.

No hay cambios C++ ni de runtime que requieran reconstruir el motor. Se usaron
los ejecutables Release validados en el hito 79. No se ejecutó la suite completa,
una partida, animaciones, agarres de armas ni comparativas de rendimiento.
105 batches son un coste de esta organización editable, no un presupuesto final.

## Repetir las comprobaciones

```powershell
& .cache/blender-mcp-env/Scripts/python.exe tools/art/verify_hound_blockout.py
& .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v01/hound-blockout-v01.blend --python-exit-code 1 --python tools/art/verify_hound_blend.py
ctest --preset windows-release -R '^gloom\.(assets|gpu_assets)$' --output-on-failure
```

Para construir desde cero usar una instancia limpia, ejecutar en orden el
constructor, la revisión y la lámina mediante MCP. Crear antes las carpetas de
salida (las carpetas versionadas ya existen; para la cache:
`New-Item -ItemType Directory -Force .cache/hound-blockout/source,.cache/hound-blockout/cooked`).
Guardar copias/versiones nuevas antes de repetir sobre trabajo del usuario.

Logs regenerables en `.cache/hound-blockout/`: `blend-verification.log`,
`rebuild.log`, `viewer.log`, fuente de prueba girada y capturas Vulkan.
La fuente .blend y los PNG de aceptación sí están versionados, fuera de cache.

## Siguiente paso

El usuario revisa proporciones, capucha, hombros, manos y espalda. Incorporar
ajustes de volumen en una versión nueva antes de retopología, UVs, rig, texturas
y animación. No reemplazar todavía `assets/characters/hound.gltf` ni el cuerpo
de Archangel usado por el Hound jugable.
