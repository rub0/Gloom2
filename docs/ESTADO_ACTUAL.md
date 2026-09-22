# Traspaso de Gloom

Actualizado: 22 de septiembre de 2026, hito 99, armadura envolvente H05/v16.

**Para una tarea nueva de Hound:** leer este inicio, AGENTS.md y el
[índice de tareas](art/hound/tasks/README.md); después, solo su CONTEXTO.md
y la ficha elegida. No cargar el historial del chat ni todos los hitos de abajo.
H01–H04 hechas. **H05 en revisión: armadura de espalda y costados ampliada en v16,
pendiente de aceptación visual explícita**.
[Galería y comparativas](art/hound/mesh-v16/README.md) ·
[Informe 99](../reports/hound-armor-99/README.md).
H06 no se ha iniciado. No ejecutar otra ficha sin encargo.

Fuente acumulada: `art/characters/hound/v16/hound-mesh-v16.blend`.
Escena `Hound_Mesh_v16`, malla `H16_DeformMesh`, rig `Hound16_Rig`,
acción `Hound16_joint_check`, colección `HOUND_v16_EXPORT`.
Exportación: `assets/characters/hound_rig/v16/hound-rig.gltf` y BIN.
V13/v14/v15 y sus manifiestos permanecen exactos; conservar las fuentes anteriores.
La caída de capucha v15 se acepta con «mucho mejor!» y queda exacta en v16.
Es aprobación específica de la capucha, no cierre artístico de H05.

Cambio H05/99: dos placas escapulares y tres lumbares más amplias, seis bandas
laterales que envuelven el torso. Once piezas reconstruidas; 95 ajenas exactas,
incluidos capucha, peto, abdominales centrales, garras, carcasas de antebrazos y grebas.
Los hombros/axilas siguen libres; quedan separaciones entre placas.
53 huesos, bind pose y claves intactos. Solo la nueva placa `Back_spine_1`
pasa a `Bip001 Spine1` para acompañar su banda lateral sin cruzarla al girar.
40.274 vértices / 80.152 triángulos; cifras de autoría, sin presupuesto H06.

Topología/pesos, 61 muestras, 33 poses globales/183.645 pares evaluados,
roundtrip en 13 poses, regresiones v15, cooker/visor y CTest 3/3 pasan.
Sin autointersecciones nuevas ni cruces de placas reconstruidas contra brazos,
cabeza, cuello, capucha o manos. Se mantienen inserciones constructivas entre placas.
[Contactos y cobertura](../reports/hound-armor-99/contacts.md).
Siguen los límites heredados de codos, cabeza/torso, abdomen, guantes y apoyo
Soul Reaper inválido; las holguras/pesos y agarre deben resolverse antes de UVs.
El rig provisional aún dobla la capucha con fuerza al bajar la cabeza.
Commit local del hito 99; resolver con `git log --oneline --grep='^hito 99:'`.

Sigue pendiente revisar la partida en Release, medir combate/HUD/audio y reducir memoria residente.
Actualizar este documento al cerrar cada hito; guardar el detalle en informes
y crear el commit de cierre según `AGENTS.md`.

## Estado y cuidado del workspace

- Proyecto: `D:\Projects\Gloom`. Original: `D:\Projects\Gloom-Legacy`.
- El commit de cierre del 68 consolida también la definición previa de su alcance
  y todos los recursos originales que necesita esta versión. Consultar `git log -1`
  para su hash.
  Revisar `git status` antes de editar y conservar cualquier cambio nuevo;
  no usar reset, checkout ni clean para limpiar. Commit local y push son acciones
  distintas: no asumir que el último hito ya está subido a GitHub.
- Instrucción permanente del usuario: **crear un commit al terminar cada hito**,
  después de validar y actualizar documentación. Informar del hash al cerrar.
- Hitos 63–64: animación, skinning, FPS/TPS, partículas e interfaz implementados.
  65–66: física original y arsenal implementados. **67: recogibles implementados**.
- El usuario confirmó que las últimas correcciones de movimiento funcionan.
  No reabrir esa física ni los hitos anteriores salvo regresión concreta.

## Decisiones aceptadas que deben conservarse

- Momento con retención terrestre `pow(0.8, dt/0.016)` al soltar y aérea
  `pow(0.98, dt/0.016)`. No poner a cero la velocidad al soltar o aterrizar.
- Salto normal: 90 % del impulso original. Esquiva sin reducción: doble WASD
  o doble Espacio en 450 ms, con dirección; una esquiva aérea por salto.
- Jolt: recuperar apoyo con `RefreshContacts` antes de `ExtendedUpdate`.
  Las superficies transitables no restan momento horizontal; paredes/techo
  cancelan impulso hacia el obstáculo. Sin empuje descendente constante en suelo.
- Vida/escudo: barras verticales exteriores al marco de armas, símbolos
  originales. Cooldown en círculo lateral. FPS medidos y XYZ arriba a la izquierda.
- Lava letal al contacto incluso con escudo. Relleno ambiente 0,65 y luces +20 %.
- Protocolo actual **22**; replica habilidad primaria/secundaria, cooldown, estado,
  factor de Flash y racha de bajas. Documentos históricos que indican protocolos 15–21 describen entregas
  previas.

## Último subhito entregado: 99, armadura envolvente H05

V16 reconstruye once placas de espalda y costados; conserva 95 piezas ajenas,
incluida la capucha aceptada. Peso provisional lumbar corregido; rig y claves
exactos. [Informe 99](../reports/hound-armor-99/README.md).
Fuente, validación y límites al inicio. H05 en revisión; H06 no iniciado.
Commit local del hito 99.

## Subhito 98, caída de capucha H05

V15 abre el frente en dos extremos descendentes; conserva nuca, rostro y
las otras 105 piezas. [Informe 98](../reports/hound-hood-98/README.md).
Fuente, validación y límites resumidos al inicio. H05 sigue en revisión,
sin aceptación artística ni inicio de H06. Commit local del hito 98.

## Subhito 97, feedback artístico H05 aplicado

F01–F06 modelados en v14; [informe 97](../reports/hound-feedback-97/README.md).
Commit `4c1f86c`. La fuente vigente v15 conserva esas seis correcciones
y cambia únicamente la caída de capucha.

## Subhito 96, preparación del feedback H05

[Especificación lista para ejecutar F01–F06](art/hound/tasks/H05-feedback-2026-09-21.md).
Tres pinchos por antebrazo, lectura de garras, continuidad de armadura de pie
hasta algo por encima de rodilla, un pincho posterior por lado, corrección
clavícula/peto y abdominales más naturales.
Distingue indicaciones del usuario de interpretaciones de modelado y fija
criterios visuales, orden, conservación, evidencias y comprobaciones.

