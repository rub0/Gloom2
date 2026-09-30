# Hound H06 — contrato y presupuesto para producción

30 de septiembre de 2026 · **H06 cerrada como entrada técnica para H07**, mediante
el [hito 106](../../../reports/hound-budget-106/README.md). H07 preparada, sin iniciar.
Este presupuesto guía la producción; no certifica todavía el personaje final
ni una partida autoritativa de ocho jugadores. Las secciones históricas conservan
los bloqueos anteriores, sustituidos por las decisiones vigentes de abajo.

Entrada inmutable: [escultura v16](../../../art/characters/hound/v16/hound-mesh-v16.blend)
y [glTF/BIN v16](../../../assets/characters/hound_rig/v16/hound-rig.gltf).
H05 está aprobada; este documento no modifica esa aprobación ni el modelo.
[Diagnóstico inicial](../../../reports/hound-contract-101/README.md) ·
[Mediciones vigentes](../../../reports/hound-budget-106/mediciones.md).

## Objetivo confirmado por el usuario

**200 FPS, 1920×1080 nativo, hasta 8 combatientes, en este equipo:**
Ryzen 7 3700X, GTX 1070, 17.082.138.624 bytes de RAM, Windows,
controlador 32.0.15.8129. Son **5 ms por fotograma para el conjunto**, no para cada Hound.
La resolución de mapas se fija ahora mediante la prueba del 106. No sustituir
este objetivo por los 100 FPS de informes anteriores ni por los Hz de simulación.

## Presupuesto vigente — entrada H07, hito 106

Se elige el perfil de **40.073 triángulos TPS y 18.108 FPS** ensayado con mapas,
mezcla de clips y cinco armas. Cuatro pasadas de 360 frames a 1080p, dos con
audio nulo y dos con dispositivo real: **296,77–298,76 FPS**, p99 máximo
**3,914 ms**, máximo **4,503 ms**, **0/1.440 frames >5 ms**. Los límites de
triángulos se redondean hacia abajo. El perfil de 80.152 TPS sí tuvo picos >5 ms;
no se acepta por su media. [Todas las distribuciones](../../../reports/hound-budget-106/mediciones.md).

| Recurso | Límite de producción / decisión |
| --- | --- |
| Geometría TPS | **LOD0 ≤40.000 triángulos**, cuerpo completo. Siete TPS más FPS local en el escenario de control. |
| Geometría FPS | **≤18.000 triángulos**, extraídos por la regla de pesos vigente; sin LOD de brazos. |
| Materiales/primitivas | **≤7 por cuerpo TPS y ≤4 FPS**. Un atlas compartido por los materiales de cada skin; no cuatro mapas nuevos por cada material. OPAQUE como base. |
| Vértices/memoria geométrica | **≤54.000 vértices cocinados TPS y ≤24 MiB de uploads geométricos del Hound**, contando copias de LODs y FPS. La prueba conserva 53.626 vértices y 22,526 MiB aunque dibuja LOD1. Son límites de asignación, no un asset de 54.000 vértices ya probado. |
| Mapas por skin | Hasta **cuatro mapas de 2048×2048**, mips completos: color BC7 sRGB, normal BC5 lineal, ORM BC7 lineal (AO en R) y emisión BC7 sRGB. ≤21,34 MiB por conjunto sin alineación. Se permiten mapas menores o ausentes. |
| Pool de mapas Hound | Hasta **ocho conjuntos distintos**, geometría compartida. **≤192 MiB asignados**, incluidos margen y mapas auxiliares; ocho conjuntos completos medidos ocupan 170,668 MiB. No implica ocho mallas distintas. |
| Residencia de escena | **Objetivo ≤384 MiB de assets residentes**, techo existente 512 MiB sin ampliar: 128 MiB de reserva respecto al objetivo. Prueba completa 355,43 MiB, sin evicciones ni recursos ausentes. No es VRAM total del driver. |
| Rig/pesos | Control de **53 huesos y máximo dos influencias** por vértice. H08 puede justificar otras distribuciones, pero debe volver a medir antes de ampliar ese coste. Ocho influencias es un techo de formato, no presupuesto validado. |
| Clips | Carga ensayada: cuatro clips TRS, 159 canales/clip, 181 muestras/canal (6 s, 30 Hz), **115.116 claves agregadas**; ruta existente de mezcla de locomoción e IK. Este es el límite inicial de carga, no animación final aprobada. |
| Armas | Las cinco armas existentes residentes y dibujadas entre los siete TPS; Soul Reaper local con disparo. H08/H12 deben resolver agarres, offsets y acciones reales, que esta prueba no certifica. |

