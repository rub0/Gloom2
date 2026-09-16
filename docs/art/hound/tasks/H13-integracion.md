# H13 — integración jugable y aceptación final

## Entrada y alcance
Leer [contexto](CONTEXTO.md) e [índice](README.md).
Requiere H10 y H12, presupuesto/contrato H06 y LODs/rig vigentes de H07/H08.
Comprobar que el acabado/animaciones tienen aceptación o pedir la decisión pendiente.
Consultar `include/gloom/gameplay/character_presentation.hpp`,
`src/gameplay/character_animation.cpp` y localizar sus consumidores con búsqueda acotada.

Sustituir únicamente la presentación de Hound por el recurso terminado y cocinado.
Mantener intacto Archangel, cuyo cuerpo comparte hoy Hound: no sobrescribir su asset.
Conectar skins/clips, materiales, sockets, brazos FPS/cortes, selección y variantes
de Hound. Conservar gameplay, IDs, protocolo, cápsula, cámara, audio y HUD.
Comprobar dependencias de cocción/empaquetado y comportamiento de error/fallback existente.

## Validación y entrega
- Juego real FPS/TPS bajo luz de Factory: locomoción, armas, habilidades, muerte/
  respawn, cambio de identidad, cámara extrema y otros personajes simultáneos.
- Skinning/sombras/normales, motion vectors e historial tras cortes, bounds/culling,
  LODs/transiciones y carga de texturas sin rutas locales absolutas.
- Build de objetivos afectados, tests de assets/GPU/animación/personajes y red
  que corresponda; seleccionar desde CTest actual. Prueba local y host/join.
- Medición Release en el escenario H06: tiempos CPU/GPU, memoria/residencia,
  draw calls y coste con personajes; comparar con presupuesto y presentar cifras.
  Las secuencias a timestep fijo no son un benchmark.
- Capturas/vídeos reales, informe de resultados y matriz de pendientes; conservar
  originales y una vía de reversión por commits, sin borrar historia.

## Criterio de cierre
Hound se utiliza realmente en partida, aspecto y animación aceptados, sin
regresiones detectadas en la matriz y dentro del presupuesto acordado.
Si falta prueba humana o una medida, declararla y dejar el estado en revisión,
no “producción terminada”. Actualizar índice/estado y commit local; sin push.