Solo documentación: fuente/exportación v13 y sus hashes intactos, sin v14 creada.
Nombres internos siguen siendo v12. Entrada exacta y rutas al inicio/índice.
V14 estaba libre, pero debe comprobarse de nuevo al comenzar la modificación.
Enlaces, consistencia, integridad y diff verificados; sin Blender/build/tests del motor.
Informe: `reports/hound-feedback-96/README.md`.
Commit local del hito 96; resolver con `git log --oneline --grep='^hito 96:'`.

H05 continúa en revisión con correcciones pendientes; no se solicita aprobación
de v13 ignorando el feedback. Siguiente: ejecutar F01–F06 cuando se encargue,
presentar una nueva candidata y solicitar aceptación explícita de esas formas.
Sin push y sin inicio de H06/otra ficha.

## Subhito 95, entrega de revisión H05

[H05: lámina global, detalles, siluetas, fuente y vídeo](art/hound/review-h05/README.md).
Candidata v13 idéntica byte a byte a v12; tres hashes SHA-256 comprobados.
No hay cambios de geometría, huesos, bind pose, pesos, claves o gameplay.
36.466 vértices / 72.568 triángulos; 98 componentes, 87 rígidos, siete materiales,
53 huesos diagnósticos, máximo dos influencias. Densidad de autoría.

Fuente reabierta: 98 superficies conexas/cerradas/orientadas, pesos y
61 muestras diagnósticas pasan. Auditoría global: 33 poses independientes,
156.849 evaluaciones de pares del cuerpo y 196 del arma. Hay cruces medidos:
codos a 85°, cabeza/torso al mirar abajo, inserciones bajo armadura/guantes
y montaje global del Soul Reaper inválido. No certifica ausencia de colisiones,
agarres, apoyos de pies ni estabilidad física.

Roundtrip en 13 poses: máximo 0,000002069 m; regresión v12 pasa.
Cooker/visor Vulkan estático 7/7; CTest Release assets/gpu_assets 2/2 y
Debug animation_vfx 1/1. 26 PNG y vídeo de 331 frames revisados.

Fuente: `art/characters/hound/v13/hound-sculpture-v13.blend`.
Exportación: `assets/characters/hound_rig/v13/hound-rig.gltf` y BIN.
Nombres internos preservados: `Hound_Mesh_v12`, `H12_DeformMesh`,
`Hound12_Rig`, `Hound12_joint_check`, `HOUND_v12_EXPORT`.
Informe y contactos: `reports/hound-review-95/README.md`.
Commit local del subhito 95: `3e8bfd8`.

**H05 en revisión**, falta aceptación explícita de formas; fecha/texto pendientes.
El feedback posterior está preparado en el hito 96; la nueva candidata sigue pendiente.
No se habilita H07 ni se inicia H06/otra tarea. Sin push.

## Hito 94, H04 ropa y ensamblaje

[V12: comparativas, poses, fuente y vídeo](art/hound/mesh-v12/README.md).
Pliegues amplios del pantalón, faja y paños de 3 mm; retornos de 4 mm en doce
placas del torso, respaldo integrado y dos apoyos mediales pecho/espalda.
Asiento local de seis piezas claviculares corregido contra los brazos.
Capucha/abertura, cuello, rostro, brazos, manos y armadura de piernas exactos.
Ocho componentes reconstruidos, quince ajustados, dos añadidos y 73 exactos.
Los 53 huesos, bind pose y claves diagnósticas permanecen iguales.

36.466 vértices / 72.568 triángulos (+2,75 %), 98 componentes, 87 rígidos,
siete materiales. Densidad de autoría; presupuesto y malla final pendientes.
Fuente reabierta, geometría/pesos y 61 muestras del clip pasan.
37 poses y 16.428 evaluaciones de contactos: bordes libres de paños/pantalón
y brazos/asientos claviculares sin cruces; uniones ocultas documentadas.
No certifica colisiones globales, movimientos combinados ni rig definitivo.

Roundtrip en 13 poses: máximo 0,000002069 m; regresión v11 pasa.
Cooker/visor Vulkan estático 7/7; CTest Release assets/gpu_assets 2/2 y
Debug animation_vfx 1/1. 22 PNG y vídeo de 361 frames inspeccionados.

Fuente: `art/characters/hound/v12/hound-mesh-v12.blend`.
Exportación: `assets/characters/hound_rig/v12/hound-rig.gltf` y BIN.
Escena `Hound_Mesh_v12`, malla `H12_DeformMesh`, rig `Hound12_Rig`,
acción `Hound12_joint_check`, colección `HOUND_v12_EXPORT`.
Informe: `reports/hound-cloth-94/README.md`. Commit local del hito 94, sin push;
resolver con `git log --oneline --grep='^hito 94:'`.
H04 hecha como propuesta. Al cerrar H04, H05 no estaba iniciada; aprobación
artística global reservada al usuario. Se mantienen los límites H01–H03 de agarres y rig.

## Hito 93, H03 rostro y anatomía visible

[V11: comparativas, poses, fuente y vídeo](art/hound/mesh-v11/README.md).
Órbitas, pómulos, nariz humana, labios y mentón; ojos pequeños ajustados a
la cara. Relieve integrado de deltoides, bíceps, tríceps y antebrazo.
Cuatro piezas reconstruidas, dos brazos ajustados y 90 componentes exactos,
incluido cuello, capucha, clavículas, manos H01 y piernas H02.
53 huesos, jerarquía, bind pose, pesos y claves conservados.

35.490 vértices, 70.624 triángulos (+27,1 %): densidad de autoría pendiente
de presupuesto/retopología. Fuente reabierta, topología y 61 muestras del clip pasan.
25 poses locales, 100 pares cara/ojos/boca contra capucha sin cruces;
volumen de brazos ≥96,825 %. Persisten pinzamientos heredados a 63,75°/85°
de codo y alcance máximo, sin empeorarlos. Cabeza/cuello/capucha mantienen
ensamblajes solapados; no es certificación global ni rig definitivo.
Roundtrip en 13 poses: máximo 0,000002069 m; regresión v10 pasa.
Cooker/visor Vulkan estático 7/7, CTest Release assets/gpu_assets 2/2
y Debug animation_vfx 1/1. 16 PNG y vídeo de 241 frames inspeccionados.

Fuente: `art/characters/hound/v11/hound-mesh-v11.blend`.
Exportación: `assets/characters/hound_rig/v11/hound-rig.gltf` y BIN.
Escena `Hound_Mesh_v11`, malla `H11_DeformMesh`, rig `Hound11_Rig`,
acción `Hound11_joint_check`, colección `HOUND_v11_EXPORT`.
Informe: `reports/hound-anatomy-93/README.md`. Commit local del hito 93, sin push;
resolver con `git log --oneline --grep='^hito 93:'`.
H03 hecha como propuesta; aprobación global pendiente de H05. H04 no estaba iniciada al cerrar H03.
H06/H08 decidirán si alguna animación necesita mandíbula y resolverán el rig.

