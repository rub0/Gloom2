# H06 — contrato de ejecución y presupuesto

Estado al 29/09/2026: **bloqueada; hito 102 ejecutado sin validar el presupuesto**.
El fallo de normales del diagnóstico 101 está corregido y comprobado en el
[hito técnico 103](../../../../reports/normal-maps-103/README.md).
[Contrato, objetivo confirmado y decisiones pendientes](../H06-contrato-presupuesto.md) ·
[Informe y reproducción](../../../../reports/hound-contract-101/README.md).
La fuente de medida es v16 aprobada en H05; no se ha fijado un presupuesto de
producción ni se habilita H07. El [hito 102 de Gloom](../../../../reports/eight-combatants-102/README.md)
midió ocho presentaciones jugables, pero incumplió 5 ms en p95/p99; siete
Hound v16 fallan al mapear constantes. La corrección de normales se realizó
en el hito técnico 103; no amplía las exclusiones de H06.
La continuación técnica está preparada en el [hito 104](../../../../reports/hound-runtime-104/README.md),
pendiente de ejecución en otra tarea; H07 sigue sin habilitarse.

## Entrada y alcance
Leer [contexto](CONTEXTO.md) e [índice](README.md), después las secciones relevantes de:
- [BLENDER_WORKFLOW](../../../BLENDER_WORKFLOW.md): contrato y reproducción.
- [CHARACTERS](../../../CHARACTERS.md) y [ANIMATION_VFX](../../../ANIMATION_VFX.md): rig/movimiento.
- Código: `src/assets/gltf_importer.cpp`, `src/assets/animation.cpp`,
  `include/gloom/render/material_surface.hpp`,
  `src/gameplay/character_animation.cpp` y `include/gloom/gameplay/character_presentation.hpp`.

Puede investigarse con v08 mientras H01–H05 avanzan; actualizar las medidas
si H05 cambia significativamente el asset. El código/estado vigente prevalecen
sobre cifras y protocolos de informes históricos.

## Trabajo acotado
Probar con fixtures pequeñas el recorrido Blender → glTF → cooker → GPU:
base color, normal/tangentes, metallic/roughness, emisión, AO si se necesita,
color lineal/sRGB y empaquetado de canales. Precisar límites de UVs, imágenes,
alpha, huesos/influencias, interpolación, constraints horneadas y morph targets.
No basta que una propiedad exista en el importador.

Conservar las medidas del asset: triángulos, materiales/draws, texturas/residencia,
skinning y tamaño en pantalla, con equipo, resolución, personajes y método.
La primera medición del escenario de ocho combatientes corresponde al hito 102.
Sus resultados detectan regulación de Present y fallo del skinning con siete
Hound v16; no permiten fijar todavía LODs/materiales/mapas. Mantener H06
bloqueada sin certificar 200 FPS/1080p/ocho ni inventar resolución de texturas.

## Entrega y límites
Documento corto de contrato/presupuesto con valores, convenciones, pruebas y
matriz de clips/estados/sockets que exige el runtime. Inventariar las armas
actuales mediante [ARSENAL](../../../ARSENAL.md); no asumir que solo existe Soul Reaper.
Registrar ruta exacta en el índice para H07–H13.

No cambiar motor, gameplay o modelo en esta tarea. Si falta una capacidad,
documentar alternativa compatible o proponer una tarea de implementación separada.
Cierre solo con contrato comprobado y decisiones suficientes para producir,
o estado bloqueado con la decisión precisa. El render estático no valida clips.
