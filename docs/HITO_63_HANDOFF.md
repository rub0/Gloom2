# Arranque del hito 63: animaciones y efectos de combate

**Entrega posterior, 2026-09-03:** hito 63 implementado y validado con 38/38
pruebas y secuencias Vulkan de 30/60/144 Hz. Véanse
[ANIMATION_VFX.md](ANIMATION_VFX.md) y el
[informe de aceptación](../reports/animation-vfx-2026-09-03/README.md).
El texto siguiente conserva la especificación y el estado inicial del traspaso;
sus indicaciones de «no implementado» describen ese momento, no la entrega.
El hito 64 permanece pendiente.

Preparado el 2026-09-03 para continuar en una tarea nueva. Este documento es
una especificación de trabajo; el hito 63 **no está implementado**. El hito 62
está completado y su último registro acredita **35/35 pruebas Debug superadas**
en 112,77 s: [evidencia](../reports/characters-2026-09-02/tests-final.log).
Al preparar este traspaso se revisaron documentación, fuentes y estado de Git;
no se volvió a ejecutar la suite ni se modificó el código del juego.

## Instrucción para la nueva tarea

Implementar y validar el hito 63 completo en `D:\Projects\Gloom`, siguiendo
este documento y las decisiones existentes. Conservar todos los cambios
actuales. El hito 64 y el resto del trabajo quedan para más adelante.

## Lectura y estado inicial

Leer `README.md`, `DEVELOPING.md`, `docs/ROADMAP.md`, `docs/CHARACTERS.md`,
`docs/ART_RESTORATION.md` (especialmente sección 7) y los ADR:

- `0063-original-character-bind-rigs.md`: contrato actual y límites del hito 62.
- `0062-original-factory-and-extended-materials.md`: materiales y Factory.
- `0061-original-assets-and-visual-restoration-plan.md`: fuentes y alcance.
- `0060-visual-correctness-and-capture-regression.md`: aceptación visual.
- `0044-roster-character-presentation.md`, `0045-authored-character-content.md`,
  `0021-temporal-render-features.md`: presentación y render temporal.

Verificar el estado real antes de editar y buscar instrucciones `AGENTS.md`
aplicables. En el traspaso, el repositorio moderno muestra sus archivos como
**untracked**; no usar clean/reset, no eliminar ni sobrescribir trabajo previo
y no crear commits sin petición. La copia original `D:\Projects\Gloom-Legacy`
está limpia en `fe59e723594cc13e0fc95a1f390d0f81f58dc45b` y debe seguir intacta.
Las conversiones se hacen sobre copias, con hashes y manifiestos.

`ART_RESTORATION.md` conserva el diagnóstico anterior a 59–62; para conocer lo
ya implementado prevalecen el estado del código, `CHARACTERS.md` y ADR 0063.
No repetir los hitos terminados porque una descripción histórica diga pendiente.

## Base disponible y límites conocidos

- Factory original, colisión, materiales, iluminación, multijugador y autoridad
  están integrados. No quedan hitos de red programados, pero se deben conservar
  y verificar sus contratos al conectar los eventos visuales.
- Archangel/Hound: 4.936 triángulos, 43 huesos y hasta cuatro influencias.
  El original contiene `forward`, `idle`, `jump` y `strafe_right`; se inventariaron,
  pero todavía no se exportan ni reproducen como clips modernos.
- Shadow: 2.788 triángulos, 17 huesos, hasta **cinco** influencias y ningún clip.
  Hay que crear su movimiento; no afirmar que se recuperaron clips inexistentes.
- Soul Reaper: 1.600 triángulos, sin rig. Ya tiene presentación FPS y TPS rígida.
- Escena cocinada versión **4**: jerarquías, inverse bind matrices y ocho
  influencias por vértice. El importador aún rechaza glTF con animaciones.
  La residencia conserva el rig; el renderer muestra la pose de reposo.
- Shadow tiene una rotación de raíz de 180° en X y un ajuste explícito de origen
  del esqueleto. Respetar el rebase y las inversas documentadas en ADR 0063;
  comprobar deformación real antes de dar el rig por válido para animación.
- Altura visual 1,8 m; cápsula, cámara y volúmenes autoritativos independientes.
- Protocolo **13**. Archangel y Shadow son IDs 2 y 3. Hound reutiliza Archangel;
  `berserker-reaper`/ID 1 es compatibilidad de Hound. No reciclar esos IDs.
