# Hound — tareas pendientes sin historial de conversación

22 de septiembre de 2026 · Índice del hito 90, actualizado con armadura envolvente H05/hito 99. Alcance: terminar **este Hound**,
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
- Fuente acumulada actual: [hound-mesh-v16.blend](../../../../art/characters/hound/v16/hound-mesh-v16.blend).
- Exportación actual: [hound-rig.gltf](../../../../assets/characters/hound_rig/v16/hound-rig.gltf), junto a su BIN.
- Evidencias: [v16, cobertura de armadura y vídeo](../mesh-v16/README.md).
- Estado artístico: diseño básico v02 aprobado; capucha v15 aceptada; **H05 en revisión con coraza envolvente v16**.
- Rig: 53 huesos, bind pose y claves conservados; ajuste provisional de una placa lumbar al hueso de su banda lateral.
- Contrato/presupuesto H06 y malla de producción H07: pendientes.
- Aprobación de cierre artístico H05: **pendiente de aceptación visual explícita de v16**.
- Dirección aplicada: F01–F06, caída de capucha y cobertura de armadura delantera/posterior/lateral.
- Integración: el juego sigue usando la presentación anterior; v16 no la sustituye.

Al cerrar una ficha, actualizar aquí la fuente acumulada, exportación y evidencia
con **rutas exactas**, incluso cuando no cambien. Registrar en su fila el commit,
el informe y los productos que consumen otras tareas. Nunca deducir la entrada
solo por el número de carpeta más alto: puede contener una propuesta descartada.
El informe de cierre debe resumir decisiones/puntos abiertos en unas pocas líneas;
la siguiente tarea no debe reconstruir la cadena de informes.

## Traspaso H01 — hito 91

Manos/guanteletes v09: carcasas y aros huecos, falanges dorsales articuladas,
guantes ajustados y extremos de placas frontales liberados. Pico dorsal,
60 componentes ajenos y rig diagnóstico exactos. Validación local y roundtrip pasan.
Escena `Hound_Mesh_v09`, malla `H09_DeformMesh`, rig `Hound09_Rig`,
acción `Hound09_joint_check`, colección `HOUND_v09_EXPORT`.
El Soul Reaper solo tiene un apoyo diagnóstico sobre su carcasa posterior;
H08 debe resolver cierre palmar, pulgar, mano izquierda y socket de apuntado.
No interpretar el apoyo como empuñadura final. H02 conserva exactamente estas manos.

## Traspaso H02 — hito 92

Grebas huecas con lengüeta de rodillera; placas ajustadas al soporte y botas
con suela y cavidad de tobillo. Ajuste del forro solo bajo Z=0,61 m.
14 componentes reconstruidos, uno ajustado y 81 exactos, incluidas las manos.
Mismos 53 huesos, pesos y claves. 17 poses locales/1.785 pares sin cruces,
rodilla hasta 70°; no certifica flexión profunda o torsión lateral.
Roundtrip, cooker/visor y tres pruebas CTest pasan.
Escena `Hound_Mesh_v10`, malla `H10_DeformMesh`, rig `Hound10_Rig`,
acción `Hound10_joint_check`, colección `HOUND_v10_EXPORT`.
Fuente/exportación/evidencia de H02 en su fila; las rutas del inicio indican la fuente acumulada vigente.
[Informe y límites de H02](../../../../reports/hound-legs-92/README.md).
H05 conserva la aprobación global; H08/H11 resolverán rig y locomoción.
Al cerrar H02, H03 no se había iniciado; su entrega se registra debajo.

## Traspaso H03 — hito 93

Rostro humano severo con órbitas, pómulos, nariz, labios y mentón refinados;
ojos pequeños ajustados, sin cuñas. Deltoides/bíceps/tríceps/antebrazo integrados.
Cuatro componentes reconstruidos, dos ajustados y 90 exactos, incluido cuello,
capucha, clavículas, manos H01 y piernas H02. Rig, pesos y claves conservados.
35.490 vértices/70.624 triángulos (+27,1 %): autoría, sin presupuesto final.
25 poses: 100 pares cara/ojos/boca contra capucha sin cruces; volumen de brazos
≥96,825 %. Persisten pinzamientos heredados al doblar mucho/elevar brazos
y ensamblajes solapados cabeza/cuello/capucha; detalles y ángulos en el informe.
Roundtrip, regresión v10, cooker/visor y tres pruebas CTest pasan.
Escena `Hound_Mesh_v11`, malla `H11_DeformMesh`, rig `Hound11_Rig`,
acción `Hound11_joint_check`, colección `HOUND_v11_EXPORT`.
[Informe y límites de H03](../../../../reports/hound-anatomy-93/README.md).
H06/H08 decidirán si hace falta mandíbula animada; Bite no la presupone.
H05 conserva la aprobación global. Al cerrar H03, H04 aún no se había iniciado.

