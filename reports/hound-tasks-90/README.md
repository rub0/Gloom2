# Hito 90 — traspaso de Hound en tareas separadas

16 de septiembre de 2026. El usuario pide documentar lo que falta para
trabajarlo en tareas nuevas sin contexto de conversación y ahorrar tokens.

## Entrega

[Índice de tareas](../../docs/art/hound/tasks/README.md), contexto mínimo y
13 fichas con entrada, alcance, exclusiones, verificaciones y criterio de cierre.
Cubren remates de manos/piernas/rostro/ropa, aprobación de escultura, contrato/
presupuesto, malla/LODs, rig, UVs/bake, materiales, locomoción, combate e integración.

Cada tarea lee únicamente el inicio del estado, índice/contexto y su ficha.
Los productos de cada fase se registrarán con rutas exactas en el índice,
evitando releer todos los informes o adivinar la versión a partir de carpetas.
Incluye encargo reutilizable, dependencias, control de aprobación y cierre local.

Base de contenido: hito 89, commit `68e686e`, fuente v08. No se ha hecho nueva
escultura, rig, textura, animación o integración en este hito. Todas las fichas
están pendientes; la v08 sigue sin aprobación de escultura final.
H06 puede investigarse antes del cierre artístico, pero las ediciones binarias
se encadenan para preservar la fuente acumulada.

## Comprobaciones

Comprobados 20 archivos Markdown: 182 enlaces locales y 12 rutas de fuentes/
herramientas existentes. Las 13 fichas están indexadas como pendientes y sus
dependencias no contienen ciclos ni IDs inexistentes. Codificación UTF-8 y
diff sin errores de espacios. El paquete suma 4.581 palabras, pero una tarea
no carga el paquete entero: lee el contexto/índice y únicamente su ficha.
No se requiere Blender/build/CTest: solo cambia Markdown, no el modelo ni código.

Siguiente encargo mínimo: ejecutar H01 del índice. El siguiente trabajo no
necesita esta conversación. Este hito no crea tareas de Codex ni hace push.