- Hay emisión HDR, bloom, mezcla alfa/aditiva y UV animadas; **no hay sistema
  de partículas**, emisores, simulación de vida, humo ni estelas integrados.

## Dirección artística y referencias

Conservar los materiales recuperados y utilizar los bocetos de
`docs/art/characters/` y la comparación del hito 62 en
`reports/characters-2026-09-02/concept-comparison.png`.

- **Archangel:** armadura dorada metálica; alas, hombros y detalles azules con
  glow cian del mismo color. Añadir energía y movimiento sin ocultar la silueta
  ni quemar la imagen con bloom.
- **Shadow:** armadura metálica oscura, ojos con glow rojo; completar movimiento
  de cola, humo oscuro y estelas de ojos inspirados en los bocetos.
- **Soul Reaper:** conservar mapas y glow originales, mejorar agarre, anclajes
  animados, brazos/manos FPS y respuesta visual de disparo.

Referencia audiovisual aportada por el usuario:
<https://www.youtube.com/watch?v=yJPoulcfIAg>, exclusivamente **0:00–2:00**.
Consultar también `reports/factory-restoration-2026-09-02/README.md`.
Ese tramo mezcla Factory y Dungeons: comparar movimiento, ritmo y efectos
pertinentes sin cambiar Factory por otro mapa. Si no se puede reproducir el
vídeo, declarar esa limitación y no presentar una comparación como realizada.
Las imágenes de inspección del hito 62 usan dos luces adicionales solo para
revisión; comprobar también el resultado con la iluminación normal de Factory.

## Trabajo a implementar

### 63.1 — Clips, evaluación y skinning

1. Auditar y convertir los cuatro clips originales de Archangel con tiempos,
   canales, transformaciones de ejes/unidades y bucles verificados. Documentar
   qué se recupera, qué se corrige y qué se crea como animación nueva.
2. Extender importación, cooker, serialización y residencia para clips y sus
   validaciones. Versionar el formato si cambia; rechazar entradas inválidas
   o modos no soportados explícitamente. Conservar las ocho influencias.
3. Implementar evaluación de poses, interpolación y mezcla; skinning GPU de
   posiciones/normales/tangentes y posiciones anteriores para movimiento
   temporal. Tratar cortes, teleport, muerte y respawn sin estelas espurias.
4. Resolver bounds animados y culling, incluida la coherencia de sombras.
   Mantener una ruta correcta para mallas estáticas y pruebas de bind pose.

### 63.2 — Movimiento y arma en juego

1. Integrar idle, avance, retroceso, strafe, salto/aterrizaje, disparo, daño y
   muerte. Completar los clips ausentes y crear movimiento apropiado de Shadow;
   las soluciones procedurales son válidas si se documentan y se ven correctas.
2. Mezclar locomoción y apuntado de torso con anclajes de mano, agarre y boca de
   fuego evaluados desde la pose. Completar brazos/manos FPS usando geometría
   disponible cuando sea viable y revisar el arma mirando arriba y abajo.
3. Incorporar presentación de equipar/cambio donde exista el evento. Animar
   recarga solo si el arma tiene esa mecánica; no añadir reglas para justificar
   una animación. Mantener rayos autoritativos desde la cámara y reglas actuales.
4. Verificar cuerpo y arma locales/remotos con interpolación y reconciliación,
   sin conducir la simulación autoritativa mediante root motion visual.

### 63.3 — Sistema de partículas y efectos

1. Crear un sistema configurable mediante datos: emisores continuos y ráfagas,
   posición/dirección, semillas reproducibles para revisión, vida, velocidad,
   tamaño/color sobre vida, billboards y estelas. Definir límites de capacidad,
   liberación y comportamiento al saturarse; evitar acumulación sin límite.
   Elegir simulación CPU/GPU según la arquitectura y documentar la decisión.
2. Integrar render con profundidad, transparencia/aditivo, texturas, HDR/bloom
   y distorsión para los efectos que la requieran. Resolver orden y lectura
   de recursos sin artefactos ni errores de validación Vulkan.
3. Auditar `D:\Projects\Gloom-Legacy\Exes\media\particles` y sus materiales.
   Contiene `shadow.pu`, `heatHaze.pu`, `fogonazo3.pu`, `groundTouch.pu`,
   `spawn.pu`, recetas de impactos y carpetas Smoke, MuzzleFlash, Hit, etc.
   Recuperar texturas/recetas con dependencias y procedencia; adaptar HLSL/Cg
   y bindings Ogre al motor moderno, sin añadir Ogre al runtime.
