# Traspaso de Gloom

Actualizado: 6 de septiembre de 2026, tras cerrar el hito 67. Traspaso para el hito 68.
Actualizar este documento al cerrar cada hito; guardar el detalle en informes
y crear el commit de cierre según `AGENTS.md`.

## Estado y cuidado del workspace

- Proyecto: `D:\Projects\Gloom`. Original: `D:\Projects\Gloom-Legacy`.
- El commit de cierre del 67 consolida también los cambios pendientes de los
  hitos anteriores que necesita esta versión. Consultar `git log -1` para su hash.
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
- Protocolo actual **18**; Factory incluye recogibles y reglas en su huella.
  Documentos históricos que indican protocolos 15/16/17 describen entregas previas.

## Último hito cerrado: 67, recogibles de Factory

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

Si se revisan recogibles, leer solo la sección 67 de
[GAMEPLAY_MIGRATION.md](GAMEPLAY_MIGRATION.md) y las partes pertinentes del informe.

## Siguiente hito: 68, alcance pendiente de definir

El roadmap termina en el 67. No hay todavía objetivo ni criterios de aceptación
acordados para el 68. Este traspaso prepara su continuación, sin asignarle una
funcionalidad por defecto. Pendientes del roadmap: habilidades de clase restantes,
audio, UX/ajustes, despliegue externo y optimización/distribución; ninguno constituye
por sí solo el encargo del siguiente hito.

Al retomar:

1. Leer este estado y revisar `git status`; continuar sobre este workspace.
2. Usar el objetivo que indique el usuario para el 68. Si solo pide «implementar
   el hito 68» y sigue sin estar definido, solicitar su alcance antes de implementar.
3. Leer únicamente el tramo final de `docs/ROADMAP.md` y los archivos relacionados
   con ese objetivo. Registrar alcance, exclusiones y aceptación antes de cambiar código.
4. Conservar las decisiones de los hitos 65–67 y usar sus pruebas como regresión
   cuando se toquen movimiento, arsenal, recogibles, HUD o red.

El ejecutable actualizado es `build/windows-vs/Debug/gloom.exe`; el servidor
`build/windows-vs/Debug/gloom_slice_server.exe` también está recompilado con
protocolo 18. Las capturas del 67 se reproducen con `gloom --pickup-review DIR`.

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
- Regresiones: `tests/legacy_arsenal_tests.cpp`, `tests/vertical_slice_tests.cpp`,
  `tests/vertical_slice_network_tests.cpp`, `tests/legacy_movement_tests.cpp`.

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
- PowerShell; CMake/CTest: `D:/Dev/CMake/bin`, preset `windows-debug`.
  Ejecutable: `build/windows-vs/Debug/gloom.exe`.
- Compilar motor antes de enlazar ejecutables. Con
  `/p:BuildProjectReferences=false` se omite reconstruir dependencias: usarlo
  únicamente cuando ya estén actualizadas, para no validar una biblioteca vieja.
- Python disponible: `.cache/legacy-tools/Scripts/python.exe` (Ogre, Pillow,
  numpy, av). `python` puede apuntar al alias de Microsoft Store.

## Trabajo con contexto reducido

Leer este documento primero; abrir fuentes con `rg` y rangos pequeños según
la necesidad. No cargar todos los informes, JSON grandes ni historial del chat.
Redirigir builds/pruebas a logs y mostrar solo errores/resultados. Pruebas
focalizadas durante cambios; ampliar por riesgos de integración concretos y
requisitos del hito. Actualizaciones breves cuando haya avances relevantes.
Al finalizar, actualizar estado, decisiones, validación y siguiente paso aquí;
crear el commit del hito, comprobar `git status` e informar del hash.