LOD1/LOD2 de producción seguirán la ruta existente del cooker: objetivos de
50 % / 20 % de índices, tolerancia de simplificación 0,01, y selección por
distancia/radio de cada primitiva a 30/80. No se exige alcanzar esos porcentajes
si el límite de error conserva más caras. **H07 debe comprobar silueta y
deformación en los LODs y sus transiciones**; el simplificador considera posición,
no calidad de skinning. No hay una convención comprobada de LODs glTF de autoría.
Si la ruta automática no conserva calidad, registrar la alternativa necesaria
antes de pasar a H08. El LOD1 forzado del 106 es una prueba de coste, no retopología.

La geometría, pesos y acción diagnósticos proceden de v16; UVs planas, mapas de
prueba y cuatro copias remuestreadas de su acción se generan solo en caché.
Siete conjuntos de mapas se muestrean entre los siete materiales de todos los
TPS; el octavo reserva memoria. Esto mide carga y acceso a texturas, sin simular
ocho geometrías distintas ni aprobar bakes, locomoción, saltos, agarres o combate final.
Los otros personajes y el arsenal siguen cargados. La compresión BC se solicita
ahora como opcional al crear el dispositivo; RGBA8 conserva su ruta de respaldo,
pero no cumple este presupuesto de residencia en la configuración medida.

### Validación para conservar el presupuesto

Repetir en Release/1920×1080 nativo/IMMEDIATE, sin VSync ni resolución dinámica,
en Ryzen 7 3700X/GTX 1070: 120 frames de calentamiento y 360 medidos, dos pasadas
con audio nulo y dos con dispositivo real, en serie y sin otros builds/tests.
CPU completa, GPU completa, percentiles y máximo; siete TPS animados con sombras
más FPS, HUD y disparo. Registrar la ruta del candidato, no medir v16 por accidente.
**p99 ≤4 ms como margen de trabajo y ningún frame >5 ms** en las cuatro pasadas;
el objetivo final sigue siendo 200 FPS/5 ms para el conjunto. Conservar y explicar
cualquier fallo, sin escoger solo medias o repeticiones rápidas.

Exigir todas las primitivas esperadas visibles, siete poses que cambian, las
cinco armas, cero mallas/texturas ausentes y cero evicciones durante carga y
medida; registrar residencia. El control v16 conserva 53 primitivas y nueve
mapeos; un candidato con menos primitivas debe justificar su recuento propio.
Los techos de contenido no sustituyen esta prueba del candidato: coste de
deformación, cobertura de pantalla y materiales pueden cambiar.

H07 verifica geometría/LODs; H08 rig/pesos y agarres; H09/H10 mapas/bakes reales;
H11/H12 clips, acciones y efectos; H13 integración y partida humana. Volver a
medir cuando cambie cada carga y validar finalmente ocho combatientes con su
integración autoritativa. Son verificaciones de producción posteriores, no
requisitos circulares para poder iniciar H07. **No se inicia ninguna aquí.**

## Convenciones de contenido y material

glTF 2.0 separado, BIN y PNG externos; metros; Blender Z-up → exportación Y-up.
Triángulos, normales explícitas, escalas aplicadas y UV0 válida. Conservar
triangulación y normales entre bake y exportación. Solo exportar malla/rig;
hornear procedimientos y constraints, sin luces/cámaras de revisión.

