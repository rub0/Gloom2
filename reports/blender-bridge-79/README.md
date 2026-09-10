# Hito 79 — puente local Blender–Gloom

10 de septiembre de 2026. Continuación tras la aprobación de la guía artística
y el boceto 01 de Hound. Instalación iniciada el 9 y validación cerrada el 10.

## Resultado

- Blender portable 4.5.13 LTS, ZIP oficial con SHA-256 verificado; Python aislado
  3.12.10, Blender MCP 1.9.1 y SDK MCP 1.30.0. No se modificó Blender personal.
- Addon incluido en el paquete fijado, versión 1.6/protocolo 5; perfil y scripts
  bajo `.cache/blender-profile/`. Lanzador y cliente de diagnóstico reutilizables.
- Registro MCP local del proyecto, cuatro herramientas permitidas, modo seguro,
  telemetría desactivada y proveedores externos apagados. Sin API keys ni compras.
- Fixture estática generada por script a través de execute_blender_code: placa
  biselada, panel con checker de 8 × 8 y esfera emisiva. 424 triángulos en 3 mallas.
- Cocción de glTF con dependencia PNG externa y carga en Diligent/Vulkan reales.
- Modo opcional de captura del visor con salida normal; no se cambia el motor,
  sus shaders, el personaje jugable, las colisiones ni la simulación de Gloom.

La [guía del flujo](../../docs/BLENDER_WORKFLOW.md) contiene versiones, checksum,
fuentes, comandos reproducibles, seguridad y la siguiente puerta de validación.
La [guía artística](../../docs/DIRECCION_ARTISTICA.md) registra ahora la aprobación
del usuario; no se ha regenerado ni alterado la lámina aceptada.

## Verificación realizada

| Comprobación | Resultado |
| --- | --- |
| Blender `--version` y checksum oficial | 4.5.13 LTS, hash coincidente |
| MCP initialize / list_tools / get_scene_info | Servidor y addon conectados a localhost:9876 |
| MCP execute_blender_code | Escena guardada y exportación completada; filtro seguro activo |
| MCP get_viewport_screenshot | PNG recibido, guardado e inspeccionado |
| `codex mcp list` | blender habilitado; sin autenticación OAuth necesaria |
| `pip check` y compileall de tools/art | Sin dependencias rotas ni errores de sintaxis |
| Lanzador PowerShell | Sintaxis válida y segunda instancia bloqueada si 9876 ya está ocupado |
| `verify_bridge_probe.py` | 3 mallas, 424 triángulos, 3 materiales, PNG externo, normales/UV0, bounds de buffers y escala/Y-up |
| Build Release | Visor, cooker, gloom_asset_tests y gloom_gpu_asset_tests correctos |
| CTest focalizado | gloom.assets y gloom.gpu_assets: 2/2 |
| Cooker sobre exportación Blender | Scene 15720082031566676004, una dependencia externa |
| Visor real en Release | Captura 1280 × 720, 3/3 visibles, 3 batches; salida 0 y cierre del renderizador |
| CLI inválida del visor | Sin argumentos: 2; cero copias con captura: 1, antes de abrir ventana |

Se inspeccionaron el viewport sólido de Blender y la captura del visor: placa,
panel y esfera en orientación correcta, metal con reflejos y emisión naranja.
La textura es deliberadamente minúscula y la cámara del visor generalista queda
lejos; esto no es una evaluación del acabado artístico ni una referencia visual
de regresión aprobada. La prueba CTest GPU-assets valida los bindings; la evidencia
de renderizado real es la ejecución adicional del visor, no ese test aislado.

Logs y medios locales regenerables (no versionados):

- `.cache/blender-setup/mcp-check.log` y `mcp-export.log`.
- `.cache/blender-setup/viewer.log`.
- `.cache/blender-bridge/bridge-probe.blend`.
- `.cache/blender-bridge/blender-viewport.png`.
- `.cache/blender-bridge/gloom-preview.ppm` y su conversión PNG para inspección.

La fixture fuente `.gltf`, `.bin` y PNG sí está versionada en
`assets/tests/blender_bridge/`. Los scripts permiten reconstruir su fuente Blender.

## Incidencias y límites

El primer intento GLB falló porque el cooker no soporta imágenes embebidas.
Se cambió a `GLTF_SEPARATE`; la muestra GLB descartada se conserva en `.cache`
y no se distribuye como formato funcional. El directorio del montaje de cache
debe existir antes de llamar al cooker. No se han relajado esas validaciones.

El helper de sandbox y el proceso de Computer Use fallaron al reiniciar.
Tras el reintento indicado por la skill no se automatizaron ventanas por vías
alternativas. Se usaron exportación/captura del propio Blender MCP y captura
del renderizador Gloom, más comandos aprobados para instalación y archivos.
El fallo de lectura local del visor de imágenes se resolvió mostrando los PNG
ya generados, sin introducir un capturador de escritorio alternativo.

El MCP está registrado, pero el manifiesto de herramientas de esta tarea todavía
no contiene Blender. El cliente stdio de diagnóstico ha probado las llamadas
reales con modo seguro; no equivale a verificar su exposición nativa en Codex.
Recargar los servidores o reiniciar la app si no aparecen en la próxima sesión.

El cambio del visor añade dos contadores y ramas solo en modo de revisión; no
introduce asignaciones por frame ni cambia el recorrido interactivo existente.
La salida conserva el drenaje y la destrucción habituales del renderizador.
No se repite la suite completa ni se valida animación, skinning, LODs de Hound,
materiales finales o presupuesto de combate. El resultado es el puente técnico,
no un personaje listo para producción.

## Próximo paso

Volumen básico bípedo de Hound fiel a la lámina 01: capucha, proporciones,
armadura y manos. Fuente `.blend` versionada fuera de `.cache`, frente/perfil/
espalda y prueba de escala en Gloom. Espalda y articulaciones serán propuestas
para aprobación; no comenzar acabado y animación hasta aceptar ese volumen.