## Traspaso H04 — hito 94

Ropa y ensamblaje v12: pliegues amplios del pantalón, faja y paños de 3 mm,
retornos de 4 mm en doce placas del torso y soporte continuo bajo las láminas.
Dos apoyos mediales conectan pecho, clavículas y espalda. Se corrige el asiento
de seis piezas claviculares que entraban en los brazos, conservando las puntas.
Ocho componentes reconstruidos, quince ajustados, dos añadidos y 73 exactos;
incluye cara, manos, capucha/abertura, cuello, brazos y armadura de piernas.
Los 53 huesos, bind pose y claves siguen intactos.

36.466 vértices / 72.568 triángulos (+2,75 %). Fuente reabierta, 61 muestras,
37 poses y 444 pares locales por pose. Sin cruces en bordes libres de paños/
pantalón ni brazos/asientos claviculares. Los solapes constructivos ocultos
y los límites del rig provisional están enumerados en el informe.
Roundtrip, regresión v11, cooker/visor y CTest 3/3 pasan.

Escena `Hound_Mesh_v12`, malla `H12_DeformMesh`, rig `Hound12_Rig`,
acción `Hound12_joint_check`, colección `HOUND_v12_EXPORT`.
[Informe y contactos H04](../../../../reports/hound-cloth-94/README.md).
Fuente/exportación/evidencias vigentes en el inicio y en su fila.
Al cerrar H04, H05 no estaba iniciada; H04 no implica aprobación global de escultura.

## Traspaso H05 — subhito 95, en revisión

Candidata inmutable v13, copia exacta de v12; no se cambia geometría, rig,
bind pose, pesos ni claves. Maestra candidata para futura H07/H09:
`art/characters/hound/v13/hound-sculpture-v13.blend`.
Exportación: `assets/characters/hound_rig/v13/hound-rig.gltf` y `hound-rig.bin`.
Integridad: `art/characters/hound/v13/sculpture-reference.json`.
Evidencias: `docs/art/hound/review-h05/README.md`.

Conserva los nombres internos v12: escena `Hound_Mesh_v12`, malla
`H12_DeformMesh`, rig `Hound12_Rig`, acción `Hound12_joint_check`,
colección `HOUND_v12_EXPORT`. 36.466 vértices / 72.568 triángulos, 98 componentes,
87 rígidos, siete materiales y 53 huesos diagnósticos; no es presupuesto final.

Comparación con concept/boceto/v02, cuatro vistas a igual escala, detalles,
siluetas de 200/100/50 px y vídeo de 331 frames. Fuente reabierta, topología/
pesos y roundtrip pasan (0,000002069 m máximo); regresión v12 y CTest 3/3.
Auditoría global: 33 poses, 156.849 evaluaciones de pares del cuerpo.
[Informe autocontenido y contactos](../../../../reports/hound-review-95/README.md).

Límites que deberán resolver malla/rig antes de UVs: pinzamiento de codos a
85°, cabeza contra torso al mirar abajo, inserciones de falanges/guante y
superficies de brazo solapadas bajo armadura. El apoyo Soul Reaper de H01
falla contra brazo/guantelete/clavícula al revisar el cuerpo entero, aunque
guante/dedos estén libres; no es agarre ni socket válido. No garantiza
apoyos de pies, estabilidad física ni todas las poses/contactos.

**V13 no recibió aceptación; fue sustituida por la candidata v14 del hito 97**.
Fecha y texto: pendientes. H05 no está hecha ni habilita H07.
Subhito de revisión con commit local `3e8bfd8`.
No se inicia H06 ni otra ficha; no se hace push.