| Dato | Contrato y evidencia actual |
| --- | --- |
| Base color | PNG sRGB; factor glTF lineal. Gris 128 sRGB frente a factor 0,2158605 da respuesta equivalente dentro de compresión/redondeo. Probado Blender → cooker → GPU. |
| Metallic/roughness | PNG de datos lineales; G = roughness, B = metallic. Factores multiplicativos. ORM (32,64,255) frente a roughness 64/255 y metallic 1 comprobado en GPU. |
| AO | R lineal del mismo ORM, enlazado también a occlusionTexture. Nodo Blender `glTF Material Output`/entrada `Occlusion`. Efecto GPU comprobado; no hornear iluminación direccional al albedo. |
| Emisión | PNG sRGB, factor/intensidad lineales, extensión emissive_strength. Textura gris ×2 y factor equivalente comprobados en GPU. |
| Normal | **Corregida y comprobada (103).** Mapa lineal tangente, XY en RG y Z positivo reconstruido por shader. UASTC almacena RRRG; el decoder restituye G desde A para RGBA8. BC5 ya recuperaba XY. Pruebas X/Y, costuras y UV reflejadas pasan. Los bakes finales siguen sujetos a H08/H09 y al presupuesto. |
| Tangentes | El importador ignora TANGENT de glTF y regenera MikkTSpace desde UV0/normales. Invertir las tangentes exportadas deja el payload cocinado idéntico. Normal maps en UV0; usar UV1 para una normal no recalcula su base tangente. |
| UVs | Solo UV0/UV1. Dos distribuciones distintas comprobadas en GPU. UV2 o textura que referencia UV ausente se rechazan. Sin UDIM/tiles automáticos. |
| Imágenes | PNG externo comprobado; imágenes embebidas rechazadas. No compartir una imagen entre color, datos y normal. Sí compartir R/G/B de ORM como datos; no empaquetar normal con ORM. |
| Compresión/mips | Cooker KTX2 UASTC con cadena completa. Runtime BC7 sRGB para color/emisión, BC7 lineal para ORM y BC5 para normal si hay soporte, RGBA8 de respaldo. El 106 activa la capacidad opcional BC que faltaba: 12 comparaciones de materiales y nueve de normales pasan ahora en GPU BC; 144 comprobaciones numéricas de canales/mips pasan. Controles X/Y también pasan forzando RGBA8. |
| Alpha | OPAQUE por defecto para cuerpo/placas. MASK y BLEND exportados de Blender y visibles en GPU; cutoff 0,5 en muestra. Transparencia es un coste adicional; no usarla para simular huecos de armadura. El visor no certifica sombras/orden en todas las escenas. |
| Materiales Blender | Principled y enlaces exportables. Los nodos de viewport no son contrato de runtime. Factores glTF lineales, sRGB solo en color/emisión. |

La regresión del decoder falla sin la corrección y pasa con ella, conservando
el formato cocinado. Las **12 comparaciones H06 pasan**, junto con nueve
controles GPU de X/Y, reflexión U/V y costuras entre islas UV. Los controles
numéricos comprueban los canales, no solo que la imagen cambie.
[Resultados actuales](../../../reports/normal-maps-103/README.md) y
[diagnóstico histórico](../../../reports/hound-contract-101/results.md).
Las imágenes y JSON enlazados son artefactos locales regenerables; se excluyen
de Git según las pautas del proyecto.

## Rig y animación

Límites de implementación: 1–256 huesos por skin, JOINTS/WEIGHTS 0 y 1
(hasta ocho contribuciones), pesos normalizados; 256 clips, un millón de claves
agregadas, duración positiva hasta 3600 s. Son techos de formato, no presupuestos.
Para Hound conservar nombres y describir ejes/rest pose; los 53 huesos actuales
son diagnósticos. Los 43 nombres Legacy no autorizan copiar sus clips.

