# H06 — contrato de ejecución y presupuesto

Estado al 30/09/2026: **hecha como entrada técnica de H07**, mediante el
[hito 106](../../../../reports/hound-budget-106/README.md).
[Contrato y presupuesto vigentes](../H06-contrato-presupuesto.md): ≤40.000
triángulos TPS, ≤18.000 FPS, siete materiales, mapas hasta 2K, pool Hound
≤192 MiB y residencia de escena objetivo ≤384 MiB sobre techo de 512 MiB.
La prueba con mapas/clips/cinco armas da 296,77–298,76 FPS a 1080p, p99 máximo
3,914 ms, máximo 4,503 ms y 0/1.440 frames >5 ms con audio nulo/real.
355,43 MiB residentes sin evicciones ni recursos ausentes; Release 10/10,
Debug 5/5 y pruebas de materiales/normales BC/RGBA8 pasan. V16 exacta.
H07 habilitada documentalmente, **sin iniciar**. Contenido final, agarres,
acciones, efectos y partida autoritativa se validarán en sus fichas posteriores.

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
El 104 corrigió skinning/Present. El 106 detecta que la presión de residencia
ocultaba parte del escenario: activar la capacidad BC opcional recupera memoria
y geometría. La nueva prueba mide mapas, carga de clips y cinco armas; fija
límites y criterios de repetición en el contrato sin modificar la escultura.

## Entrega y límites
Documento corto de contrato/presupuesto con valores, convenciones, pruebas y
matriz de clips/estados/sockets que exige el runtime. Inventariar las armas
actuales mediante [ARSENAL](../../../ARSENAL.md); no asumir que solo existe Soul Reaper.
Registrar ruta exacta en el índice para H07–H13.

La ficha original excluía cambios de motor/gameplay/modelo. El encargo posterior
de continuar el presupuesto y lo pendiente antes de H07 autorizó la corrección
técnica acotada del 106 y sus medidas. No se amplió gameplay ni se modificó v16.
Esta ficha no autoriza por sí misma más implementación ni comenzar otra ficha.
Cierre solo con contrato comprobado y decisiones suficientes para producir,
o estado bloqueado con la decisión precisa. El render estático no valida clips.