## Hito 92, H02 grebas, rodillas, tobillos y botas

[V10: comparativas, apoyos, fuente y vídeo](art/hound/mesh-v10/README.md).
Grebas huecas de pared radial de 6 mm, lengüeta integrada de rodillera,
placas de 4 mm en Y ajustadas al soporte y botas con suela/cavidad de tobillo.
Forro ajustado solo bajo Z=0,61 m; 14 piezas reconstruidas, una ajustada,
81 exactas, incluidas las manos. Huesos, pesos, bind pose y claves conservados.

27.958 vértices, 55.560 triángulos (+19,0 %), siete materiales, 87 rígidos
y 53 huesos. Fuente reabierta: topología, conservación y 61 muestras del clip pasan.
17 poses adicionales, 1.785 pares locales sin cruces y plantas sin inversión.
Cobertura hasta 70° de rodilla; flexión profunda y torsión lateral sin certificar.
Roundtrip en 13 poses: máximo 0,000002069 m; regresión v09 pasa.
Cooker/visor Vulkan estático 7/7, CTest Release assets/gpu_assets 2/2
y Debug animation_vfx 1/1. 14 PNG y vídeo de 181 frames inspeccionados.

Fuente: `art/characters/hound/v10/hound-mesh-v10.blend`.
Exportación: `assets/characters/hound_rig/v10/hound-rig.gltf` y BIN.
Escena `Hound_Mesh_v10`, malla `H10_DeformMesh`, rig `Hound10_Rig`,
acción `Hound10_joint_check`, colección `HOUND_v10_EXPORT`.
Informe: `reports/hound-legs-92/README.md`. Commit local del hito 92, sin push;
commit `6c2fe7f`. Al cerrar el hito 92, H03 no estaba iniciada.
H05 conserva la aceptación global; H08/H11 resolverán rig y marcha finales.

## Hito 91, H01 manos y guanteletes

[V09: comparativas, poses, fuente y vídeo](art/hound/mesh-v09/README.md).
Carcasas/aros huecos, 28 falanges dorsales, guantes ajustados y extremos de
placas frontales liberados. Pico dorsal, resto del cuerpo y rig conservados.
32 componentes reconstruidos, cuatro ajustados, 60 exactos; 23.522 vértices,
46.680 triángulos (+17,1 %), siete materiales, 87 rígidos y 53 huesos.

Fuente reabierta, topología/pesos y 61 muestras del clip pasan. Cinco poses por
mano, 121 pares locales por pose sin cruces; guantes sin autointersecciones
no adyacentes. Roundtrip en 13 poses: máximo 0,000002036 m; regresión v08 pasa.
Cooker/visor Vulkan estático 7/7, CTest Release assets/gpu_assets 2/2 y Debug
animation_vfx 1/1. Renders y vídeo de 151 frames inspeccionados.

Soul Reaper original intacto: apoyo diagnóstico en la carcasa posterior, sin
cruces en esa pose. No es empuñadura cerrada ni socket jugable aprobado.
H08 debe resolver cierre palmar, oposición del pulgar, apoyo izquierdo y apuntado.
La aceptación global sigue reservada a H05; no hay rig/texturas finales o integración.
Informe: `reports/hound-hands-91/README.md`. Commit local del hito 91, sin push;
commit `8bc8060`. Al cerrar el hito 91, H02 no estaba iniciada.

## Hito 90, tareas independientes de Hound

[Índice, dependencias y encargo reutilizable](art/hound/tasks/README.md).
13 fichas cubren remates artísticos, aprobación de escultura, contrato/presupuesto,
malla/LODs, rig, UVs/horneado, materiales, locomoción, combate e integración.
Cada ficha limita alcance, entradas, exclusiones, comprobaciones y cierre.
Al ejecutarla, actualizar en el índice la fuente acumulada y productos concretos;
no inferirlos por el número de versión ni releer toda la cadena de informes.

Solo documentación: no se modifica modelo, exportación, scripts o gameplay.
V08 no se aprueba por este plan. H05 exige aceptación visual antes de producción;
H06 puede investigarse por separado. Las ediciones de .blend se encadenan.
Enlaces, rutas y dependencias comprobados; informe `reports/hound-tasks-90/README.md`.
Commit local, sin push; consultar `git log -1` para el hash.

## Hito 89, primera propuesta artística global de Hound

[V08: comparación completa, rostro, fuente y vídeo](art/hound/mesh-v08/README.md).
Pase de formas secundarias, no escultura final. Se reconstruyen 33 placas,
dos paños y los tres elementos de ojos/boca. Se refinan brazos, cabeza,
pantalones y faja conservando su topología y pesos; los otros 53 componentes
permanecen exactos. Capucha/cuello, clavículas, filos y falanges no cambian.
Las placas dorsales de manos se refinan conservando su orientación y extremo en pico.

20.130 vértices y 39.880 triángulos (+12.208, 44,1 %), 96 componentes,
87 rígidos, un mesh/skin y siete materiales. Mismos 53 huesos y clip diagnóstico;
máximo dos influencias. Densidad de autoría: no presupuesto final de ejecución.

Fuente reabierta: superficies modificadas cerradas/conexas/orientadas, posiciones
de placas comprobadas contra v07, 61 muestras del clip y cuatro poses de cuello.
Reimportación en 13 poses: error máximo 0,000002036 m. Regresión v07 pasa.
Cooker y visor Vulkan estático 7/7 piezas/siete batches; CTest Release
assets/gpu_assets 2/2 y Debug animation_vfx 1/1. Seis PNG, poses adicionales
y vídeo revisados. Informe: `reports/hound-art-89/README.md`.

Cierre local `68e686e`, sin push. El hito anterior
88 es `1b9e537`, conservado sin cambios. Siguiente: validar visualmente la v08
y continuar los remates artísticos pendientes; no presentar las pruebas
técnicas como cierre de escultura, aprobación artística o rig definitivo.
Sin UVs/texturas finales, retargeting, caminar/Bite finales o sustitución jugable.

## Hito 88, feedback de daño y VFX de armas

Al perder vida o escudo, el HUD presenta durante 1,5 s una viñeta roja con
gradiente desde todos los bordes físicos y centro totalmente transparente. Un
arco junto a la mirilla indica la dirección relativa del rival. La presentación
se deriva de snapshots existentes, sin cambiar el protocolo 22.

El fogonazo confirmado conserva su ráfaga, pero ahora acompaña al socket animado
del arma hasta expirar. Los proyectiles de IronHellGoat dejan una estela de humo
gris persistente; su explosión pasa de 8 a 28 partículas, con mayor expansión,
duración y brillo. No cambia daño, radio, velocidad ni autoridad del arma.

