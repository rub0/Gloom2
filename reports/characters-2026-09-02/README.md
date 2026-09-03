# Hito 62: personajes y materiales originales

**35/35 pruebas Debug superadas.** [Log completo](tests-final.log).

Archangel y Shadow se recuperan desde las mallas originales, con sus esqueletos
y pesos. Soul Reaper original se presenta en primera y tercera persona. Hound
comparte el cuerpo de Archangel; no se inventa una malla nueva para Berserker.

## Comparación con los bocetos

![Bocetos y capturas de ambos personajes](concept-comparison.png)

Los paneles del motor son recortes de capturas reales en pose de reposo, con
dos luces de inspección. No son imágenes generadas ni retocadas. Las luces se
usan exclusivamente para esta revisión; el juego conserva las de Factory.

- **Archangel:** armadura dorada metálica; alas, hombros y detalles cian tienen
  emisión en ese mismo color. Se conserva el desgaste de las texturas originales.
- **Shadow:** armadura metálica oscura y ojos emisivos rojos. La cola sigue
  siendo la geometría original; humo y estelas se completarán con partículas.
- **Soul Reaper:** malla y mapas originales con UV corregidas, escala FPS
  independiente y anclaje de mano para el otro jugador.

![Siete vistas de aceptación](capture-contact.png)

## Comprobaciones

- [Importación reproducible](import-verification.json): 20 archivos idénticos
  y 17 fuentes verificadas. Copia legacy conservada sin cambios.
- Rigs: 43 huesos de Archangel y 17 de Shadow, con sus cuatro/cinco influencias
  máximas intactas tras cocinar. Matrices y pesos inválidos son rechazados.
- Se corrigieron la inversión vertical de Shadow y el desplazamiento de su
  esqueleto. La modificación queda registrada en el manifiesto y ADR 0063.
- Dos clientes reales GNS seleccionan Archangel y Shadow, se mueven en Factory
  y reciben correctamente el personaje del otro.
- Siete referencias nuevas de Vulkan complementan las nueve del hito 58 y
  seis del 61. Los logs se comprueban para rechazar errores de validación.
  [Comparación automática](captures/comparison.txt) · [Log de Vulkan](captures/render.log).

El formato cocinado es 4 y el protocolo 13. Los IDs anteriores se conservan;
`berserker-reaper` es una opción de compatibilidad presentada como Hound.

**Siguiente: hito 63.** Esta entrega no reproduce animación dinámica: conserva
y presenta la pose de reposo. Quedan evaluación de clips, skinning GPU, agarres
animados y brazos FPS dedicados, movimiento de alas/cola, partículas, humo y
estelas. Archangel conserva cuatro clips inventariados; Shadow no tiene clips
en su archivo original. Habilidades nuevas y Screamer tampoco se han añadido.

[Documentación y comandos](../../docs/CHARACTERS.md) ·
[Decisión técnica](../../docs/architecture/0063-original-character-bind-rigs.md)