TRS LINEAR/STEP; CUBICSPLINE, morph targets, Draco y animación de weights se
rechazan. Nodos animados en TRS, tiempos estrictamente crecientes, valores
finitos y cuaterniones unitarios. Constraints/IK/controladores se hornean:
fixture de dos huesos/dos pesos y rotación horneada de 31 claves LINEAR cocinada.
Eso prueba transporte. El hito 102 reproduce `Hound16_joint_check` en GPU en un
cuerpo TPS y muestra dos poses distintas; **no valida clips jugables ni
equivalencia de todas las poses**. La evaluación CPU existente y los rigs
originales pasan sus tests. El fallo con siete Hound se corrigió en el 104;
su prueba mantiene animación diagnóstica y sombras, sin certificar clips finales.

| Estado / evento | Entrada que exige hoy el runtime | Consecuencia para H08/H11/H12 |
| --- | --- | --- |
| Reposo | Clip `idle`, bucle | Debe existir con ese nombre; clip ausente vuelve silenciosamente a rest pose. |
| Avance/retroceso | `forward`, bucle; retroceso invierte el tiempo | Crear para el nuevo rig; sin root motion autoritativo. |
| Strafe derecha/izquierda | `strafe_right`; izquierda invierte el tiempo | Probar diagonales y mezcla con avance. |
| Aire/salto | `jump`, sin bucle; muestra hasta 0,3 s | Verificar aire, salto y esquiva; no hay clip separado de dodge seleccionado hoy. |
| Apuntar/respirar/aterrizar | Capas procedurales en spine/neck y raíz visual | Probar ejes nuevos y mirada arriba/abajo; pelvis `Bip001` anclada al bind. |
| Disparo/daño/equipar | Recoil, reacción y bajada de arma procedurales | No añadir nombres de clips suponiendo que se reproducirán automáticamente. |
| Muerte/respawn/teleport | Congela última pose, caída procedural y corte temporal | Probar historial de skin/transform, cadáver quieto y reinicio. |
| Bite/Guard | Presentación FPS oculta brazos/arma durante primaria activa | Mordida facial no está exigida ni controlada por el runtime actual. |
| Berserker | Estado/habilidad y emisor `hound_odor` | Conservar presentación compatible; no cambiar reglas/cooldowns. |

Anclajes por nombre: `Bip001`, `Bip001 Spine`, `Bip001 Neck`,
`Bip001 Head`, `Bip001 L/R UpperArm`, `Forearm`, `Hand`, dedos
identificados por `Finger`, y clavículas. La extracción FPS conserva triángulos
enteros cuyos tres vértices tienen >95 % del peso en UpperArm/Forearm/Hand/Finger.
No existe rig facial/morph compatible ni simulación de tela en este contrato.

### Armas y sockets vigentes

| Arma / escena | Acciones que deben revisarse con el agarre |
| --- | --- |
| Soul Reaper — `characters/original/soul_reaper.gltf` | Rayo principal y tirón secundario; apoyo diagnóstico H05 inválido. |
| Sniper — `legacy/sniper.gltf` | Disparo principal y secundario expansivo. |
| ShotGun — `legacy/shotgun.gltf` | Proyectiles y retorno secundario. |
| MiniGun — `legacy/minigun.gltf` | Ráfaga principal, carga/descarga secundaria. |
| IronHellGoat — `legacy/iron_hell_goat.gltf` | Carga de bola, disparo y redirección. |

Inventario: [ARSENAL](../../ARSENAL.md), contrastado con código vigente.
El animador usa la mano derecha como origen del arma, IK hacia dos puntos
comunes y ajustes de agarre/boca basados en Soul Reaper **para todas las armas**.
No consume sockets glTF nombrados por arma. H08 debe resolver mano/palma/pulgar,
apoyo izquierdo, orientación y muzzle FPS/TPS para las cinco; si exige offsets
por arma, hace falta una tarea de presentación aparte. No certificar el agarre
solo por coincidir los nombres de hueso. H05 conserva los límites de contactos
de [v16](../../../reports/hound-armor-99/contacts.md).

## Historial 101 — medidas y presupuesto entonces pendiente

