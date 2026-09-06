# Recuperación visual de Gloom: puntos 5–8

Refinamiento del 2 de septiembre de 2026. Los hitos **59–61 están implementados**;
véanse [la entrega](FACTORY_RESTORATION.md), ADR 0062 y el
[informe comparado](../reports/factory-restoration-2026-09-02/README.md).
El hito **62** recupera los dos personajes disponibles y Soul Reaper, con rigs
en reposo y materiales dirigidos por los bocetos: véase [CHARACTERS.md](CHARACTERS.md).
El hito **63** añade clips, skinning, brazos FPS y partículas; véanse
[ANIMATION_VFX.md](ANIMATION_VFX.md) y el
[informe con 38/38 pruebas y secuencias](../reports/animation-vfx-2026-09-03/README.md).
El hito **64 está completado**: HUD, menú, navegador, sala, selección y flujo
de reconexión gráfico, con 41 pruebas validadas. Véanse [UI.md](UI.md), ADR 0065
y el [informe de aceptación](../reports/ui-2026-09-04/README.md).
Audio, ampliación del juego, balance,
despliegue público y distribución quedan para después.

El resto del documento conserva la auditoría inicial y los criterios acordados
cuando el motor estaba en el hito 58. Las carencias descritas en presente en
esa auditoría son el punto de partida; el estado actual de 59–64 está en la
documentación de las entregas enlazada arriba.

## Referencias comprobadas