4. Integrar energía cian de Archangel, humo/cola y estelas rojas de Shadow,
   fogonazos e impactos de Soul Reaper y efectos de daño, escudo, aterrizaje,
   aparición y lava donde correspondan a eventos existentes. Auditar sangre y
   explosiones originales; si no hay evento jugable aplicable, mantener una
   muestra de revisión y documentarlo sin inventar nuevas armas o habilidades.
5. Conectar efectos transitorios a eventos identificables de la simulación;
   reconciliar predicción/confirmación sin duplicar fogonazos o impactos.
   No reproducir eventos antiguos al reconectar. Cancelar emisores persistentes
   al morir, reaparecer, eliminar entidad o salir de escena. El dedicado debe
   funcionar sin renderer ni simulación cosmética de partículas.

### 63.4 — Aceptación y entrega

- Pruebas de clips/poses/mezcla, datos malformados, pesos y serialización;
  deformación y anclajes reales, bounds y posiciones anteriores.
- Pruebas de emisores y eventos: vida finita, límites, deduplicación,
  reconciliación, reconnect y limpieza en muerte/respawn/salida.
- Secuencias reproducibles de movimiento y combate a **30/60/144 FPS**, con
  tiempos comparables. Capturar secuencias o vídeo, no solo poses fijas.
  Comprobar arma/cuerpo sincronizados, ausencia de clipping grave y efectos
  que no cambien sustancialmente con la frecuencia de render.
- Revisar ambos personajes en Factory con luces de juego y en inspección;
  comparar contra bocetos y el tramo del vídeo indicado con evidencia concreta.
- Validar local, host/join y dedicado con dos clientes reales, incluyendo
  disparo, daño, muerte, respawn y reconnect; no cambiar daño ni cooldowns.
- Mantener las regresiones visuales de 58/61/62. No reemplazar referencias para
  ocultar diferencias sin inspeccionarlas y justificar el cambio.
- Compilar y pasar la suite completa, añadiendo pruebas relevantes al alcance.
  Guardar logs, capturas/secuencias, manifiesto reproducible e informe con límites
  en `reports/`. Actualizar documentación y crear el ADR de animación/VFX con
  el siguiente número libre. Marcar 63 terminado solo al cumplir su aceptación.

## Puntos de entrada del código

- Conversión: `tools/legacy/import_characters.py`, `verify_characters.py`;
  datos y manifiesto en `assets/characters/original/`.
- Importación/rigs: `include/gloom/assets/gltf_importer.hpp`,
  `include/gloom/assets/rig.hpp`, `src/assets/gltf_importer.cpp`.
- Render: `include/gloom/render/gpu_assets.hpp`, `scene.hpp`, `temporal.hpp`,
  `src/backends/diligent_renderer.cpp` y sus rutas de shaders/cooker asociadas.
- Presentación: `include/gloom/gameplay/character_presentation.hpp`,
  `src/gameplay/vertical_slice.cpp`, `vertical_slice_network.cpp` y `apps/`.
- Regresiones: `tests/character_restoration_tests.cpp`,
  `tests/vertical_slice_transport_tests.cpp`, `tests/character-visual-references/`
  y los registros CTest de `CMakeLists.txt`.

## Comandos de partida

Desde `D:\Projects\Gloom`:

```powershell
git status --short
& 'D:\Dev\CMake\bin\cmake.exe' --preset windows
& 'D:\Dev\CMake\bin\cmake.exe' --build --preset windows-debug
& 'D:\Dev\CMake\bin\ctest.exe' --preset windows-debug --output-on-failure
.\build\windows-vs\Debug\gloom.exe --character-review .cache\character-review\captures
.\build\windows-vs\Debug\gloom.exe --vertical-slice
```

La revisión `--character-review` existente solo comprueba siete vistas estáticas;
crear una revisión temporal para 63. Los comandos de conversión y comparación
están en `docs/CHARACTERS.md`; verificar el intérprete y las dependencias locales
antes de regenerar. El build normal no necesita las herramientas Ogre/Python.

## Fuera de este hito

HUD/menús/lobby gráfico (64), nuevas clases/modelo de Screamer, armas o poderes
nuevos, balance, audio, despliegue público y otros hitos posteriores. Conservar
la identidad, materiales y mapa recuperados. El alcance incluye animación y
partículas funcionales en partida: una demo aislada no completa el hito.
