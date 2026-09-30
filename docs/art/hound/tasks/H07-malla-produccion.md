# H07 — malla de producción y LODs

30 de septiembre de 2026 · **Encargo preparado con presupuesto H06; producción no iniciada.**
H05 aprobada y H06 cerrada como entrada técnica en el 106. Esta ficha ya dispone
de límites medidos; ejecutarla requiere el encargo de H07 en una tarea posterior.

## Lectura mínima y entrada exacta

Leer primero `docs/ESTADO_ACTUAL.md` y `AGENTS.md`; después el inicio del
[índice](README.md), [CONTEXTO.md](CONTEXTO.md) y esta ficha. No reconstruir el chat.
Referencias necesarias:

- [Contrato H06](../H06-contrato-presupuesto.md): presupuesto vigente 106, convenciones y rig.
- [Informe 106](../../../../reports/hound-budget-106/README.md): método, pruebas, límites y reproducción.
- [V16 aprobada](../mesh-v16/README.md) y [contactos conocidos](../../../../reports/hound-armor-99/contacts.md).
- [Fuente maestra](../../../../art/characters/hound/v16/hound-mesh-v16.blend),
  [glTF](../../../../assets/characters/hound_rig/v16/hound-rig.gltf),
  [BIN](../../../../assets/characters/hound_rig/v16/hound-rig.bin) y
  [manifiesto SHA-256](../../../../art/characters/hound/v16/sculpture-reference.json).

Escena `Hound_Mesh_v16`, malla `H16_DeformMesh`, rig `Hound16_Rig`,
acción `Hound16_joint_check`, colección `HOUND_v16_EXPORT`.
Conservar esos archivos byte a byte y trabajar en una versión nueva libre;
registrar sus rutas y nombres, sin asumir que el número H07 determina la versión.
El manifiesto conserva el estado histórico del modelado; la aprobación posterior
es la del [hito 100](../../../../reports/hound-approval-100/README.md).

## Requisito de entrada: presupuesto H06

El 104 (`3b5925c`) corrige skinning/Present; el 106 recupera residencia y mallas
del escenario activando BC5/BC7. Sus medidas sustituyen la referencia incompleta del 104.
La referencia tiene 80.152 triángulos de autoría, siete materiales, 53 huesos,
máximo dos influencias actuales y ninguna imagen propia. Son medidas, no límites.
Prueba 106 con mapas/clips/cinco armas: 355,43/512 MiB, sin evicciones o recursos
ausentes; perfil de 40.073 TPS/18.108 FPS a 296,77–298,76 FPS, p99 máximo
3,914 ms, máximo 4,503 ms, 0/1.440 frames >5 ms con audio nulo/real.
Se mantiene el objetivo de **200 FPS / 5 ms, 1920×1080 nativo, hasta ocho
combatientes, Ryzen 7 3700X y GTX 1070**.

Antes de editar geometría, leer los límites completos de H06 y verificar que
siguen vigentes. Resumen de entrada:

| Decisión de H06 | Qué necesita H07 |
| --- | --- |
| Geometría TPS/FPS | LOD0 ≤40.000 triángulos TPS; brazos ≤18.000. ≤54.000 vértices cocinados TPS y ≤24 MiB geométricos contando copias de LODs/FPS. |
| Materiales y memoria | ≤7 primitivas/materiales TPS y ≤4 FPS. Atlas de hasta cuatro mapas 2K por skin, pool de hasta ocho conjuntos ≤192 MiB; residencia de escena objetivo ≤384 MiB. No crear mapas finales en H07. |
| Rig provisional | 53 huesos y máximo dos influencias en la carga medida. Conservar rig/acción para comparar; cualquier aumento necesita nueva medida en H08. |
| LODs y distancia | Ruta automática del cooker: objetivos 50 %/20 %, error 0,01; distancia/radio por primitiva 30/80. H07 debe comprobar silueta/deformación y transiciones, no solo recuentos. |
| Alcance de la medida | Siete TPS + FPS, geometría compartida, mapas/UVs y clips diagnósticos, cinco armas. Cuatro pasadas 1080p/120+360 frames, audio nulo/real; p99 ≤4 ms, máximo ≤5 ms, cero evicciones/recursos ausentes. |

El importador no tiene una convención comprobada para consumir LODs glTF de
autoría. No exportar varios objetos superpuestos y afirmar que son LODs activos.
Los LODs automáticos de v16 tampoco certifican calidad de deformación. Los
porcentajes anteriores son la ruta existente: puede conservar más caras para
respetar el error. Si su calidad no basta, dejar la alternativa pendiente antes de H08.

El presupuesto permite producir; no hace falta esperar a UVs/bakes/clips finales
para iniciar retopología. Si aparece una regresión que invalida esta entrada,
documentarla y resolver su alcance antes de modificar assets. No ampliar H07
a memoria del motor, mapas/clips finales o integración autoritativa.