- Vídeo indicado: [Gloom, 0:00–2:00](https://www.youtube.com/watch?v=yJPoulcfIAg).
  Inspección de fotogramas de todo ese intervalo, con una secuencia adicional
  cada medio segundo entre 0:15 y 1:05. Las tarjetas iniciales ocupan parte del
  intervalo. El montaje alterna Factory y Dungeons: sus interiores de piedra,
  lava y demostraciones de efectos no deben confundirse con geometría de Factory.
- Repositorio local original: `D:\Projects\Gloom-Legacy`, solo lectura, commit
  `fe59e723594cc13e0fc95a1f390d0f81f58dc45b`. Coincide con el HEAD comprobado de
  [franaisa/Gloom](https://github.com/franaisa/Gloom). La malla Factory coincide
  byte a byte; los tres mapas principales y su material coinciden normalizando
  CRLF/LF. El remoto de la copia local se llama `rub0/Gloom`.
- Estado moderno: hito 58, última suite completa 30/30. Los seis glTF del juego
  son recursos provisionales, aunque ya atraviesan el pipeline real de assets.
  La nueva revisión no reemplaza esos recursos ni altera la simulación.

La referencia visual es un FPS industrial oscuro, con recorridos a varias
alturas, suelos y metales texturados, lava luminosa, armas con acentos de color
y una interfaz ornamental turquesa. El fotograma 0:45 muestra las pasarelas,
columnas y lava de Factory. Hacia 1:15 se muestra el relieve especular del suelo;
hacia 1:35–1:45 se muestran efectos volumétricos en el otro escenario. Esas
demostraciones orientan los materiales y efectos, no la planta de Factory.
El objetivo es recuperar esa identidad conservando legibilidad y las
correcciones de cámara, normales y sombras del hito 58.

## Qué se puede recuperar

| Recurso original | Evidencia | Tratamiento |
| --- | --- | --- |
| `models/mapaAlberto/mapaAlberto.mesh` | Ogre v1.8, 16 submallas/materiales, 16.351 triángulos | Conversión estática probada a glTF; conservar geometría, normales, UV y asignaciones de material |
| `materials/scripts/mapaAlberto/mapaAlberto.material` | 38 nombres de textura referenciados, todos encontrados | Traducir semántica de cada material; no basta con importar el nombre |
| `models/mapaAlberto/mapaAlberto.RepX` | XML PhysX 3.2, 3.564 puntos y 5.856 triángulos | Extraer geometría y transformaciones; generar colisión Jolt |
| `maps/Factory_client.txt`, `Factory_server.txt` | 88 y 97 registros de tipo; diferencia de nueve spawns | Una escena normalizada con procedencia e identificadores; sin duplicar objetos compartidos |
| `models/personajes/archangel.mesh` | 4.936 triángulos; existe `.skeleton` | Recuperar personaje humanoide y rig; falta soporte de skinning en el motor |
| `models/personajes/shadow.mesh` | 2.788 triángulos; existe `.skeleton` | Recuperable, sin asumir que contiene todas las animaciones necesarias |
| `models/weapons/soulReaper.mesh` | 1.600 triángulos; color, normal, specular y glow en sus scripts | Primera arma para sustituir el modelo provisional |
| `models/weapons/miniGun.mesh` | 4.734 triángulos; existe `.skeleton` | Auditar el rig y movimiento mecánico antes de convertir animación |
| `gui`, `particles`, `materials/programs` | SWF, imágenes, layouts, partículas y shaders HLSL/Cg | Reutilizar arte y comportamiento; reconstruir presentación en el motor moderno |

Las cifras de triángulos proceden de importaciones reales con Assimp. La prueba
de Factory expande los vértices por caras a 49.053; no es un recuento de vértices
únicos del fichero original. Sus límites en unidades originales son
`(-413.927, -30.895, -260.769)` a `(308.505, 143.339, 83.740)`.

En `Exes/media/models` hay **133 `.mesh`**: 117 con cabecera v1.8 y 16 v1.41,
además de cuatro `.skeleton`, cinco FBX y doce MAX. No se afirma haber validado
los 133 modelos. Factory, Archangel, Shadow, Soul Reaper, Minigun y lavaFondo
se abrieron correctamente. El ensayo con `armourSmall.mesh` v1.41 fue rechazado
por Assimp y pide actualización de formato: esos recursos necesitan una copia
normalizada con herramientas Ogre antes de pasar por el mismo pipeline.

La ruta preferida es **Ogre `.mesh` → glTF/GLB → cooker → `.gasset`**. El lector
actual de Assimp admite binario v1.8; OgreXMLConverter/Upgrader sirve de ruta de
normalización para versiones antiguas. Es una herramienta offline: Ogre y PhysX
no vuelven a ser dependencias del juego. Un `.max` es una fuente adicional; no
es necesario disponer de 3ds Max para recuperar la geometría ya exportada.
Fuentes: [importador Ogre de Assimp](https://github.com/assimp/assimp/blob/master/code/AssetLib/Ogre/OgreImporter.cpp)
y [herramientas Ogre](https://ogrecave.github.io/ogre/api/latest/_mesh-_tools.html).

## 5. Factory fiel y materiales completos

**59 — Importación reproducible del contenido original.** Crear un manifiesto
de fuentes, hashes, versiones, dependencias y conversiones. Resolver nombres
según `resources.cfg`, que tiene directorios separados y nombres duplicados
como `lavaFondo.mesh`; no elegir el primer archivo encontrado recursivamente.
Normalizar las mallas v1.41 en copias, preservar submallas, normales, tangentes,
UV y materiales, y convertir TGA a un formato de intercambio sin perder alfa.
Importar geometría estática y registrar explícitamente toda característica no
traducida. Los modelos con rig no deben quedar aceptados silenciosamente como
modelos estáticos.

Aceptación: conversión reproducible de Factory, Soul Reaper y un objeto v1.41;
ninguna dependencia perdida; hashes y límites geométricos verificados; escena
cocinada y visible en Vulkan. La prueba de esta revisión solo cubre una parte
de este trabajo. Esqueletos y clips completos se cierran en 62–63.

**60 — Materiales extensibles y recuperación de los shaders.** Hay una base
metallic/roughness con color, normal y emisivo constante. El importador guarda
alpha mode/cutoff y doble cara, pero el puente GPU todavía no los transmite:
su funcionamiento completo en pantalla también está pendiente.
Faltan mapas emisivos y de oclusión en el contrato importado, control especular,
anisotropía, múltiples UV y animación de materiales. El ambiente actual es una
aproximación cielo/suelo; no equivale a reflejos del escenario.

La primera versión debe cubrir:

- Base color, normal con intensidad, metallic, roughness, AO, emisivo con mapa
  e intensidad HDR; interpretación sRGB/lineal y canales documentados.
- Intensidad y color especular con mapas. Los `_SPEC` antiguos representan
  respuesta especular, **no son mapas metallic ni roughness**. Convertir
  `shininess` con una política documentada y ajustes visuales; no prometer una
  equivalencia física exacta del shader antiguo.
- Anisotropía de reflexión: intensidad, dirección/rotación y mapa, con espacio
  tangente coherente. Es distinta del filtrado anisotrópico de texturas.
- Opaco, recorte alfa, transparencia y mezcla aditiva con profundidad/orden
  correctos; doble cara donde corresponda.
- Tiling, offset, rotación y desplazamiento temporal de UV, texturas de detalle
  y segundo canal UV para lightmaps. Lava y glow deben conservar su animación.
- Iluminación ambiental/reflejos mediante IBL o sondas locales, emisivos y bloom
  controlado, y luces locales que permitan leer el metal en interiores.

Usar materiales maestros parametrizables para piedra, metal, arma emisiva,
lava y efectos. glTF ofrece contratos para
[specular](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_specular)
y [anisotropy](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_anisotropy).
Habrá que extender importador, formato cocinado/versionado, residencia y shader;
escribir esas propiedades en el glTF no basta. Los efectos procedurales se
describen en datos propios del motor, conservando la fuente original.

Esto proporciona las características visuales solicitadas de los materiales
habituales de Unreal. Importar `.uasset`, ejecutar grafos de Unreal, disponer
de un editor completo de nodos o implementar todos sus modelos de sombreado
son proyectos diferentes. Clear coat, parallax/height, transmisión y subsurface
quedan como extensiones posteriores a este conjunto, salvo necesidad concreta
de un material recuperado.

Aceptación: muestras de suelo original, metal especular, metal anisotrópico,
arma emisiva y lava en luz fija y móvil; anisotropía cero recupera el material
isotrópico; sin NaN, tangentes inválidas ni pérdida de alfa. Revisar intensidad
de brillos, reflejos, detalle a distancia, costuras y versiones del cooker.

**61 — Reconstrucción completa de Factory.** Usar la malla original, no una
aproximación modular de su planta. Importar plataformas, corredores y columnas
tal como están modelados. Los objetos independientes y puntos funcionales
proceden de los mapas y de sus arquetipos/blueprints.

La carga multijugador original selecciona `_client.txt` y `_server.txt` en
`LobbyClientState.cpp` y `LobbyServerState.cpp`. Sus hermanos `.map` son copias
idénticas en este commit, pero **`Factory.map` difiere**: tiene 100 registros y
luces adicionales. Tomar los TXT como base multijugador, emitir una comparación
con `Factory.map` y resolver diferencias visuales conscientemente. No concatenar
los cinco archivos: duplicaría pickups, mundo, lava y otros objetos.

El servidor contiene nueve spawns; cliente y servidor comparten 43 SmallOrb,
cinco Orb, armas/munición, escudos, mejoras, jumper y lava. La lava de esta
variante se define como plano generado en `(-218, -20, -67.7)`, de 1500×1500 y
tiling 20×20: no debe sustituirse automáticamente por el archivo homónimo.
Conservar inventario de objetos incluso si la mecánica de algunos queda para
la fase de contenido posterior; esos casos deben quedar identificados, sin
presentarlos como pickups funcionales que todavía no existen.

Definir una única transformación de unidades/ejes para mundo, spawns, objetos,
cámaras, luces y colisión. Calibrar contra personaje y recorridos; no encoger
el mapa arbitrariamente para que encaje en el blockout. El RepX tiene un
`GlobalPose` de aproximadamente +90° en X: aplicar escala, pose local y pose
del actor, sin volver a rotar la malla visual ya exportada. Los límites de
colisión transformados difieren de los visuales; verificar contacto real y
conservar las barreras del diseño donde estén justificadas.

Aceptación: superposición visual/colisión; recorrido de las rutas bajas y altas;
spawns transitables; límites y lava correctos; comparación de cámaras con el
vídeo; ausencia de objetos duplicados y texturas perdidas. Render, movimiento
Jolt, consultas de disparo y predicción deben consumir la misma escena. La
sustitución de cajas por triángulos requiere comprobar también oclusión de
disparos, no solo que el personaje se mantenga sobre el suelo. Probar local,
host/join y dedicado, manteniendo la autoridad actual.

## 6. Modelos originales de personajes y armas

**62 — Recuperar identidad, rigs y presentación.** Empezar por Soul Reaper y
Archangel/Hound; conservar silueta, texturas, proporciones y pivotes. Auditar
materiales de clase y equipamiento, y separar presentación FPS de tercera
persona. Crear anclajes de mano, arma y boca de fuego; completar brazos/manos
solo si los originales no proporcionan lo necesario.

Hay una corrección de alcance: `archetypes.txt` asigna **`archangel.mesh` a
Hound**, con material `Hound`. El cuadrúpedo moderno no reproduce ese modelo.
`Hound.cpp` trata **Berserker como una habilidad de Hound**; no hay evidencia
en los arquetipos auditados de una clase original separada llamada Berserker.
El roster original enumera Shadow, Screamer, Archangel y Hound. El tuple moderno
`berserker-reaper` queda documentado como provisional; antes de convertirlo en
arte final hay que decidir y migrar su identidad de forma explícita, preservando
compatibilidad de selección/red. Este refinamiento no modifica esos IDs.

Importar huesos, pesos, jerarquía y bind pose. El cooker moderno **rechaza**
glTF con skins/animations; añadir ese soporte es trabajo real de este hito.
La API Python del ensayo no exporta rig ni clips, aunque abra su geometría.

Aceptación: personaje original con pose de reposo y pesos correctos, arma FPS
con encuadre útil en todos los ángulos, mismo personaje visible para el otro
jugador y sin discrepancias de cápsula/cámara. Acordar hit volumes explícitos
si la silueta cambia; no derivar autoridad de los bounds de un asset cargado.

## 7. Animaciones y efectos de combate

**63 — Skinning, mezcla y efectos asociados a eventos.** Recuperar clips útiles,
completar los ausentes y añadir idle, avance/retroceso/strafe, salto/aterrizaje,
disparo, cambio de arma, daño y muerte. Animar recarga solo para armas que tengan
esa mecánica. `archangel.skeleton` contiene chunks de animación con los nombres
`forward`, `idle`, `jump` y `strafe_right`; hay que verificar su calidad y bucle,
no asumir una biblioteca completa.

Implementar evaluación y mezcla de poses, apuntado de torso, anclajes y
skinning GPU, incluyendo posiciones anteriores para vectores de movimiento.
Validar bounds animados para evitar desapariciones por culling. Los eventos
visuales siguen la simulación: evitar duplicados por reconciliación/reconexión
y limpiar efectos al morir o reaparecer.

Recuperar las texturas/recetas de `particles` y los shaders de fogonazos,
impactos, sangre, explosiones, escudos, lava, glow y distorsión. HLSL/Cg antiguo
usa pases y bindings Ogre: necesita adaptación. Los volumétricos del vídeo
sirven como referencia de efecto, sin importar Dungeons en lugar de Factory.
No incorporar nuevas armas o habilidades jugables solo por existir un efecto.

Aceptación: secuencia reproducible de movimiento y combate a 30/60/144 FPS,
arma y cuerpo sincronizados, efectos únicos y finitos, sin clipping grave ni
cambios en daño/cooldowns. Revisar secuencias además de capturas fijas.

## 8. HUD, menús, lobby y selección

**64 — Interfaz gráfica coherente con el original.** Recuperar iconografía,
texturas, tipografías y composición ornamental turquesa. Hay 17 SWF, entre ellos
`Hud`, `MenuPrincipal`, `MultiplayerClient`, `MultiplayerServer`, `NetworkGame`
y `SeleccionPersonaje`. No se ejecutarán Flash/Hikari: extraer recursos útiles
y reconstruir las pantallas sobre una capa UI moderna.

HUD: vida, escudo, retícula, arma/munición cuando exista, habilidad/cooldown,
marcador y mensajes legibles. Menú: jugar, navegador, sala, selección y pausa;
carga, vacío, error, reconexión y confirmación deben verse en la ventana.
Conectar las pantallas al navegador/lobby/autenticación existentes; no inventar
otro backend. Conservar opción CLI para herramientas y pruebas. Ajustes amplios
y cambios de cuenta ajenos a estas pantallas quedan en el bloque UX posterior.

Aceptación: mouse y teclado, foco/cursor correctos al entrar/salir del juego,
escalado DPI, 16:9 y ultrawide sin recortes, y flujo completo buscar → seleccionar
→ listo → jugar → desconectar/reconectar con dos procesos. El vídeo proporciona
referencia de HUD; los menús se contrastan con los recursos originales porque
no se ven suficientemente en esos dos minutos.

## Prueba reproducible de esta revisión

`tools/legacy/probe_factory.py` produce un glTF estático de diagnóstico y un
manifiesto con materiales, dependencias, versiones y recuentos. Necesita
Python y las dependencias fijadas en `tools/legacy/requirements-probe.txt`.
Ejemplo desde una instalación aislada:

```powershell
python -m pip install -r tools/legacy/requirements-probe.txt
python tools/legacy/probe_factory.py --legacy-root D:/Projects/Gloom-Legacy --output-root D:/Projects/Gloom/.cache/legacy-review/probe
New-Item -ItemType Directory -Force .cache/legacy-review/cooked
& build/windows-vs/Debug/gloom_asset_cooker.exe D:/Projects/Gloom/.cache/legacy-review/probe D:/Projects/Gloom/.cache/legacy-review/cooked game:/factory-probe.gltf cache:/factory.gasset
```

Los originales quedan intactos. El glTF experimental conserva unidades y UV0,
convierte 26 imágenes de color/normal a PNG y registra las 38 referencias de
textura del material. Usa roughness provisional 0.55 y metallic 0; todavía no
traduce specular, glow, blending multipase, entidades, colisión ni animaciones.
**No es la versión de Factory integrada ni una referencia visual aprobada.**
La comprobación visual definitiva de UV, handedness del normal map, luz y
materiales pertenece a los hitos de aceptación anteriores.

Evidencias conservadas en `reports/legacy-review-2026-09-02`; salidas grandes y
descarga de referencia en `.cache/legacy-review`. El resto de hitos queda
aplazado. Orden de ejecución: **59 → 60 → 61 → 62 → 63 → 64**.