Build Debug de `gloom`, pruebas UI/VFX/red 3/3 y captura Vulkan de efectos completados
sin partículas descartadas (489 nacidas, 377 expiradas). Galería UI ampliada a
18 estados en 1280×720, 1920×1080 y 2560×1080; viñeta e indicador inspeccionados.
Informe: `reports/damage-weapon-vfx-88/README.md`. Queda pendiente confirmar
subjetivamente intensidad y duración durante una partida humana.

## Hito 87, capucha, cuello y encaje clavicular

[V07: comparaciones, fuente y vídeo](art/hound/mesh-v07/README.md).
Dos superficies sustituyen capucha/borde/cuello; la capucha incluye su forro
interior y conserva exactamente las polilíneas frontales aprobadas. Se ajusta
solo la zona medial de las dos bases claviculares. Los otros 92 componentes,
incluidos seis filos, hombros, rostro y manos, conservan geometría y pesos.

14.026 vértices y 27.672 triángulos (+548), un mesh/skin, siete materiales,
96 componentes y 87 rígidos. Mismos 53 huesos y claves del clip. Pesos nuevos
provisionales, máximo dos influencias; la base de la capucha acompaña al torso.
No hay rig definitivo, simulación de tela o topología facial final.

Fuente reabierta: superficies cerradas/conexas/orientadas, abertura exacta,
61 muestras del clip, cuatro poses extra y 13 poses reimportadas (máximo
0,000002036 m). Contactos capucha/cuello con clavículas en reposo: 80 pares
de caras en v06, cero en v07. Giros de cabeza ±35° e inclinaciones cuello/cabeza
±10°/±20°: sin cruces detectados capucha/clavículas ni autointersecciones entre
caras no adyacentes de la capucha. Es una prueba local, no colisión exhaustiva.

Cooker y visor estático Vulkan 7/7 piezas/siete batches; CTest Release
assets/gpu_assets 2/2 y Debug animation_vfx 1/1. Regresión v06, renders y vídeo
revisados. Informe: `reports/hound-mesh-87/README.md`.

Cierre local, sin push en esta continuación. Hitos 85 (`56d3fc3`), 86 (`5e494ea`)
y 87 locales; último remoto comprobado `a5302d1`. Consultar `git log -1` para
el hash del cierre actual. No confundir cierre técnico con aprobación artística.
Siguiente: contactos del conjunto y agarres reales, después anatomía/acabado
y topología facial específica. Sin UVs/texturas finales, retargeting,
caminar/Bite de producción o sustitución del personaje jugable.

## Hito 86, cintura, ropa y rostro continuos

[V06: comparaciones de cintura/rostro, fuente y vídeo](art/hound/mesh-v06/README.md).
Tres superficies sustituyen once componentes: pantalón con entrepierna conexa,
faja hueca de 3 mm y cabeza neutra sin los bloques superpuestos de mandíbula,
nariz, mejillas y cejas. Se conservan exactamente los otros 94 componentes,
incluidos capucha, ojos, boca provisional, armadura, brazos y manos.

13.752 vértices y 27.124 triángulos de autoría (+6.738), un mesh/skin y siete
materiales. Mismos 53 huesos y claves del clip; pesos nuevos provisionales,
hasta dos influencias. La densidad final y los LODs quedan pendientes.
La cabeza está ligada al hueso Head: no hay rig facial ni apertura de mandíbula.

Fuente reabierta: topología cerrada/conexa/orientada, 61 muestras del clip,
cuatro poses adicionales aisladas y 13 poses reimportadas (máximo 0,000002800 m).
Cooker y visor estático Vulkan 7/7 piezas/siete batches; CTest Release
assets/gpu_assets 2/2 y Debug animation_vfx 1/1. Regresión v05, seis PNG,
poses de esfuerzo y vídeo revisados. Informe: `reports/hound-mesh-86/README.md`.

Sin push en esta continuación. Hitos 85 (`56d3fc3`) y 86 con cierre local;
último remoto comprobado `a5302d1` (hitos 83–84). Consultar `git log -1` para
el hash del cierre actual. No confundir cierre técnico con aprobación artística.
Siguiente: capucha/cuello y ajuste de placas; después contactos/agarres y un
pase anatómico/facial específico. Sin UVs/texturas finales, retargeting,
caminar/Bite finales, simulación de tela o sustitución del personaje jugable.

## Hito 85, superficie interior de manos continua

[V05: comparación de manos, fuente y vídeo](art/hound/mesh-v05/README.md).
Dos guantes interiores cerrados unen las palmas a los cinco dedos, bajo las
falanges metálicas y la placa dorsal completa en pico. Se sustituyen solo las
dos palmas; 103 componentes, 93 piezas rígidas, 53 huesos y clip conservados.
Una malla, siete materiales, 20.386 triángulos; pesos provisionales, hasta dos
influencias. No hay rig definitivo ni aprobación nueva del acabado.

Fuente reabierta y comprobada: 61 muestras del clip, cuatro casos adicionales
de pulgar, manos conexas/manifold y reimportación glTF en 13 poses (error máximo
0,000002800 m). Cooker y visor estático Vulkan 7/7 piezas/siete batches;
CTest assets/gpu_assets Release 2/2 y animation_vfx Debug 1/1. Regresión v04,
renders y vídeo revisados. Informe: `reports/hound-mesh-85/README.md`.

Push solicitado en esta sesión: hitos 83–84 publicados en `rub0/Gloom2/main`,
hasta `a5302d16b393d1715f36c91106826c326f2a6309`, hash remoto comprobado.
El hito 85 tiene cierre local; no se ha hecho un nuevo push después de continuarlo.
Siguiente pase: cintura/ropa y rostro; comprobar después agarres/contactos de
armadura. Las pruebas no certifican ausencia de penetraciones ni calidad final.
Sin UVs/texturas finales, retargeting, caminar/Bite finales o sustitución jugable.

## Hito 84, hombros y brazos continuos

[V04: comparación, fuente y vídeo](art/hound/mesh-v04/README.md).
Dos superficies continuas sustituyen seis volúmenes superpuestos; 37 anillos
de 32 vértices por brazo. Una malla, siete materiales, 17.466 triángulos (+336).
103 componentes restantes y los 53 huesos de prueba conservados. La desviación
dirigida máxima de vértices nuevos a las superficies anteriores es 2,763 mm.

Validación de fuente/topología/pesos en 61 muestras y reimportación glTF en 13
poses (error máximo 0,000002800 m); cooker/visor estático Vulkan 7/7 piezas,
siete batches, CTest assets/gpu_assets Release 2/2 y animation_vfx Debug 1/1.
Regresión de v03 comprobada; renders y vídeo inspeccionados. Informe:
`reports/hound-mesh-84/README.md`. Hitos 83–84 publicados el 12 de septiembre,
antes de comenzar v05; el informe original conserva el estado de su fecha.