[Datos por material y proyección](../../../reports/hound-contract-101/asset-measurements.json).
Cocción vigente de v16, sin cambiar el modelo:

| Medida | Valor |
| --- | ---: |
| Triángulos LOD0 | 80.152 |
| Vértices tras splits/optimización del cooker | 50.392 |
| Primitivas/materiales | 7 / 7 |
| LOD automáticos del cooker | 40.073 / 16.023 triángulos |
| Brazos FPS extraídos | 18.108 triángulos |
| Huesos / influencias máximas | 53 / 2 |
| Imágenes propias | 0; todavía no hay texturas finales |
| Archivo cocinado | 7.021.345 bytes |
| Buffers geométricos calculados para LODs y brazos | 22.340.272 bytes ≈21,305 MiB |
| Ocho cuerpos LOD0 / siete cuerpos + brazos FPS | 641.216 / 579.172 triángulos, sin armas/sombras |

El cálculo de buffers aplica el layout real de 104 bytes/vértice y uint32 en
índices. El puente duplica vertex buffers para LODs/brazos; cuenta esas copias.
No es lectura de VRAM del driver ni incluye alineación, texturas, paletas o
buffers de frame. Ocho copias del mismo recurso comparten geometría/texturas;
skins diferentes pueden multiplicar materiales/mapas. Paleta de 53 huesos:
6784 bytes de matrices+normales por pose CPU; el 104 mide ocho cargas de 10.176 bytes GPU (actual/anterior/normales), más reserva estática: 432 KiB reservados por frame.

El visor estático a 1280×720 envía 56 primitivas de ocho copias: 54 visibles,
9 batches, LODs 35/12/7; tamaños completos proyectados de 94–232 px de alto.
Hay recorte de pantalla en las copias cercanas. Esto **no** representa ocho
combatientes animados visibles en Factory ni demuestra rendimiento a 1080p.
Los lotes agrupan material/malla/LOD; no equivalen al total de draws de todas
las pasadas. Los LOD automáticos consideran posición, no error de deformación;
no aceptarlos como LODs de producción sin revisar movimiento/silueta.
El importador no establece una convención para consumir LODs glTF de autoría.

**En el 101 no había presupuesto de producción para LOD0/LODs, materiales o mapas.**
Faltaban mediciones animadas. Como comparación de memoria,
cuatro mapas BC5/BC7 cuadrados con mips completos ocupan aproximadamente
5,33 MiB a 1K, 21,33 MiB a 2K o 85,33 MiB a 4K por conjunto, sin alineación.
Es una comparación, no selección de resolución o autorización para texturizar.

Factory Release actual (dos combatientes y presentación original):
dos pasadas nativas 1080p de 360 muestras tras 120 de calentamiento,
sin VSync ni resolución dinámica: **144,17 / 144,19 FPS**, medias
6,936 / 6,935 ms, p95 6,955 / 6,953 ms. Draw/Present concentra ~6,41 ms.
La cadencia estable sugiere regulación de presentación; no prueba saturación de
la GTX 1070. Las últimas consultas GPU (2,345 / 3,116 ms) no son una media completa.
Control 720p: 530,77 FPS; no sustituye el objetivo. [Medidas](../../../reports/hound-contract-101/benchmark.json).
HUD/audio, poll_events y arranque quedan fuera del intervalo del benchmark.

## Historial 102 — resultado y trabajo pendiente en ese cierre

Mantener 200 FPS/1080p/8 en este equipo. El [hito técnico 102](../../../reports/eight-combatants-102/README.md)
midió siete Archangel TPS animados más FPS local en Factory, con HUD, salida de
audio nula y disparo. Ocho instancias skinned son visibles en el frustum. Una
pasada rápida obtuvo 283,65 FPS de media, pero p95 5,259 ms y p99 5,441 ms;
otra reprodujo ~144,15 FPS con p95 6,993 ms. La espera de `end_frame/Present`
domina en la cadencia de 144; GPU completa ~2,3–2,4 ms. La misma cadencia aparece
con dos combatientes. No se aprueba el objetivo por la media rápida.

