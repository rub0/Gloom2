# Correcciones de gameplay y HUD — 5 de septiembre de 2026

Correcciones solicitadas antes de continuar la migración de recogibles. Se conserva el trabajo previo del repositorio.

## Cambios

- Vida y escudo en barras verticales exteriores, con cruz y escudo recortados de Hud-83.png. Cinco iconos de armas originales; los no adquiridos se atenúan. Se contrastaron con los PSD originales para identificar cada arma.
- La habilidad implementada ocupa el círculo superior izquierdo, con progreso de cooldown. Los demás slots aún no tienen habilidades implementadas.
- FPS reales promediados cada 0,5 segundos y coordenadas XYZ del jugador arriba a la izquierda.
- Doble Espacio en una ventana de 450 ms, con dirección de movimiento: primer toque salta, segundo esquiva. Una esquiva aérea por salto; WASD doble continúa funcionando. Repetición automática y pérdida de foco no generan pulsaciones extra.
- CharacterVirtual ya era un controlador virtual. GetLinearVelocity conserva la velocidad solicitada: ahora se proyecta contra las normales de contactos reales, ignorando sensores. Se conserva la componente tangencial y se elimina la componente hacia techo/pared. Se elimina el sesgo vertical artificial.
- Al soltar movimiento sobre suelo apoyado se detiene la velocidad. Se mantiene la inercia de vuelo y esquiva hasta el aterrizaje.
- Lava letal en el primer tick de contacto, incluso con escudo. Reutiliza la muerte y reaparición existentes.
- Relleno ambiente 0,65, conservando el probe; intensidad direccional y puntual +20 %. La contribución ambiente añadida no queda anulada por el lightmap antiguo.
- Protocolo 17: disponibilidad de esquiva aérea en snapshots de gameplay, completos y deltas de movimiento. Nueva huella de simulación de Factory.

## Validación

Compilación Debug completa correcta (`build-all.log`). 43 pruebas validadas: la pasada completa dio 40 aprobadas y 3 diferencias visuales esperadas por el cambio solicitado (`tests-all.log`, 226,67 s). Tras inspeccionar las capturas y actualizar únicamente las referencias de HUD afectadas y la iluminación de Factory/personajes, las 3 comparaciones pasan (`tests-visual-final.log`, 63,94 s). No se relajaron umbrales ni controles negativos.

Regresiones: techo con cápsula Jolt real, velocidad tangencial, descenso al tick siguiente, reposo de 600 ticks en Factory, frenado al soltar, segundo Espacio a los 250 ms de salto, rechazo de otra esquiva aérea, ventana de 450 ms, repetición/foco, flags de red completos/delta y lava sin daño gradual. La suite incluye servidor dedicado y flujo de dos clientes con pausa/reconexión.

La regeneración del atlas coincide byte a byte: 117 archivos, 20 fuentes verificadas y 17 SWF (`ui-import.log`). Revisión de espacios del código/documentación sin errores (`diff-check.log`, respetando CRLF).

## Evidencia visual

Se inspeccionaron los dos vídeos aportados; los contactos resumen están en `10-27-46.jpg` y `10-28-20.jpg`. Los vídeos no contienen registro de teclas; el caso sin entrada se valida con simulación de entrada neutra explícita.

- `hud-1280.png`: fixture determinista de HUD. Los 120 FPS son un valor de prueba, no una medición de rendimiento.
- `hud-states.png`: HUD a 1280, 1920 y ultrawide 2560, muerte y pausa.
- `factory-hud.png`: captura real del flujo de dos clientes Debug; el contador de FPS es medido. No constituye un benchmark de ejecución individual.
- `factory-lighting.png`: seis vistas de Factory, incluidos pasillos, techo y lava.
- `character-lighting.png`: personajes y arma bajo la iluminación ajustada.
- `original-weapons.png`: identificación de las armas desde los PSD del proyecto original.

Las capturas completas se regeneran en `build/windows-vs/ui-review/Debug`, `factory-review/Debug`, `character-review/Debug` y `ui-flow/Debug`. La verificación de colisiones fue automatizada; no se afirma haber repetido manualmente las pulsaciones de los vídeos.