Este pase elimina las uniones visibles de los volúmenes del brazo; NO es malla
definitiva ni validación exhaustiva de colisiones/agarres. Continuar anatomía y
conexiones de manos/dedos, cintura/ropa y rostro, conservando el rig diagnóstico.
Sin UVs/texturas finales, retargeting, caminar/Bite finales o sustitución jugable.

## Hito 83, malla y rig de prueba de Hound

[Fuente, vistas y vídeo v03](art/hound/rig-v03/README.md). Una malla, siete
materiales/primitivas, 17.130 triángulos y 53 huesos; cuatro extremidades con
malla continua y 93 piezas rígidas. Máximo dos influencias por vértice.
43 nombres Legacy conservados, pero ejes/longitudes/jerarquía adaptados: requiere
retargeting, no copiar clips directamente. Clip diagnóstico de 6 s, no caminar/Bite.

Validación de fuente en 61 muestras, reimportación glTF en 13 poses con error
máximo de 0,000004689 m, cooker/visor estático Vulkan 7/7 piezas y siete batches;
CTest assets/gpu_assets Release 2/2 y animation_vfx Debug 1/1. No se valida
reproducción del clip en el juego, agarres, ausencia de penetraciones ni acabado final.
Informe: `reports/hound-rig-83/README.md`. Cierre local, sin push del hito 83.

Push anterior solicitado: hitos 80–82 publicados en `rub0/Gloom2/main`, hasta
`d7fc1f8`, hash remoto comprobado. La aceptación posterior del usuario establece
primero estabilizar topología; el rig actual sirve solo para detectar problemas.
Quedan solapes deltoides/bíceps/brazo y contactos de placas. Trabajar en otra
versión, conservando diseño v02 y prototipo v03. Sin texturas ni sustitución jugable.

## Hito 82, diseño de Hound v02 aprobado

El usuario acepta el diseño presentado en el hito 81 con «apruebo el diseño».
La [v02 y sus detalles](art/hound/blockout-v02/README.md) quedan como referencia
aprobada de silueta y volúmenes: hombros libres, armadura clavicular, capucha
más cerrada y manos orientadas con placa dorsal completa en pico.

Registro documental únicamente: no se modifica el modelo ni se inicia el rig.
El siguiente hito debe conservar v02, trabajar en otra versión y preparar la
malla para un rig de prueba. Comprobar caminar, apuntar y Bite, además de
deformaciones, penetraciones y agarres. Evaluar el rig existente antes de
decidir reutilizarlo. UVs/materiales finales e integración jugable siguen pendientes.

Validación de diff, enlaces locales y alcance exclusivamente documental;
no se repiten pruebas del motor para esta aceptación. Informe:
`reports/hound-approval-82/README.md`. Commit local, sin push.

## Hito 81, Hound v02 — clavículas, capucha y manos

Fuente: `art/characters/hound/v02/hound-blockout-v02.blend`.
[Vistas y detalles](art/hound/blockout-v02/README.md). La versión 01 se conserva
sin cambios. El usuario pidió liberar los hombros, acercar la armadura a la
clavícula, tapar más la cara y corregir las manos con una placa dorsal completa en pico.

Copas de hombro retiradas y deltoides descubiertos; filos anclados más medialmente.
Capucha adelantada y abertura más baja/estrecha. Palmas hacia los muslos,
pulgares anteriores, dedos que flexionan hacia la palma y cobertura dorsal
desde muñeca a nudillos con punta distal. Son cambios de volumen, no acabado.
113 piezas, 7.626 triángulos y 7 materiales, con altura de 1,80 m.

Validación de .blend y glTF, 67 piezas ajenas conservadas, reconstrucción del
BIN idéntica, regresión de v01, cooker/visor Vulkan con 113/113 piezas y
CTest **2/2**. Vistas y primeros planos inspeccionados. Sin rig, texturas finales,
pruebas de deformación/agarres ni sustitución del personaje jugable.
Informe: `reports/hound-blockout-81/README.md`. Hito con commit local, sin push.
La valoración artística de v02 queda aprobada en el hito 82; el acabado final
y la animación todavía requieren sus propias revisiones.

## Hito 80, volumen básico de Hound

Fuente editable: `art/characters/hound/v01/hound-blockout-v01.blend`.
[Frente/perfil/espalda y ficha](art/hound/blockout-v01/README.md), más tres cuartos.
105 piezas, 6.974 triángulos evaluados y 7 materiales planos. Altura 1,80 m,
igual al Archangel actual; ancho con filos 1,144 m y fondo 0,382 m.

Capucha, ojos naranja, torso por placas, brazos desnudos, hombreras/guanteletes
afilados y piernas. Rostro/manos son placeholders de volumen; espalda interpretada
provisionalmente. El facetado no cambia la dirección artística a low-poly.
No hay rig, texturas o UVs finales; no cambia el Hound jugable ni el gameplay.
La lámina de vistas proviene de la malla real, no de generación 2D.

MCP nativo de Blender ya disponible y usado directamente. Fuente .blend abierta
y validada en background; glTF validado y geometría reconstruida con BIN idéntico.
Cooker y visor Vulkan correctos (105/105 piezas), capturas revisadas y CTest **2/2**.
105 batches corresponden a piezas editables: optimizar después de aceptar formas.
No se ha validado deformación, agarres de armas, partida ni presupuesto de combate.
Informe: `reports/hound-blockout-80/README.md`.

Push solicitado y confirmado al repositorio público `rub0/Gloom2/main`:
siete commits publicados hasta `988437d` (hito 79), hash remoto comprobado.
El hito 80 se cierra con commit local; no extender automáticamente aquel push.
Las siguientes revisiones deben conservar v01 y usar una nueva versión.

## Hito 79, puente local Blender–Gloom

Blender portable 4.5.13 LTS y Blender MCP 1.9.1 instalados en `.cache`, con Python
aislado, perfil propio, telemetría desactivada, proveedores externos apagados y
modo seguro del MCP activo. `.codex/config.toml` registra cuatro herramientas
locales. No se abren puertos a la red ni se usan servicios 3D de pago.

Arranque: `tools/art/start_blender.ps1`. Flujo, reinstalación y límites de seguridad:
[BLENDER_WORKFLOW.md](BLENDER_WORKFLOW.md). La sesión iniciada antes del registro
puede necesitar recargar MCP para mostrar sus herramientas; se ha validado la
conexión mediante un cliente stdio real, no mediante herramientas nativas ya cargadas.

Fixture independiente: 3 mallas, 424 triángulos, 3 materiales y 1 PNG externo.
Exportar **glTF separado**, no GLB con texturas embebidas: el cooker actual las
rechaza. Metros, normales, UV0, conversión a Y-up y dependencias verificadas.
Visor con captura opcional y salida tras 32 frames con la escena residente.