Un Hound v16 y su clip diagnóstico se reprodujeron en GPU. Su skin/bounds CPU
costó 0,982 ms de media. Siete Hound v16 fallaron antes de medir por
`Skin constant mapping failed`; el contenido
jugable final aún carece de mapas y clips. La muestra de ocho usa geometría
original compartida, por lo que no fija presupuesto LOD0/LODs/materiales/mapas
para Hound ni decide si son necesarios LODs de autoría.

## Historial 104/105 — presupuesto aún pendiente en esos cierres

**Rectificación 106:** las 30 evicciones de carga del 104 dejaban mallas del
escenario sin dibujar. Las 53 skins visibles no probaban la integridad de Factory.
Se conservan las cifras históricas, pero no validan la escena completa ni fijan
el presupuesto vigente. La prueba actual cuenta recursos ausentes y restaura
584 draws/436 de sombras en v16, frente a 527/406 del 104.

El [104 ejecutado](../../../reports/hound-runtime-104/README.md) resuelve los
bloqueos de mapeo y presentación del 102. Siete Hound v16 más FPS completan
el benchmark con sombras y animación; 53 skins pasan el frustum en todos los
frames medidos. Paletas por actor/nodo y pareja actual/anterior: 9 mapeos,
442.368 bytes reservados y 81.408 copiados por frame, heap original de 8 MiB.
Bounds conservadores por hueso: 0,106–0,124 ms de skin/bounds, frente a 3,203 ms.

MAILBOX reproduce 144 FPS esperando ~5,4 ms al adquirir imagen; IMMEDIATE es
ahora la prioridad con VSync off. VSync on conserva sincronización. Dos imágenes;
finalización de cola medida como cota superior observada por fence en CPU,
no latencia de pantalla. Modo efectivo, CPU/GPU y límites en el informe.

Dieciséis pasadas Release/1080p, 120 frames de calentamiento y 360 medidos por
pasada: dos/ocho originales, un/siete Hound más FPS; dos repeticiones con salida
nula y dos con SDL3 Headphones (High Definition Audio Device), HUD y disparo.
**0/5.760 frames >5 ms**. Siete Hound: **281,29–283,98 FPS**, p99 máximo
**3,854 ms**, máximo **3,974 ms**. [Todas las medidas](../../../reports/hound-runtime-104/mediciones.md).
El control antiguo de dos solo tiene una skin dentro del frustum al final;
no usarlo para atribuir un coste exacto por personaje. Ocho originales: 8/8;
siete Hound más FPS: 53/53, 46 batches, 527 draws, 406 de sombras al final.

El 104 dio por comprobados 200 FPS **para ese escenario diagnóstico**, con la
salvedad de integridad descubierta arriba. **H06 quedó bloqueada entonces**:
v16 carece de mapas y clips jugables finales, falta coste de las cinco armas y
combate autoritativo de ocho, y residencia de assets alcanza **511,73/512 MiB**.
Cero evicciones durante medidas, 30 en carga/calentamiento; no es VRAM total.
No fijar aún LOD0/LODs, materiales o mapas a partir de ese margen.

Siguiente entrada para un encargo posterior: mantener v16 y la regresión del
104 como control, medir contenido de producción cuando exista y resolver
residencia antes de aprobar límites H06. No requiere reabrir normales ni
modificar escultura. Release 7/7, Debug 4/4 y hashes exactos. **H07 no se inicia**.
Commit local de ejecución del 104, sin push.

H07 quedó [preparada documentalmente](tasks/H07-malla-produccion.md) en el 105,
sin iniciar producción. Su entrada debe comprobar las decisiones de presupuesto
anteriores; si faltan, limitar la nueva tarea a auditoría y plan. Esta preparación
no fijó cifras, no cerró H06 y conservó v16 exacta. El 106 sustituye esa dependencia
pendiente por el presupuesto y las verificaciones del inicio de este documento.