## Preparación de correcciones H05 — hito 96

[Especificación F01–F06 y encargo reutilizable](H05-feedback-2026-09-21.md).
Feedback del usuario: tres pinchos por antebrazo; garras; armadura continua
pie/rodilla con remate algo superior; un pincho posterior por lado; arranque
clavicular/peto más achatado; abdominales más naturales e integrados.
En el hito 96 se preparó el documento sin cambiar geometría. Su ejecución posterior está registrada en el hito 97.

Al cerrar el hito 96 se conservaban fuente/exportación/evidencias v13, con nombres
internos v12. V14 aún no existía entonces; el hito 97 la crea y la registra como fuente vigente.
Resolver conjuntamente pecho/abdomen/espalda, después antebrazos/garras y piernas.
Los detalles de interpretación, comprobación y alcance están en la especificación.

El encargo posterior ejecuta el feedback en el hito 97. H05 sigue en revisión
y no habilita H07. No se inicia H06 ni otra ficha.
[Informe 96](../../../../reports/hound-feedback-96/README.md).
Commit local `ece90a7`.

## Correcciones H05 — hito 97, candidata v14

F01–F06 aplicados: tres pinchos por antebrazo, garras, continuidad pie/rodilla,
dos filos posteriores, asiento clavicular bajo con peto achatado, abdomen
curvo y flancos articulados. [Galería v14](../mesh-v14/README.md) e
[informe autocontenido](../../../../reports/hound-feedback-97/README.md).

Fuente/glTF/BIN de esta revisión histórica: [v14](../mesh-v14/README.md). Rutas vigentes al inicio.
Escena `Hound_Mesh_v14`, malla `H14_DeformMesh`, rig `Hound14_Rig`,
acción `Hound14_joint_check`, colección `HOUND_v14_EXPORT`.
[Manifiesto SHA-256](../../../../art/characters/hound/v14/sculpture-reference.json).
38.162 vértices / 75.928 triángulos (+4,63 %), 106 componentes, 95 rígidos.
32 reconstruidos, cuatro ajustados, ocho añadidos y 62 ajenos exactos.
V13 intacta; mismos 53 huesos, bind pose y claves.

Topología/pesos, 61 muestras, 33 poses/183.645 pares evaluados, roundtrip,
regresión v13, cooker/visor y CTest 3/3 pasan. Vídeo de 331 frames y 37 PNG.
Sin nuevas autointersecciones; siguen codos a 85°, cabeza/torso al bajar,
inserciones ocultas y contactos entre láminas al girar el torso.
El apoyo Soul Reaper sigue inválido; las garras nuevas añaden contacto con
la carcasa. H08 deberá resolver holguras, pesos y agarre antes de UVs.
[Inventario](../../../../reports/hound-feedback-97/contacts.md) y
[evolución frente a v13](../../../../reports/hound-feedback-97/contact-changes.md).

**V14 se entregó sin aceptación artística**; las candidatas posteriores siguen sin aprobación global.
H05 no está hecha ni habilita H07. H06 no se ha iniciado.
Commit local `4c1f86c`.
No hay push ni trabajo de otra ficha.

## Ajuste H05 — hito 98, candidata v15

Capucha abierta sobre el centro del pecho, con dos bordes descendentes.
Nuca y marco superior conservados; 105 piezas ajenas, rig y claves exactos.
[Galería v15](../mesh-v15/README.md) e [informe 98](../../../../reports/hound-hood-98/README.md).
Fuente/glTF/BIN históricos: [v15](../mesh-v15/README.md); [manifiesto](../../../../art/characters/hound/v15/sculpture-reference.json).
Escena `Hound_Mesh_v15`, malla `H15_DeformMesh`, rig `Hound15_Rig`,
acción `Hound15_joint_check`, colección `HOUND_v15_EXPORT`.
38.085 vértices / 75.774 triángulos, 106 componentes y 95 rígidos.
V13/v14 intactas. 61 muestras, 33 poses × 105 pares de capucha, roundtrip,
regresión v14, cooker/visor y CTest 3/3 pasan. Vídeo de 211 frames y 16 PNG.
Sin autointersecciones de capucha ni contactos nuevos con piezas antes libres;
persisten inserciones del forro y los límites de codo, torso, láminas y arma de v14.
[Contactos de capucha](../../../../reports/hound-hood-98/contacts.md).
El rig provisional aún dobla la tela con fuerza al bajar la cabeza.
El usuario acepta la caída de capucha con **«mucho mejor!»**, el 22 de septiembre.
Aceptación específica de esa forma; la armadura continúa en revisión.
H05 no está hecha ni habilita H07. H06 no iniciado.
Commit local `84a47d9`.
Sin push ni trabajo de otra ficha.

