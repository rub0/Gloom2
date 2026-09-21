# Hito 96 — preparar las correcciones artísticas de H05

21 de septiembre de 2026. Encargo: reunir todo el feedback del usuario y
dejarlo preparado para modificar después. Workspace limpio al comenzar;
entrada documental y artística del hito 95, commit `3e8bfd8`.

## Entrega

[Especificación ejecutable F01–F06](../../docs/art/hound/tasks/H05-feedback-2026-09-21.md).

Recoge tres pinchos por antebrazo, lectura de garras, continuidad de armadura
pie/rodilla con remate algo superior, un pincho posterior por lado, corrección
clavícula/peto y abdominales más naturales.
Distingue las indicaciones del usuario de interpretaciones de modelado;
incluye criterios visuales, orden, componentes orientativos, conservación,
salidas previstas, comprobaciones y un encargo reutilizable sin historial.

Las decisiones aún no cuantificadas se concretarán al modelar y se mostrarán
en la nueva candidata; no se atribuye al usuario un número distinto de dedos,
una malla rígida única para toda la pierna ni medidas que no pidió.
El aspecto de garra se propone sobre la articulación existente.

Se enlaza desde H05, el índice, la galería v13 y ESTADO_ACTUAL.
H05 sigue **en revisión con correcciones pendientes**, sin aceptación de formas.
No se inicia H06 ni ninguna otra ficha.

## Conservación y siguiente entrada

Fuente vigente: `art/characters/hound/v13/hound-sculpture-v13.blend`.
Exportación vigente: `assets/characters/hound_rig/v13/hound-rig.gltf` y BIN.
Manifiesto: `art/characters/hound/v13/sculpture-reference.json`.
Nombres internos siguen siendo v12, tal como indica la especificación.

Los tres archivos mantienen sus hashes del manifiesto. No se modificaron
modelos, recursos gráficos, exportaciones ni scripts; no se generó una v14.
Las carpetas v14 de autoría, exportación y evidencia estaban libres al comprobarlas;
son propuestas de salida, deben revisarse de nuevo antes de editar.

## Comprobaciones

Lectura acotada de estado, H05, índice y CONTEXTO; búsquedas de componentes en
los scripts existentes. No había AGENTS.md adicionales en docs/reports.
Rutas y enlaces de los documentos afectados comprobados; integridad de la
fuente/exportación contrastada con SHA-256; diff y alcance solo documental revisados.
No se ejecutaron Blender, cooker, builds ni pruebas del motor: esta entrega
solo prepara el trabajo y no modifica sus entradas.

Comandos usados: `rtk git status --short`, `rtk git log -5 --oneline`,
`rtk rg` para inventario de componentes/instrucciones, lecturas y escrituras
UTF-8 mediante `rtk proxy powershell`, comprobación local de enlaces/hashes,
`rtk git diff --check` y revisión del diff antes del commit.
El sandbox no iniciaba procesos; se utilizó ejecución revisada fuera de él.

Commit local del hito 96; resolver con `git log --oneline --grep='^hito 96:'`.
Sin push. La siguiente acción de modelado, cuando se encargue, es ejecutar
F01–F06 dentro de H05 sobre una versión nueva y presentar el resultado.
