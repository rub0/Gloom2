# H06 — contrato de ejecución y presupuesto

Estado al 29/09/2026: **bloqueada por presupuesto de producción; hito 104 ejecutado**.
El [104](../../../../reports/hound-runtime-104/README.md) corrige mapeo y Present,
y comprueba 200 FPS en el escenario diagnóstico: siete Hound más FPS,
281–284 FPS a 1080p, p99 ≤3,854 ms, sin frames >5 ms en las cuatro repeticiones
con salida nula/real. Todas las series: 0/5.760 frames >5 ms. Fuente v16 aprobada
exacta, animación/sombras y 53 skins visibles; Release 7/7 y Debug 4/4.
[Contrato vigente y límites](../H06-contrato-presupuesto.md).
Faltan costes de mapas/clips/armas de producción y margen de residencia:
511,73/512 MiB. No hay presupuesto final ni se habilita H07. Normales del 103
conservadas. Esta ficha H06 no autoriza empezar otra tarea de motor o producción.

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
El 104 corrige sus bloqueos y valida 200 FPS/1080p con el contenido diagnóstico.
No permite fijar todavía LODs/materiales/mapas de producción. Mantener H06
bloqueada sin inventar resolución de texturas ni extrapolar esa prueba.

## Entrega y límites
Documento corto de contrato/presupuesto con valores, convenciones, pruebas y
matriz de clips/estados/sockets que exige el runtime. Inventariar las armas
actuales mediante [ARSENAL](../../../ARSENAL.md); no asumir que solo existe Soul Reaper.
Registrar ruta exacta en el índice para H07–H13.

No cambiar motor, gameplay o modelo en esta tarea. Si falta una capacidad,
documentar alternativa compatible o proponer una tarea de implementación separada.
Cierre solo con contrato comprobado y decisiones suficientes para producir,
o estado bloqueado con la decisión precisa. El render estático no valida clips.
