# Hito 78 — guía artística y primer boceto de Hound

9 de septiembre de 2026. Entrega de documentación y concept 2D; aceptación
artística pendiente del usuario. No se entrega una malla ni se modifica el juego.

Actualización posterior (hito 79, 10 de septiembre): el usuario ha aprobado la
guía y la lámina con «asi esta perfecto, continua con lo siguiente». Las notas
de revisión de este informe describen el estado de la entrega original, no una
aprobación pendiente actual. Continúa el [flujo Blender](../../docs/BLENDER_WORKFLOW.md).

## Resultado

- [Guía artística 0.1](../../docs/DIRECCION_ARTISTICA.md): jerarquía de referencias,
  formas, materiales, paleta provisional, aplicación por familia de assets y
  validaciones previas a producción.
- [Boceto Hound 01](../../docs/art/hound/hound-concept-v01.png): lámina de 1536 × 1024
  con cuerpo completo, estudio de cabeza, guanteletes y muestras de materiales.
- [Prompt exacto](../../docs/art/hound/prompt-v01.md), conservado para iteraciones.

![Hound — propuesta 01](../../docs/art/hound/hound-concept-v01.png)

## Procedencia y decisiones

El usuario eligió `concept_hound2.jpg` de `D:\Descargas\Gloom` y encargó la guía
y el primer boceto. Se usa como única referencia de imagen enviada a la generación
integrada (`imagegen`), en una llamada. No se utiliza un servicio externo de 3D,
una API con clave propia ni Blender. El generador entrega una interpretación;
no reproduce una malla, un rig ni un render del motor.

El diseño conserva Hound bípedo, capucha, piel pálida, ojos naranja, torso por
placas y filos en hombreras/guanteletes. La guía aplica las preferencias UT3 y
toque leve de Overwatch 2 como peso material y claridad, subordinadas al original.
El documento distingue rasgos elegidos de cambios todavía propuestos.

Las referencias de Archangel/Shadow ya estaban versionadas. Se añaden copias
intactas del Hound elegido, la variante de rostro de Screamer y la lámina de armas.
No se selecciona por ello el diseño final de Screamer ni se añade al roster.
Autor original no identificado por nombre; firmas preservadas en las fuentes.

| Copia | SHA-256 |
| --- | --- |
| `hound-original-concept.jpg` | `90422ca01d234719597b7ffdf439ec546b7070004d9b532098f96a452d3c7962` |
| `screamer-original-reference.jpg` | `09531e354041b4a8c607afb49bab12f84788426fcf4bdf29a7d99222929c288a` |
| `weapons-original-reference.jpg` | `574d56d4cf7d173e118ae24d443c1dccae53d58f029430f9d4e50d2b44c2522b` |
| `hound-concept-v01.png` (generado) | `dfab3558b678209849481dec1edd77481817dc864342a4e77cebfebeb5bccd4e` |

## Revisión visual

La lámina muestra ambos pies, pose legible y manos separadas del torso. El rostro
y los materiales se pueden comparar con la referencia sin iluminación de fuego.
Los estudios son útiles para discutir el diseño; no están comprobados como piezas
ensamblables ni como planos ortográficos.

Puntos a revisar con el usuario:

- La piel presenta marcas y facetas más pétreas que en el original; suavizarlas
  si se desea una apariencia más orgánica.
- La armadura y los guanteletes tienen mucho peso visual. Valorar si conservarlo
  o reducir placas secundarias sin perder los filos característicos.
- Los dedos, uniones y placas de piernas son interpretaciones. Antes de producción
  hay que comprobar agarre de armas, movilidad de hombros/codos y perfil/espalda.
- El matiz estilizado es contenido: no se cambia la anatomía ni se introduce una
  paleta viva. La valoración de si alcanza el equilibrio buscado es del usuario.

## Validación y alcance

Comprobaciones de entrega: hashes de las tres copias contra sus fuentes, lectura
del PNG, enlaces relativos locales de los documentos modificados y `git diff --check`.
Revisión visual de la imagen generada frente al original. Workspace inicialmente
limpio en `2c83268`; ninguna modificación de código, assets del runtime o build.
No se ejecuta CTest: no hay cambio ejecutable que validar en este hito.

Los medios viven en `docs/art`, ya que `.gitignore` excluye medios bajo `reports`.
Las imágenes son referencias de producción, no recursos cargados por el juego.
El rendimiento no cambia; el presupuesto de la futura malla se decidirá con
medición en Gloom. No se presupone que el rig de Archangel encaje en el diseño.

Siguiente paso: aprobación o corrección de guía/boceto. Después comprobar Blender
MCP, exportación GLB y cooker con una escena pequeña, y pasar al modelo básico.