Validación: build Release del visor/cooker y dos tests; CTest **2/2**, cliente MCP,
verificador de fixture, cocción y render Vulkan con **3/3 piezas visibles**.
Capturas Blender y Gloom inspeccionadas. No se repite la suite completa ni se
afirma validación de rigs/animaciones desde Blender. Informe:
`reports/blender-bridge-79/README.md`. No cambia el personaje jugable ni gameplay.

La primera fuente Hound fuera de cache y las vistas se entregan en el hito 80.
No hacer retopología, UVs, rig o texturas finales hasta la validación del volumen.

## Hito 78, guía artística y boceto de Hound — aprobados

Entregada la [guía 0.1](DIRECCION_ARTISTICA.md) y la lámina
`docs/art/hound/hound-concept-v01.png`: cuerpo completo, cabeza y guanteletes.
Referencia elegida por el usuario: `D:\Descargas\Gloom\concept_hound2.jpg`,
con copia intacta en `docs/art/characters/hound-original-concept.jpg`.
Hound es humanoide bípedo: capucha, piel pálida, ojos naranja y armadura afilada.
El diseño actual usa el cuerpo de Archangel, no un rig cuadrúpedo recuperado.

Identidad desde los concepts originales; UT3 orienta peso y materiales y el toque
de Overwatch 2 orienta claridad. El usuario acepta guía y lámina con «asi esta
perfecto, continua con lo siguiente». Conservar el peso de la armadura y la lectura
ligeramente pétrea de esta imagen. La aceptación no aprueba vistas aún no resueltas.

Validación documental, integridad de las tres referencias copiadas y revisión
visual de la lámina. Prompt y procedencia guardados; no corresponde compilar ni
repetir pruebas del motor para esta entrega de documentación y concept 2D.
Informe: `reports/art-direction-78/README.md`. El hito 78 no entregó modelo ni
instalación; el puente Blender/MCP posterior está validado en el hito 79.

## Hito 67, recogibles de Factory

14 arquetipos en 73 ubicaciones auditadas: recompensas únicas en la autoridad,
topes, reserva de munición sin arma, adquisición, respawn y tirón del Soul Reaper.
Disponibilidad/posición/respawn y efectos se replican en snapshots completos;
primer snapshot tardío y reconexión conservan el estado. Las mallas siguen ese estado.

Decisiones: consumir incluso al tope; seleccionar el arma recogida si se llevaba
Soul Reaper. Efectos de 15 s, refresco sin apilar: daño ×3, reducción saturada
al 100 % con mínimo de un tick. Expiración independiente y limpieza al morir.
Soltar el tirón, cambiar de arma o morir devuelve el objeto al origen sin premio.
`SniperAmmo1` genérico `Ammo` queda visual: está fuera de los 14 arquetipos y
73 ubicaciones que cuenta la auditoría (hay 74 registros visuales con recompensa).

Validación: **9/9 pruebas focalizadas** y cuatro capturas Vulkan inspeccionadas.
Informe: `reports/pickups-2026-09-06/README.md`. No se repitió la suite completa
ni una sesión manual con dos clientes gráficos. La revisión jugable del usuario
está pendiente; el cierre técnico no implica esa confirmación manual.

## Hito 77, presentación visual de recogibles

El objetivo aprobado es que los objetos del mapa inviten a recogerlos mediante
flotación, giro y halo aditivo al estilo Quake. La lógica de disponibilidad,
recompensa, respawn y red del hito 67 no se modifica. La presentación será local
y estará anclada a la posición original del objeto para no introducir estado de
red ni deriva física.

Los 73 recogibles flotan hasta ±8 cm y giran a 0,72 rad/s con fases distribuidas.
Escudo, armas, munición y modificadores llevan un halo radial aditivo coloreado;
vida flota y gira sin brillo. Aparición y respawn interpolan en 0,45 s desde 35 %
de escala y opacidad cero. La presentación es local, no modifica autoridad ni
protocolo, no añade luces dinámicas ni sombras y el halo evita la copia de escena
de las partículas.

Builds Debug/Release, **48/48 pruebas** y cinco capturas Vulkan inspeccionadas.
El énfasis por proximidad y el destello de recogida continúan como pulido opcional;
queda pendiente la confirmación subjetiva en una partida humana. Informe:
`reports/pickup-presentation-2026-09-09/README.md`.

Si se revisan recogibles, leer solo la sección 67 de
[GAMEPLAY_MIGRATION.md](GAMEPLAY_MIGRATION.md) y las partes pertinentes del informe.

## Hito 68, audio original

Backend SDL3 desacoplado con fallback nulo, mezclador a 48 kHz, buses, límite de
voces, loops y lifecycle seguro. Música compartida entre menú/partida; pausa atenúa
música y silencia efectos/ambiente. Listener en cámara local, primera persona
estéreo y fuentes remotas/impactos/recogibles/nueve ambientes de Factory en 3D.
Volúmenes configurables con `GLOOM_AUDIO_MASTER`, `GLOOM_AUDIO_MUSIC` y
`GLOOM_AUDIO_EFFECTS` en `[0,1]`.

Auditoría reproducible de 89 archivos: 37 importados/cocinados, 3 referencias
ausentes conocidas, un grupo duplicado y `troll` excluido. PCM GAU1 preserva la
salida decodificada y valida formato, canales, frecuencia, duración y finitud.
Reglas: `docs/AUDIO.md`; inventario: `assets/audio/inventory.json`; informe:
`reports/audio-2026-09-06/README.md`.

Pasos cada 365 ms solo con desplazamiento y apoyo. Umbrales físicos convertidos:
aterrizaje -6,5625 m/s y gruñido -18,75 m/s. Disparos/cargas, impactos, explosión,
retorno/guiado, sin munición, cambio/adquisición, daño, muerte y respawn emiten
eventos autoritativos. Diario de 128 eventos; protocolo 19 envía solo el último
segundo con redundancia. Primer snapshot, repetición/reordenación y reconexión no
reproducen one-shots históricos; los bucles vigentes se reconstruyen.

Validación: **11/11 pruebas focalizadas**, auditoría/cooker y smoke SDL3 real sobre
`Headphones (High Definition Audio Device)`. Secuencia regenerable de 58 s en el
informe. Sigue pendiente que una persona confirme subjetivamente volumen, timbre
y espacialización; el cierre técnico no sustituye esa escucha.

## Último hito cerrado: 69, revisión jugable

La captura del usuario confirmó tres regresiones. IronHellGoat convertía como ticks
una velocidad Legacy por milisegundo y volaba 16,67 veces demasiado lento; sus
proyectiles no consultaban la geometría y `Jumper1` seguía explícitamente diferido.

