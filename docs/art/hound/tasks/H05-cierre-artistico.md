# H05 — revisión global y aceptación de la escultura

## Entrada y alcance
Leer [contexto](CONTEXTO.md) e [índice](README.md); requiere H04.
Comparar la fuente acumulada con concept, boceto y v02 aprobados.
Esta tarea cierra **formas**, no UVs, materiales, animaciones o rendimiento final.

Revisar silueta, proporciones, jerarquía de detalle, anatomía, construcción y
coherencia de espalda/perfil. Corregir remates pequeños; si aparece un rediseño
sustancial, documentarlo y pedir dirección antes de cambiarlo.

## Feedback vigente para la siguiente revisión

El feedback del 21 de septiembre está en la [especificación F01–F06](H05-feedback-2026-09-21.md).
El hito 96 lo preparó y el hito 97 lo aplica en **v14**: antebrazos, garras,
piernas, espalda, clavícula/peto y abdomen. V13 queda conservada.
[Entrega v14](../mesh-v14/README.md) e [informe 97](../../../../reports/hound-feedback-97/README.md).
El feedback adicional del 22 de septiembre pide que la capucha caiga como en el
concept, sin cerrar sobre el pecho. El hito 98 lo aplica en **v15**: dos bordes
descendentes, centro abierto y nuca cubierta. Las otras 105 piezas y rig se conservan.
[Entrega v15](../mesh-v15/README.md) e [informe 98](../../../../reports/hound-hood-98/README.md).
Esta indicación posterior autoriza modificar la capucha conservada por F01–F06.
**H05 sigue en revisión: falta aceptación visual explícita de v15.**
Las pruebas técnicas no aprueban la escultura. H06 no se ha iniciado.

## Evidencias y criterios
- Lámina frontal/perfil/espalda/tres cuartos a igual escala; silueta a tamaño
  de combate; detalles de cara, clavículas, manos, botas y uniones.
- Vídeo diagnóstico con alcance, torso, piernas, dedos y cabeza. Posar con arma
  existente; revisar contactos globales y estabilidad, sin afirmar cobertura total.
- Reabrir fuente final, comprobar topología/pesos y roundtrip; señalar toda
  limitación pendiente que pueda afectar a retopología, UVs o rig.
- Presentar renders reales al usuario y solicitar aceptación **explícita**
  de la escultura. No confundir v02 aprobada o “continúa” con esta aceptación.

## Entrega y puerta
Guardar fuente de escultura de referencia inmutable, exportación y lámina/vídeo.
Registrar en el índice la ruta maestra para H07/H09 y, cuando exista, fecha y
texto breve de aprobación. Si falta, estado **en revisión**: se puede hacer
commit del subhito de revisión, pero H05 no está hecha ni habilita H07.
H06 puede investigar su contrato mientras tanto.

No empezar rig/texturas definitivos dentro de esta ficha.
Aplicar el cierre común y dejar un resumen suficiente para no releer H01–H04.
