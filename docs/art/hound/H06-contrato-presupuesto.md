# Hound H06 — contrato comprobado y presupuesto pendiente

26 de septiembre de 2026 · Subhito 101 · **H06 bloqueada; no habilita H07**.
Mediciones realizadas el 23 de septiembre sobre el código vigente del hito 100.

Entrada inmutable: [escultura v16](../../../art/characters/hound/v16/hound-mesh-v16.blend)
y [glTF/BIN v16](../../../assets/characters/hound_rig/v16/hound-rig.gltf).
H05 está aprobada; este documento no modifica esa aprobación ni el modelo.
[Pruebas, reproducción y decisiones pendientes](../../../reports/hound-contract-101/README.md).

## Objetivo confirmado por el usuario

**200 FPS, 1920×1080 nativo, hasta 8 combatientes, en este equipo:**
Ryzen 7 3700X, GTX 1070, 17.082.138.624 bytes de RAM, Windows,
controlador 32.0.15.8129. Son **5 ms por fotograma para el conjunto**, no para cada Hound.
No se acordó una resolución de texturas. No sustituir este objetivo por los
100 FPS de informes anteriores ni por los Hz de simulación.

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
| Normal | **Bloqueada.** El mapa es lineal, tangente, XY en RG y Z positivo reconstruido por shader. La ruta actual desplaza G a A durante la compresión; el relieve Y no llega correctamente a GPU. No producir bakes finales antes de corregir y repetir pruebas X/Y y costuras. |
| Tangentes | El importador ignora TANGENT de glTF y regenera MikkTSpace desde UV0/normales. Invertir las tangentes exportadas deja el payload cocinado idéntico. Normal maps en UV0; usar UV1 para una normal no recalcula su base tangente. |
| UVs | Solo UV0/UV1. Dos distribuciones distintas comprobadas en GPU. UV2 o textura que referencia UV ausente se rechazan. Sin UDIM/tiles automáticos. |
| Imágenes | PNG externo comprobado; imágenes embebidas rechazadas. No compartir una imagen entre color, datos y normal. Sí compartir R/G/B de ORM como datos; no empaquetar normal con ORM. |
| Compresión/mips | Cooker KTX2 UASTC con cadena completa. Runtime BC7 sRGB para color/emisión, BC7 lineal para ORM y BC5 para normal si hay soporte, RGBA8 de respaldo. El respaldo de normales tampoco está certificado. |
| Alpha | OPAQUE por defecto para cuerpo/placas. MASK y BLEND exportados de Blender y visibles en GPU; cutoff 0,5 en muestra. Transparencia es un coste adicional; no usarla para simular huecos de armadura. El visor no certifica sombras/orden en todas las escenas. |
| Materiales Blender | Principled y enlaces exportables. Los nodos de viewport no son contrato de runtime. Factores glTF lineales, sRGB solo en color/emisión. |

La prueba de normal **detecta y deja visible el defecto vigente**;
no se cambió la referencia para convertirlo en aprobado. Los otros diez
controles de imagen pasan; que X produzca un cambio tampoco certifica su dirección
correcta. [Resultados versionados](../../../reports/hound-contract-101/results.md).
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
Eso prueba transporte; **no valida reproducción animada GPU ni equivalencia de
todas las poses**. La evaluación CPU existente y los rigs originales pasan sus
tests; la reproducción GPU del nuevo rig queda pendiente.

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

## Medidas y presupuesto

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
6784 bytes de matrices+normales por pose CPU; su coste por frame aún no medido.

El visor estático a 1280×720 envía 56 primitivas de ocho copias: 54 visibles,
9 batches, LODs 35/12/7; tamaños completos proyectados de 94–232 px de alto.
Hay recorte de pantalla en las copias cercanas. Esto **no** representa ocho
combatientes animados visibles en Factory ni demuestra rendimiento a 1080p.
Los lotes agrupan material/malla/LOD; no equivalen al total de draws de todas
las pasadas. Los LOD automáticos consideran posición, no error de deformación;
no aceptarlos como LODs de producción sin revisar movimiento/silueta.
El importador no establece una convención para consumir LODs glTF de autoría.

**No hay presupuesto de producción aprobado para LOD0/LODs, materiales o mapas.**
Fijar números ahora no garantizaría el objetivo. Como comparación de memoria,
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

## Decisión necesaria para desbloquear H06

Mantener 200 FPS/1080p/8 en este equipo. Proponer trabajo técnico separado,
antes de continuar H06 o H07:

1. **Normales:** corregir el contrato UASTC → BC5/RGBA para conservar X/Y
   independientes; repetir controles X+, Y+ y costuras/UV reflejadas. Alternativa
   compatible actual: materiales sin normal map, que sacrifica el bake previsto.
2. **Medición:** aislar la regulación de Draw/Present a 1080p y habilitar una
   muestra de ocho presentaciones animadas, con cámara/distancias registradas,
   sin ampliar todavía la autoridad/red de dos jugadores. Medir CPU de poses/
   bounds, GPU, draws/sombras, residencia, HUD/audio y margen dentro de 5 ms.
   Incluir la fixture horneada y el clip diagnóstico v16 en reproducción GPU.
3. Con esos resultados, fijar los límites de geometría/materiales/mapas y
   decidir si bastan LODs compatibles actuales o hace falta consumo de LODs
   de autoría. No iniciar retopología/texturas con cifras inventadas.

La [ficha H06](tasks/H06-contrato-presupuesto.md) excluye modificar motor,
gameplay o modelo. Este subhito entrega diagnóstico y propuesta; no ejecuta
esas correcciones ni inicia H07. No falta una respuesta sobre FPS/equipo/cantidad:
esos datos ya están resueltos.