IronHellGoat vuela ahora a 22,5–5,25 m/s según carga, nace delante de la cápsula,
barre su radio contra sólidos/triángulos y destruye/explota en el primer contacto.
Daño radial, estela y destello son autoritativos/replicados. Un impacto emite solo
`fireball_hit`, sin el segundo clip de explosión que se solapaba. `Jumper1` aplica
en autoridad y predicción su fuerza original (4,6875/32,8125/0 m/s) y reproduce
`gameplay/plasma.wav` en 3D. Protocolo **20**.

Validación: **12/12 pruebas focalizadas**, auditoría de 89 audios con 38 importados,
smoke Vulkan de 90 frames (381 partículas, 0 descartadas) y ejecutables cliente/
servidor recompilados. Informe: `reports/gameplay-fixes-2026-09-07/README.md`.
Queda pendiente la nueva confirmación jugable/subjetiva del usuario.

## Último hito cerrado: 70, habilidades de clase

Las seis habilidades del roster original disponible están integradas. Hound usa
Bite (`Q`) y Berserker (`E`); Archangel, Diamond Skin y Life Dome; Shadow,
Invisibility y Flash. Guard permanece como loadout compatible. Screamer sigue
fuera del roster porque carece de modelo y definición de contenido propia.

Decisiones del usuario: Berserker conserva fidelidad literal a la ruta ejecutable
Legacy —20 s de estado, olor y audio, sin bonus de daño/cadencia— y podrá calibrarse
después. Life Dome cura 10 puntos solo al propio Archangel, nunca al enemigo.
Diamond Skin inmuniza frente a combate durante 5 s sin impedir la lava letal.
Flash conserva alcance, orientación y línea de visión Legacy. Muerte, respawn y
cambio de selección limpian estados y loops.

La autoridad valida las activaciones; snapshots y protocolo **21** replican ambas
habilidades, cooldowns, estados y factor de Flash bajo pérdida/reordenación,
primer snapshot y reconexión. HUD Q/E, audio original y VFX FPS/TPS están
integrados. Inventario de audio: 89 auditados, 46 importados, tres ausentes
conocidos y un duplicado.

Validación: build Debug completo, **47/47 pruebas**, auditoría de audio y smoke
Vulkan/Jolt. Se inspeccionaron seis capturas deterministas generadas mediante
`gloom --ability-review DIR`. Informe y comandos:
`reports/abilities-70/README.md`.

Siguiente paso: sesión jugable humana con cada habilidad y dos clientes. Ese pase
puede abrir ajustes de Berserker o presentación, pero no queda funcionalidad del
alcance técnico diferida. El siguiente hito definido es la primera pasada de
rendimiento descrita abajo.

## Último hito cerrado: 71, primera pasada de rendimiento

El renderer agrupaba batches de visibilidad pero los ignoraba al dibujar; ahora
los draws opacos comparten pipeline, recursos y bindings por mesh/material. Los
transparentes conservan su orden por distancia y existe compatibilidad para
snapshots sin batches. La iluminación ya no crea una `vector` por cada celda de
la rejilla 16×9×24: usa conteo plano, prefijo y relleno en dos pasadas. Las
paletas de skinning reservan su capacidad antes de insertar matrices.

Corrección del hito 74: los 108 FPS publicados medían solo el tramo de render de
una escena simplificada, no Factory jugable ni el frame completo. El resultado
histórico de la suite fue 47/47, pero no sirve como benchmark comparable con
Factory. Informe: `reports/performance-2026-09-09/README.md`.

Esta referencia no sustituye la medición de la partida humana a 1920×1080 ni
explica por sí sola los 13 FPS reportados. El siguiente frente es perfilar
skinning/presentación, asignaciones por frame y sombras en esa configuración.

## Hito 73, perfil 1080p y reservas de presentación

Se añadió `--vertical-slice-performance-1080p`, que ejecuta el escenario de
perfil a 1920×1080, mide el loop completo y expone instancias/batches. El caso
mide **32,32 FPS** en 720 frames: 30,9369 ms por loop, 29,4376 ms de
render/presentación, 6,84914 ms de visibilidad, 0,269229 ms de iluminación y
3,54779 ms GPU. Son 227 instancias sometidas, 71 visibles y 26 batches; la
memoria pico es 702,32 MB device-local y 42,67 MB host-visible. Corrección del
hito 74: esa media incluía carga y esperas de trabajos; no permitía atribuir el
coste a visibilidad, streaming o resolución. Además se ejecutaba el guion del
smoke simplificado sobre Factory y se habían omitido comprobaciones incompatibles.

La lista completa de instancias y las listas animadas reservan capacidad antes
de añadir proyectiles, personajes y efectos, reduciendo realojos y copias por
frame. Informe ampliado: `reports/performance-2026-09-09/README.md`.

## Hito 74, benchmark corregido y sombras

Factory original se mide a 720p/1080p con la misma cámara, contenido y simulación:
esperar todos los recursos solicitados, calentar 120 frames y medir otros 360,
resolución nativa fija, sin VSync ni capturas. Informe de media/p50/p95/p99/máximo,
seis etapas CPU y memoria de proceso. Fallo o cierre prematuro devuelve error.
El benchmark no incluye HUD/audio ni un recorrido de combate completo.

Se eliminó el sleep fijo de 16 ms del juego, que impedía alcanzar 100 FPS.
VSync queda desactivado por defecto; `GLOOM_VSYNC=1` lo activa en partida.
Se descartan conservadoramente las esferas fuera de cada volumen de sombras;
el descarte usa espacio de luz y contempla escala no uniforme/reflejada. Las
listas de presentación conservan capacidad entre frames; se intercambian las
listas de animación actual/anterior sin copiar toda la lista.

Ryzen 7 3700X / GTX 1070: Debug 1080p pasa de 41,273 a 32,934 ms con el benchmark
corregido (20,2 % menos). Release 1080p: **144,17 FPS**, media 6,936 ms,
p99 6,964 ms, máximo 7,111 ms. Release 720p: 546,76 FPS; ambas resoluciones
terminan con 224 instancias/85 visibles/35 batches. No atribuir el salto de
Debug a Release únicamente al cambio de código ni garantizar esos FPS en todo combate.

Validación: builds Debug/Release; siete pruebas focalizadas únicas pasan
(render_scene, visibility, factory/character_visual_review, vulkan_sync, ui_flow,
vertical_slice_smoke). Las 13 capturas de Factory/personajes coinciden píxel a
píxel con referencias. Se corrigió el límite de carga visual contado en frames
por un límite temporal después de un fallo reproducido. Sin nueva suite completa.
Memoria Release 1080p: 2085 MiB privados/1133 MiB working set; queda trabajo de
residencia y retención de recursos. Detalle: `reports/performance-74/README.md`.

Ejecutable para jugar y medir: `build/windows-vs/Release/gloom.exe`.
Comandos: `--vertical-slice-performance-720p` y `--vertical-slice-performance-1080p`.