## Ajuste H05 — hito 99, candidata v16

El usuario pide metal exterior por delante, detrás y laterales, permitiendo
malla interior en algunas zonas. V16 amplía dos placas escapulares y tres
lumbares y reconstruye seis bandas que envuelven el torso.
95 piezas ajenas exactas; capucha aceptada, peto/abdomen central y extremidades
conservados. Carcasas de antebrazo y greba ya envolventes. Axilas/hombros libres.
[Galería](../mesh-v16/README.md) · [Informe 99](../../../../reports/hound-armor-99/README.md).
Fuente/exportación vigentes al inicio; [manifiesto](../../../../art/characters/hound/v16/sculpture-reference.json).
Escena `Hound_Mesh_v16`, malla `H16_DeformMesh`, rig `Hound16_Rig`,
acción `Hound16_joint_check`, colección `HOUND_v16_EXPORT`.
40.274 vértices / 80.152 triángulos, 106 componentes, 95 rígidos.
Mismos 53 huesos, bind pose y claves; `Back_spine_1` sigue ahora `Bip001 Spine1`
para acompañar a su banda lateral. V13/v14/v15 intactas.
61 muestras, 33 poses globales/183.645 pares, roundtrip, regresiones v15,
cooker/visor y CTest 3/3 pasan. Vídeo de 331 frames y comparativa de cobertura.
Sin autointersecciones nuevas ni cruces de placas reconstruidas con cabeza,
cuello, capucha, brazos o manos. Persisten inserciones y límites heredados
de codo, torso, abdomen y arma; [contactos](../../../../reports/hound-armor-99/contacts.md).
**H05 sigue en revisión: falta aceptación artística explícita de v16**.
La aceptación de la capucha no cierra H05. H06 no iniciado; no habilita H07.
Commit local del hito 99; resolver con `git log --oneline --grep='^hito 99:'`.
Sin push ni trabajo de otra ficha.

## Fichas y dependencias

Los IDs H01–H13 son estables; **no son números de hito ni versiones Blender**.
Los resultados y enlaces se registran al realizar cada tarea; H01 entrega v09, H02 entrega v10, H03 entrega v11 y H04 entrega v12.

| ID | Tarea | Requiere | Estado | Entrega / commit |
| --- | --- | --- | --- | --- |
| [H01](H01-manos-guanteletes.md) | Manos, dedos y guanteletes | Base v08 | Hecha | [v09: fuente, glTF/BIN y vistas](../mesh-v09/README.md); [informe 91](../../../../reports/hound-hands-91/README.md); commit `8bc8060` |
| [H02](H02-grebas-botas.md) | Grebas, rodillas, tobillos y botas | H01 | Hecha | [v10: fuente, glTF/BIN y vistas](../mesh-v10/README.md); [informe 92](../../../../reports/hound-legs-92/README.md); commit `6c2fe7f` |
| [H03](H03-rostro-brazos.md) | Rostro y anatomía visible | H02 | Hecha | [v11: fuente, glTF/BIN y vistas](../mesh-v11/README.md); [informe 93](../../../../reports/hound-anatomy-93/README.md); commit `de11e03` |
| [H04](H04-ropa-uniones.md) | Ropa y ensamblaje del conjunto | H03 | Hecha | [v12: fuente, glTF/BIN y vistas](../mesh-v12/README.md); [informe 94](../../../../reports/hound-cloth-94/README.md); commit `0643723` |
| [H05](H05-cierre-artistico.md) | Revisión global y aprobación de escultura | H04 | En revisión; capucha aceptada, coraza envolvente pendiente | [v16 y evidencias](../mesh-v16/README.md); [informe 99](../../../../reports/hound-armor-99/README.md); commit del hito 99, resolver con git log; v13–v15 preservadas |
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
