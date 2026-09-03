# Personajes originales: hitos 62–63

El hito 62 se validó con **35/35 pruebas Debug superadas**. Véase el
[informe con capturas](../reports/characters-2026-09-02/README.md).
La animación y los efectos del hito 63 se documentan en
[ANIMATION_VFX.md](ANIMATION_VFX.md) y su informe de aceptación, con **38/38
pruebas Debug superadas** y secuencias comparables de 30/60/144 Hz.

Archangel y Shadow se importan con sus materiales, esqueletos y pesos. Hound
comparte el cuerpo de Archangel. Soul Reaper original sustituye al arma
provisional en primera y tercera persona. El hito 63 evalúa los cuatro clips
recuperados de Archangel y movimiento procedural de Shadow, con skinning GPU,
brazos FPS, agarres y partículas. La revisión estática conserva la pose de reposo.

## Dirección de materiales

Los [bocetos suministrados](art/characters/README.md) son la referencia de esta
entrega. El cuarto adjunto es una copia idéntica del segundo dibujo acabado de
Shadow; se conserva una sola copia de esos bytes.

| Modelo | Superficie | Emisión |
| --- | --- | --- |
| Archangel | Armadura dorada metálica; traje oscuro poco metálico | Máscara cian de alas, hombros y detalles azules del atlas |
| Shadow | Armadura metálica oscura con desgaste original; cuerpo oscuro | Máscara original de ojos, en rojo |
| Soul Reaper | Difuso, normal, especular y glow originales | Glow traducido con su color original |

El glow del material es emisión HDR con bloom; el hito 63 añade emisores
independientes de energía, humo y estelas. Archangel usa metallic 0,9
en armadura y 0,05 fuera de ella; Shadow 0,85 en armadura y 0,02 fuera. Ambos
usan roughness entre 0,26 y 0,58, derivada del alfa especular original. La
intensidad emisiva es 4. Los controles son reproducibles en
`tools/legacy/import_characters.py`; no se alteran los originales.

## Selección y compatibilidad

El selector del cliente ofrece Hound con sus loadouts existentes, Archangel y
Shadow. Los dos nuevos usan Soul Reaper sin habilidades nuevas. El nombre
`berserker-reaper` sigue admitiéndose para partidas de desarrollo existentes y
se presenta como Hound; ya no se ofrece como clase original independiente.
Screamer queda pendiente de un modelo y una definición de contenido propia.

```powershell
# Host; ejecutar el cliente en otro proceso:
.\build\windows-vs\Debug\gloom.exe --vertical-slice-host 0.0.0.0:27020
.\build\windows-vs\Debug\gloom.exe --vertical-slice-join 127.0.0.1:27020 Nyx archangel-reaper
# Alternativa:
.\build\windows-vs\Debug\gloom.exe --vertical-slice-join 127.0.0.1:27020 Nyx shadow-reaper
```

El protocolo es 14. Los IDs existentes se conservan; Archangel y Shadow son
2 y 3. Cápsula, vida, alcance, cooldown, autoridad de disparo y cámara siguen
las reglas anteriores. Esta incorporación de arte no implementa los poderes
originales de las dos clases ni modifica el balance.

## Conversión y rigs

Usar el entorno Python de [FACTORY_RESTORATION.md](FACTORY_RESTORATION.md):

```powershell
& '.cache\legacy-tools\Scripts\python.exe' tools\legacy\import_characters.py --legacy-root D:\Projects\Gloom-Legacy --output-root assets\characters\original --work-root .cache\character-import
& '.cache\legacy-tools\Scripts\python.exe' tools\legacy\verify_characters.py --legacy-root D:\Projects\Gloom-Legacy --reference-root assets\characters\original --work-root .cache\character-reproduction
& 'D:\Dev\CMake\bin\cmake.exe' --build --preset windows-debug
```

Resultado reproducido: **20 archivos idénticos y 17 fuentes verificadas**.
El manifiesto incluye hashes, versiones de herramientas, límites, escala,
influencias, clips disponibles y el ajuste de origen del esqueleto de Shadow.
Ogre/Assimp son herramientas offline y no se incorporan al juego.

| Modelo | Triángulos | Huesos | Influencias máximas | Clips encontrados |
| --- | ---: | ---: | ---: | --- |
| Archangel | 4.936 | 43 | 4 | forward, idle, jump, strafe_right |
| Shadow | 2.788 | 17 | 5 | Ninguno |
| Soul Reaper | 1.600 | Sin rig | — | Ninguno |

El formato de escena **5** conserva ocho influencias por vértice mediante
JOINTS/WEIGHTS 0 y 1, jerarquías, inversas de bind pose y clips TRS LINEAR/STEP.
Se rechazan datos inválidos e interpolaciones no soportadas. CPU evalúa y mezcla
poses; GPU deforma posiciones, normales, tangentes y posiciones anteriores.
El rig permanece en residencia para los anclajes y los bounds animados.

La altura visual elegida es 1,8 m. Shadow necesitaba invertir su eje vertical
y corregir la separación entre esqueleto y geometría; el ajuste está explícito
en [ADR 0063](architecture/0063-original-character-bind-rigs.md), y los tests
comprueban cabeza/mano dentro de la altura esperada. Las alas y la cola son
geometría visual; no redefinen los volúmenes autoritativos de impacto.

## Revisión

```powershell
.\build\windows-vs\Debug\gloom.exe --character-review .cache\character-review\captures
.\build\windows-vs\Debug\gloom_visual_capture_tests.exe .cache\character-review\captures tests\character-visual-references --characters
& 'D:\Dev\CMake\bin\ctest.exe' --preset windows-debug --output-on-failure
```

Siete vistas revisan frente/espalda de ambos modelos y Soul Reaper mirando
adelante, arriba y abajo. Las dos luces de inspección pertenecen solo a este
modo: permiten comprobar metal y normales dentro de Factory. La partida
normal conserva la iluminación del mapa y puede mostrar superficies más oscuras.
Los errores de Vulkan y diferencias excesivas respecto a referencias fallan.
Se mantienen las nueve referencias de hito 58 y seis de Factory del hito 61.

La presentación FPS usa geometría original de brazos/manos con IK de agarre y
anclaje de arma evaluado. Las revisiones temporales de 63 añaden locomoción,
combate, humo/cola y estelas de ojos, tanto con luces de inspección como de
Factory. Véanse los comandos y límites de [ANIMATION_VFX.md](ANIMATION_VFX.md).
