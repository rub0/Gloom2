# Hito 82 — aprobación del diseño de Hound v02

10 de septiembre de 2026. Registro de la respuesta del usuario a la entrega
del hito 81: «apruebo el diseño».

## Decisión

El [volumen básico v02 y sus detalles](../../docs/art/hound/blockout-v02/README.md)
quedan aprobados como referencia de diseño. Conservar silueta, proporciones,
hombros libres y armadura clavicular, capucha más cerrada y orientación de las
manos con placa dorsal completa terminada en pico.

La fuente aprobada es `art/characters/hound/v02/hound-blockout-v02.blend`,
entregada en el commit `67cda2e`. No se modifica en este hito. Tampoco cambian
exportaciones, imágenes, scripts ni el personaje jugable.

La aprobación no convierte los materiales planos, el facetado o la anatomía
provisional en acabado final, ni demuestra compatibilidad de rig o animaciones.

## Siguiente fase acordada por el flujo artístico

1. Trabajar en una nueva versión conservando v01/v02 y las formas aprobadas.
2. Preparar topología de zonas deformables y placas rígidas para un rig de prueba;
   evaluar la reutilización del rig existente sin darla por garantizada.
3. Revisar caminar, apuntar y Bite, deformación, penetraciones y agarres reales.
4. Tras esa revisión, abordar UVs, materiales y detalle final; verificar después
   integración FPS/TPS y coste en Gloom antes de sustituir el personaje jugable.

Este hito registra la aceptación; no ejecuta esas fases.

## Validación y entrega

- Workspace limpio antes de editar; cambios limitados a documentación.
- Diff revisado y comprobación de whitespace sin errores.
- Enlaces Markdown locales de los documentos modificados comprobados.
- Fuentes 3D, glTF/BIN, vistas y herramientas sin cambios respecto a `67cda2e`.
- No se repiten builds, renders ni pruebas del motor por una aceptación documental;
  las pruebas técnicas previas constan en el [informe 81](../hound-blockout-81/README.md).
- Estado, guía, flujo Blender y ficha actualizados. Cierre mediante commit local,
  sin push ni llamadas a servicios externos.
