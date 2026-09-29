# Hito 105 — preparación del encargo H07

29 de septiembre de 2026 · **Preparación documental terminada; H07 no ejecutada.**

Base `3b5925c`, hito 104, workspace limpio al comenzar. El usuario solicita
preparar H07 para una tarea nueva; esta entrega deja el encargo en la
[ficha H07](../../docs/art/hound/tasks/H07-malla-produccion.md) y el texto de inicio
en el [índice](../../docs/art/hound/tasks/README.md#encargo-preparado-para-h07).
No se crea ni se pone en marcha otra tarea automáticamente.

La ficha reúne entrada v16 exacta, referencias mínimas, decisiones de presupuesto
necesarias, límites de retopología/LODs, pruebas, entregables y cierre local.
H05 está aprobada; H06 sigue pendiente de presupuesto de producción. Los
281–284 FPS del 104 corresponden al escenario diagnóstico y no fijan límites
de triángulos, materiales, mapas o LODs. H07 comienza comprobando esa dependencia;
si sigue pendiente, su salida se limita a auditoría y plan, sin modificar assets.

Se hace explícito que los LODs de autoría necesitan una ruta de consumo comprobada,
que la regresión 104 carga v16 y no mide automáticamente un candidato, y que
el rig/agarre final pertenece a H08. Se conserva el objetivo de 200 FPS/1080p/ocho
en Ryzen 7 3700X y GTX 1070. La maestra v16 y sus exportaciones no se modifican.

Validación documental: 25 enlaces/anclas añadidos comprobados, los tres SHA-256
de v16 coinciden con el manifiesto, coherencia de estado/índice/contrato/roadmap
revisada y `git diff --check` sin errores.
Sin código, modelos ni benchmarks nuevos; no hace falta recompilar o ejecutar Blender.
Commit local de preparación: resolver con `git log -1 --oneline --grep='^hito 105:'`.
Esta preparación no cierra H06/H07 ni inicia H08. Sin push.
