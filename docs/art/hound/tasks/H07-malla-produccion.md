# H07 — malla de producción y LODs

## Entrada y alcance
Leer [contexto](CONTEXTO.md) e [índice](README.md).
Requiere H05 **aprobada** y contrato/presupuesto H06; abrir sus salidas exactas.
Conservar la escultura maestra de H05 como referencia para comparación/horneado.

Preparar retopología solo donde haga falta, loops de articulación, densidad útil,
normales y fronteras rígido/deformable. Retirar geometría oculta solo tras
comprobar que no aparece en ninguna pose/encuadre previsto; no soldar armadura
articulada al cuerpo por reducir objetos. Respetar el presupuesto acordado.
Producir los LODs que H06 justifique, sin imponer cantidades o porcentajes arbitrarios.

## Fuera de alcance
Rediseño artístico, nuevas texturas, rig definitivo y cambios en runtime.
No prometer “manifold en todo” como sustituto de topología adecuada: justificar
bordes abiertos/cortes FPS si son intencionales y compatibles con el contrato.

## Comprobaciones y entrega
- Comparación con escultura en cámaras fijas y silueta/distancias de uso.
- Normales, degenerados, escalas, materiales y deformación con pesos provisionales.
- Recuentos por LOD, discrepancias visibles y coste respecto al presupuesto;
  roundtrip y visor. No borrar detalle crítico de cara/manos/silueta.
- Fuente de producción nueva y LODs, exportación de prueba, tabla de correspondencia
  con la escultura y lista de bordes/separaciones intencionales. Cierre común.

## Criterio de cierre
Silueta/detalle relevante conservados dentro del presupuesto y malla apta para
skinning/UV. H08 puede pulir pesos sin rehacer estructura.
Si cambia la silueta aceptada, volver a revisión artística, no ocultarlo como optimización.
