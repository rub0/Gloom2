# H06 — contrato de ejecución y presupuesto

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

Medir el asset en el escenario de uso acordado: triángulos, materiales/draws,
texturas/residencia, skinning y tamaño en pantalla. Registrar equipo, resolución,
número de personajes y método; fijar presupuesto LOD0/LODs/materiales/mapas
con esos datos. No inventar un objetivo de FPS, número de rivales o resolución
de texturas: usar los del proyecto, o pedir ese dato si no están definidos.

## Entrega y límites
Documento corto de contrato/presupuesto con valores, convenciones, pruebas y
matriz de clips/estados/sockets que exige el runtime. Inventariar las armas
actuales mediante [ARSENAL](../../../ARSENAL.md); no asumir que solo existe Soul Reaper.
Registrar ruta exacta en el índice para H07–H13.

No cambiar motor, gameplay o modelo en esta tarea. Si falta una capacidad,
documentar alternativa compatible o proponer una tarea de implementación separada.
Cierre solo con contrato comprobado y decisiones suficientes para producir,
o estado bloqueado con la decisión precisa. El render estático no valida clips.