El ejecutable actualizado es `build/windows-vs/Debug/gloom.exe`; el servidor
`build/windows-vs/Debug/gloom_slice_server.exe` está recompilado con protocolo 22.
Revisión de audio: `gloom --audio-review DIR [--device]`.

## Hito 75, rachas de bajas y audio

La autoridad mantiene `current_spree` por combatiente: aumenta únicamente al
conseguir una baja atribuida a otro jugador y se reinicia al morir, incluida la
muerte ambiental. Se replica junto a los snapshots con el protocolo **22**.
La semántica Legacy reproduce `feedback/bell.mp3` en 3, 6, 9, …, 27 bajas
consecutivas; el cliente solo lo presenta cuando el actor es el jugador local,
por lo que no hay sonidos duplicados por eventos remotos.

Validación: auditoría/cocción de audio (**47/89** importados), build Debug de
`gloom_audio_tests` y prueba focalizada de audio completa, incluyendo umbrales,
reinicio, serialización de racha y filtros local/remoto. Informe:
`reports/kill-streak-75/README.md`.

## Último hito cerrado: 76, marcador de partida

Mientras se mantiene `Tab` durante una partida aparece un marcador sobre el HUD,
sin pausar ni bloquear el control. Presenta los dos combatientes con nombre,
clase, bajas y muertes; ordena por bajas y usa menos muertes como desempate. El
jugador local queda resaltado. Los nombres se resuelven por entidad desde la sala
y la partida local usa etiquetas de reserva.

La implementación reutiliza los contadores autoritativos ya presentes en
`SliceSnapshot`, por lo que no cambia el protocolo **22**. Se añadió el estado 17
a la galería de revisión y referencias deterministas para 1280×720, 1920×1080 y
2560×1080. Informe y validación: `reports/scoreboard-76/README.md`.

## Mapa mínimo de archivos

- Reglas auditadas: `assets/gameplay/legacy_rules.json` y
  `tools/legacy/audit_gameplay.py`; código fuente de referencia en el checkout
  Legacy, sobre todo `Src/Logic/Entity/Components` y `Exes/media/maps`.
- Ubicaciones/collider: `assets/legacy/factory_scene.json`,
  `src/gameplay/factory_scene.cpp`.
- Inventario: `include/gloom/gameplay/legacy_arsenal.hpp` y su `.cpp` en
  `src/gameplay`; ya existen `acquire`, `add_ammo`, `owns`, `ammo` y `select`.
- Recogibles: `include/gloom/gameplay/legacy_pickups.hpp` y `.cpp`;
  `tests/legacy_pickups_tests.cpp`. Revisión visual: `gloom --pickup-review DIR`.
- Simulación, daño y proyectiles: `src/gameplay/vertical_slice.cpp`;
  contratos en `include/gloom/gameplay/vertical_slice.hpp` y `components.hpp`.
- Red: `src/gameplay/vertical_slice_network.cpp`,
  `src/network/movement_replication.cpp`, `include/gloom/network/protocol.hpp`.
- Presentación: `apps/gloom/main.cpp`, `apps/gloom/game_ui.hpp`.
- Audio: contratos en `include/gloom/audio`, mezclador en `src/audio`, backend
  SDL3 en `src/backends/sdl_audio.cpp`; eventos/presentación en `src/gameplay`.
- Pipeline de audio: `tools/legacy/audit_audio.py`, contenido en `assets/audio`,
  reglas en `docs/AUDIO.md` e informe en `reports/audio-2026-09-06`.
- Regresiones: `tests/legacy_arsenal_tests.cpp`, `tests/vertical_slice_tests.cpp`,
  `tests/vertical_slice_network_tests.cpp`, `tests/legacy_movement_tests.cpp`.
- Habilidades: contratos en `slice_selection.hpp`, `vertical_slice.hpp` y
  `components.hpp`; autoridad en `vertical_slice.cpp`, red en
  `vertical_slice_network.cpp` y revisión con `gloom --ability-review DIR`.

## Validación y herramientas

- Revisión de HUD/colisiones: 43 pruebas validadas (40 en pasada completa y
  3 visuales tras inspeccionar y actualizar referencias).
  Informe: `reports/gameplay-fixes-2026-09-05/README.md`.
- Última revisión, escaleras/momento: **7/7** pruebas focalizadas, con geometría
  real de Factory, techo, reposo en pendiente, red y smoke gráfico. Confirmación
  posterior del usuario. Informe: `reports/stairs-momentum-2026-09-05/README.md`.
  No se ha repetido la suite completa después de esta última revisión.
- Hito 67: **9/9**, incluyendo recogibles, arsenal, simulación, dos clientes con
  contención/primer snapshot tardío/reconexión/respawn, protocolo, GNS, movimiento,
  servidor dedicado y smoke gráfico. Logs/capturas en el informe del hito.
- Hito 68: **11/11**, auditoría y corrupción WAV/OGG/MP3, mezclador/backend nulo,
  eventos, dos clientes bajo pérdida/reordenación, reconexión, protocolo 19,
  arsenal/recogibles/movimiento/transporte, dedicado, smoke de dispositivo y
  smoke gráfico Vulkan/Jolt.
- Hito 69: **12/12** focalizadas sobre físicas, arsenal, audio/red, Factory, movimiento,
  VFX, simulación/red/transporte, protocolo y dedicado. Auditoría 89/38 y smoke
  Vulkan de 90 frames; no se repitió la suite completa ni una partida manual.
- Hito 70: **47/47** pruebas Debug, auditoría 89/46 y smoke Vulkan con seis
  capturas inspeccionadas. Falta la aceptación jugable/subjetiva humana.
- PowerShell; CMake/CTest: `D:/Dev/CMake/bin`, preset `windows-debug`.
  Ejecutable: `build/windows-vs/Debug/gloom.exe`.
- Compilar motor antes de enlazar ejecutables. Con
  `/p:BuildProjectReferences=false` se omite reconstruir dependencias: usarlo
  únicamente cuando ya estén actualizadas, para no validar una biblioteca vieja.
- Python disponible: `.cache/legacy-tools/Scripts/python.exe` (Ogre, Pillow,
  numpy, av). PyAV puede inspeccionar/decodificar los WAV/OGG/MP3 durante la
  importación offline; el runtime no debe depender de este entorno. `python`
  puede apuntar al alias de Microsoft Store.

## Trabajo con contexto reducido

Leer este documento primero; abrir fuentes con `rg` y rangos pequeños según
la necesidad. No cargar todos los informes, JSON grandes ni historial del chat.
Redirigir builds/pruebas a logs y mostrar solo errores/resultados. Pruebas
focalizadas durante cambios; ampliar por riesgos de integración concretos y
requisitos del hito. Actualizaciones breves cuando haya avances relevantes.
Al finalizar, actualizar estado, decisiones, validación y siguiente paso aquí;
crear el commit del hito, comprobar `git status` e informar del hash.
