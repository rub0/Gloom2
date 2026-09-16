# Hound — tareas pendientes sin historial de conversación

16 de septiembre de 2026 · Hito 90 (documentación). Alcance: terminar **este Hound**,
desde el pase artístico v08 hasta su integración jugable. No es el backlog del motor.

## Cómo abrir una tarea nueva

Copiar este encargo y cambiar solo el identificador:

> En D:/Projects/Gloom, ejecuta únicamente H01 del índice
> docs/art/hound/tasks/README.md. Lee AGENTS.md, el inicio de
> docs/ESTADO_ACTUAL.md, este índice, CONTEXTO.md y la ficha indicada.
> Usa las entradas registradas en el índice; no necesitas el chat anterior.
> Respeta sus dependencias y exclusiones. Documenta resultados y crea el commit
> local del hito; no hagas push ni empieces la siguiente tarea.

La ficha y el [contexto mínimo](CONTEXTO.md) sustituyen al historial.
Abrir solo las referencias que la ficha necesita: no leer todas las fichas,
todos los informes ni todas las versiones anteriores. Las imágenes sí deben
inspeccionarse cuando se trabaja el aspecto. No crear tareas automáticamente.

## Punto de partida y traspaso vigente

- Base inicial: commit `68e686e`, hito 89, Hound v08.
- Fuente acumulada actual: [hound-mesh-v08.blend](../../../../art/characters/hound/v08/hound-mesh-v08.blend).
- Exportación actual: [hound-rig.gltf](../../../../assets/characters/hound_rig/v08/hound-rig.gltf), junto a su BIN.
- Evidencias: [vistas y vídeo v08](../mesh-v08/README.md).
- Estado artístico: diseño básico v02 aprobado; **v08 propuesta, escultura abierta**.
- Rig: diagnóstico de 53 huesos; no rig ni animaciones finales.
- Contrato/presupuesto H06: pendiente. Malla de producción H07: pendiente.
- Aprobación de cierre artístico H05: pendiente, sin fecha ni aceptación registrada.
- Integración: el juego sigue usando la presentación anterior; v08 no la sustituye.

Al cerrar una ficha, actualizar aquí la fuente acumulada, exportación y evidencia
con **rutas exactas**, incluso cuando no cambien. Registrar en su fila el commit,
el informe y los productos que consumen otras tareas. Nunca deducir la entrada
solo por el número de carpeta más alto: puede contener una propuesta descartada.
El informe de cierre debe resumir decisiones/puntos abiertos en unas pocas líneas;
la siguiente tarea no debe reconstruir la cadena de informes.

## Fichas y dependencias

Los IDs H01–H13 son estables; **no son números de hito ni versiones Blender**.
Los resultados y enlaces se añadirán al realizar cada tarea; ahora no existen.

| ID | Tarea | Requiere | Estado | Entrega / commit |
| --- | --- | --- | --- | --- |
| [H01](H01-manos-guanteletes.md) | Manos, dedos y guanteletes | Base v08 | Pendiente | — |
| [H02](H02-grebas-botas.md) | Grebas, rodillas, tobillos y botas | H01 | Pendiente | — |
| [H03](H03-rostro-brazos.md) | Rostro y anatomía visible | H02 | Pendiente | — |
| [H04](H04-ropa-uniones.md) | Ropa y ensamblaje del conjunto | H03 | Pendiente | — |
| [H05](H05-cierre-artistico.md) | Revisión global y aprobación de escultura | H04 | Pendiente | — |
| [H06](H06-contrato-presupuesto.md) | Contrato del motor y presupuesto medido | Base v08; actualizar con H05 | Pendiente | — |
| [H07](H07-malla-produccion.md) | Retopología, densidad y LODs | H05 aprobada + H06 | Pendiente | — |
| [H08](H08-rig-pesos.md) | Rig, pesos, sockets y agarres definitivos | H07 + H06 | Pendiente | — |
| [H09](H09-uv-horneado.md) | UVs y horneado | H08 + H06 | Pendiente | — |
| [H10](H10-materiales.md) | Texturas y materiales finales | H09 | Pendiente | — |
| [H11](H11-locomocion.md) | Locomoción y transiciones | H08 + H06 | Pendiente | — |
| [H12](H12-combate.md) | Apuntado, armas, habilidades y reacciones | H11 | Pendiente | — |
| [H13](H13-integracion.md) | Integración jugable y aceptación | H10 + H12; LODs/presupuesto vigentes | Pendiente | — |

Orden sencillo: H01 → H02 → H03 → H04 → H05 → H06 → H07 → H08 →
H09 → H10 → H11 → H12 → H13.
H06 puede investigarse antes de terminar el arte; no fija a ciegas sus cifras finales.
H11 no necesita texturas, pero debe partir de la fuente acumulada si H09/H10 ya terminaron.

**Tareas separadas no significa editar el mismo .blend a la vez.** Ejecutar las
mutaciones de forma secuencial; no fusionar binarios ni sobrescribir un trabajo
posterior. Estas fichas no autorizan agentes paralelos, servicios externos o push.

## Estados y puertas de decisión

- Pendiente: no ejecutada; comprobar dependencias antes de empezar.
- En revisión: entrega técnica preparada, pero falta una aceptación indicada.
- Hecha: criterios cumplidos, evidencias y commit local registrados.
- Bloqueada: registrar dato/decisión concreta que falta, sin inventarla.

H05 necesita aceptación visual explícita del usuario. Que Blender/glTF pasen
tests no la sustituye. No cerrar H07–H13 por anticipado ni llamar final a v08.
Si una tarea resulta demasiado grande, cerrar un subhito coherente, anotar
exactamente qué falta y dejarla pendiente/en revisión; no ampliar su alcance.