## Trabajo una vez satisfecha la entrada

Preparar retopología solo donde haga falta, loops de articulación, densidad útil,
normales y fronteras rígido/deformable. Retirar geometría oculta solo tras
comprobar que no aparece en ninguna pose/encuadre previsto; no soldar armadura
articulada al cuerpo por reducir objetos. Respetar el presupuesto acordado.
Validar los LODs automáticos definidos en H06 con el candidato nuevo.

Conservar rostro, manos/garras, hombros libres, caída de capucha y cobertura
delantera/posterior/lateral de v16. Inventariar los 106 componentes y distinguir
detalle que necesita geometría de detalle transferible al horneado posterior;
no borrar de la maestra el relieve que H09 necesitará.

Transferir pesos provisionales para comprobar la nueva topología, conservando
rig, bind pose y acción diagnóstica como control. H08 resolverá rig/pesos/agarres
definitivos. Dejar geometría apta en codos, cuello/capucha, abdomen y manos;
comparar los contactos heredados sin exigir que sus recuentos de caras coincidan
tras retopología. El apoyo Soul Reaper de v16 sigue siendo inválido.

## Fuera de alcance

Rediseño artístico, nuevas texturas, rig definitivo y cambios en runtime.
No prometer “manifold en todo” como sustituto de topología adecuada: justificar
bordes abiertos/cortes FPS si son intencionales y compatibles con el contrato.
No iniciar H08/H09, integrar el candidato como personaje del juego ni modificar
gameplay/red. No reabrir normales o el arreglo 104 sin una regresión concreta.

## Comprobaciones y entrega

- Reabrir la fuente final; comprobar hashes intactos de v16 y versiones previas.
  Comparar frente, perfil, espalda, tres cuartos y detalles a igual cámara/escala,
  además de siluetas a distancias de uso y vistas FPS. Inspeccionar imágenes reales.
- Comprobar normales, degenerados, escalas aplicadas, materiales, pesos finitos
  normalizados y separación rígido/deformable. Cubrir el clip diagnóstico completo
  y poses extremas/funcionales pertinentes; indicar cuáles y sus límites.
- Exportar/reimportar glTF 2.0 separado + BIN; comparar poses y silueta en cada LOD.
  Respetar extracción FPS y demás convenciones de H06. El visor estático comprueba
  transporte/apariencia, no toda la animación o rendimiento del juego.
- Entregar tabla por componente/material/LOD: vértices de autoría y cocinados,
  triángulos TPS/FPS, primitivas, huesos/influencias y memoria de geometría;
  diferencias frente a v16 y al presupuesto. Justificar LODs y separaciones.
- Medir el candidato en runtime y demostrar que se carga esa nueva ruta.
  La regresión 104 carga v16 y exige 53 primitivas: pasarla conserva el control,
  pero no mide una malla nueva. No relajar sus aserciones para ocultar una pérdida.
  Aplicar el protocolo y carga diagnóstica del 106 en serie/Release/1080p y
  registrar distribuciones, visibilidad, animación, sombras, cinco armas y residencia.
  Adaptar solo la entrada de la prueba a la nueva ruta y su número justificado
  de primitivas; no confundir el LOD1 forzado de v16 con el LOD0 de producción.

Reutilizar `tools/art/verify_hound_armor_v16.py` y
`tools/art/review_hound_h05.py -- --v16 --audit` para el control v16;
`tools/art/verify_hound_rig_roundtrip_v03.py -- v16` ya reconoce esa versión.
Son verificadores específicos: adaptar solo lo necesario para la nueva topología
y repetir la regresión de v16 si se modifican herramientas compartidas.
Recetas de arranque/exportación en [BLENDER_WORKFLOW.md](../../../BLENDER_WORKFLOW.md)
y [reproducción v16](../../../../reports/hound-armor-99/README.md#reproducción).

Salida: fuente de producción nueva, exportación de prueba y LODs justificados;
tabla de correspondencia con la escultura, bordes/separaciones intencionales,
evidencias visuales y lista de problemas que H08 debe resolver. Guardar las rutas
exactas en el índice, junto al informe del siguiente hito libre. No versionar
cachés, renders regenerables, logs de build ni `.blend1`.

## Criterio de cierre

Silueta/detalle relevante conservados dentro del presupuesto y malla apta para
skinning/UV. H08 puede pulir pesos sin rehacer estructura.
Si cambia la silueta aceptada, volver a revisión artística, no ocultarlo como optimización.
Auditoría o propuesta sin presupuesto no equivalen a H07 terminada.
Actualizar informe, índice y ESTADO_ACTUAL; ejecutar las comprobaciones pertinentes,
crear y verificar el commit local. No hacer push ni empezar la siguiente ficha.
