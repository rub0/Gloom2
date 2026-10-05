# Hito 131 — workspace canónico de Hound y preparación Meshy

5 de octubre de 2026. Base Git: `4cf8af8`, workspace inicialmente limpio.
Se usa 131 porque 116–130 ya identifican fichas pendientes del plan C++;
ninguna de ellas se ejecuta ni se renumera con este encargo.

## Resultado

Workspace en `assets-source/hound/` con `ref/`, `docs/`, `work/` y `export/`.
Se guardan el master oficial aprobado en el encargo y la segunda lámina adjunta
como copias binarias exactas. Las notas se extraen completas del texto solicitado,
sin resumir ni reescribir su contenido. Se documentan estado, procedencia,
nombres futuros, trabajo intermedio, exportaciones, plan y prompt Meshy MCP.

Archivos nuevos:

- `assets-source/hound/README.md`
- `assets-source/hound/ref/Hound_master.jpg`
- `assets-source/hound/ref/Hound_turnaround_sheet.jpg`
- `assets-source/hound/ref/README.md`
- `assets-source/hound/docs/hound_notes.md`
- `assets-source/hound/docs/hound_status.md`
- `assets-source/hound/work/README.md`
- `assets-source/hound/export/README.md`
- `tools/meshy/meshy_hound_plan.md`
- `tools/meshy/meshy_hound_prompt.txt`
- `tasks/hound-workspace.md`
- Este informe.

Se actualizan `docs/ESTADO_ACTUAL.md`, el índice Hound y su contexto mínimo
para que tareas futuras encuentren el canon de este encargo y la producción previa.
El texto de `hound_notes.md` es el solicitado, con terminación final de línea;
solo la representación CRLF/LF puede variar al versionarlo con Git.

La lámina adjunta contiene cuatro vistas; se conserva como una sola imagen.
No se han creado archivos individuales, imágenes nuevas ni placeholders gráficos.
No se ha llamado a Meshy MCP, ejecutado el prompt, configurado una conexión
ni iniciado generación, rigging, animación o integración.

## Validación

- Copias originales y destino comparadas mediante SHA-256: idénticas.
- Master: 1086 × 1448, 2049478 bytes;
  `C37D24A0E9551791E53FEC444E6D55FF67B4C9D4DFCA52D267F7CC0505A9854B`.
- Lámina: 1448 × 1086, 2262767 bytes;
  `9EA1A43452F931E7AC23841FB0FA242056AB3D578A22D1BC3DD6BC8F59390014`.
- Imágenes JPG abiertas y dimensiones verificadas; diseño inspeccionado en los adjuntos.
- Notas comparadas íntegramente con el bloque `# Hound Notes` del encargo UTF-8.
- Comprobación local correcta: diez textos nuevos UTF-8, 30 enlaces locales,
  seis directorios requeridos y las cuatro imágenes individuales todavía ausentes.
- Diff revisado: solo documentación nueva/traspaso y dos imágenes suministradas;
  fuentes/exportaciones previas y código sin cambios.
- `rtk git diff --check` correcto; comprobación del contenido preparado con
  `rtk proxy git -c core.whitespace=-blank-at-eol diff --cached --check`,
  conservando los dos espacios Markdown explícitos de las notas solicitadas.
- Sin compilación/CTest/Blender: no cambia código, malla, rig ni runtime.

Las comprobaciones de copia y texto se ejecutan con PowerShell local a través
de `rtk proxy`; hashes calculados con SHA-256 de .NET. `Get-FileHash` no estaba
disponible en ese proceso, por lo que se sustituyó por .NET y se repitió con éxito.
Los helpers temporales viven en `.cache/` ignorada; no son productos versionados.

## Conservación y siguiente paso

Fuente acumulada intacta: `art/characters/hound/v17/hound-production-v17.blend`.
Exportación intacta: `assets/characters/hound_rig/v17/hound-rig.gltf` y `hound-rig.bin`.
Maestra de escultura aprobada conservada: `art/characters/hound/v16/hound-mesh-v16.blend`.
Rig existente de 53 huesos y acción diagnóstica conservados; H08 sigue pendiente.
Los estados de rig/animación pendientes del nuevo checklist son de la futura ruta Meshy.

Siguiente paso: preparar y revisar las cuatro vistas individuales partiendo de
`assets-source/hound/ref/Hound_turnaround_sheet.jpg`, compararlas con
`assets-source/hound/ref/Hound_master.jpg` y `assets-source/hound/docs/hound_notes.md`,
y actualizar el estado. **waiting for turnaround completion; do not call MCP yet.**
Después hará falta un encargo de generación y revisar el esquema MCP realmente disponible;
este setup no presupone capacidades concretas del proveedor.

Commit local del hito 131; resolver con `rtk git log -1 --oneline --grep='^hito 131:'`.
Sin push. El siguiente hito del plan C++ sigue siendo 116, no iniciado.
