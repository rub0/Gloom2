# Factory original y materiales: hitos 59–61

Entrega del 2 de septiembre de 2026: **33/33 pruebas Debug superadas**.
Factory original se usa en la partida local, host/join y servidor dedicado.
La referencia de apariencia es el primer tramo de dos minutos del
[vídeo facilitado](https://www.youtube.com/watch?v=yJPoulcfIAg), distinguiendo
Factory de Dungeons. Véanse el [informe visual](../reports/factory-restoration-2026-09-02/README.md)
y [ADR 0062](architecture/0062-original-factory-and-extended-materials.md).

## Ejecutar y revisar

Desde la raíz del repositorio:

```powershell
& 'D:\Dev\CMake\bin\cmake.exe' --preset windows
& 'D:\Dev\CMake\bin\cmake.exe' --build --preset windows-debug
& 'D:\Dev\CMake\bin\ctest.exe' --preset windows-debug --output-on-failure
.\build\windows-vs\Debug\gloom.exe --vertical-slice
# Host y cliente se ejecutan en procesos separados:
.\build\windows-vs\Debug\gloom.exe --vertical-slice-host 0.0.0.0:27020
.\build\windows-vs\Debug\gloom.exe --vertical-slice-join 127.0.0.1:27020
# Alternativa al host gráfico:
.\build\windows-vs\Debug\gloom_slice_server.exe 0.0.0.0:27020
```

WASD mueve, ratón orienta, Space salta, LMB dispara y Q/RMB usa la habilidad
del loadout. La selección y las reglas de identidad/red anteriores se conservan.
Los modelos de personaje, arma FPS y habilidades siguen siendo provisionales.

```powershell
.\build\windows-vs\Debug\gloom.exe --factory-review .cache\factory-review
.\build\windows-vs\Debug\gloom_visual_capture_tests.exe .cache\factory-review tests\factory-visual-references --factory
& 'D:\Dev\CMake\bin\ctest.exe' --preset windows-debug -R 'gloom\.(factory_visual_review|material_render|factory_restoration)' --output-on-failure
```

Las seis cámaras cubren planta, spawn bajo, dos spawns altos, lava y pasarela.
Esperan residencia completa y 32 fotogramas de estabilización por vista, con
tiempo de material fijo. CTest guarda capturas, log de render y comparación
en `build/windows-vs/factory-review/Debug`; falla ante errores de Vulkan.
Las referencias son miniaturas inspeccionadas de 160×90. Se conservan los
umbrales y controles negativos de [VISUAL_REVIEW.md](VISUAL_REVIEW.md), y las
nueve referencias del blockout siguen pasando sin regenerarse.

## Regenerar el contenido original

El build normal usa `assets/legacy` y no necesita Ogre. Para regenerar, usar
Python 3.12 con las dependencias fijadas; estas herramientas son offline:

```powershell
py -3.12 -m venv .cache\legacy-tools
& '.cache\legacy-tools\Scripts\python.exe' -m pip install -r tools\legacy\requirements-import.txt
& '.cache\legacy-tools\Scripts\python.exe' tools\legacy\import_factory.py --legacy-root D:\Projects\Gloom-Legacy --output-root assets\legacy --work-root .cache\legacy-import
& '.cache\legacy-tools\Scripts\python.exe' tools\legacy\verify_import.py --legacy-root D:\Projects\Gloom-Legacy --reference-root assets\legacy --work-root .cache\legacy-reproduction
```

La verificación convierte en un directorio separado, compara cada archivo
generado y comprueba los SHA-256 de las fuentes. Resultado de esta entrega:
80 archivos idénticos, 111 fuentes verificadas, cuatro controles negativos del
parser y 15 entradas de malla auditadas. La copia original permanece limpia
en `fe59e723594cc13e0fc95a1f390d0f81f58dc45b`.

`factory_scene.json` registra fuentes, versiones de herramientas, entidades,
arquetipos resueltos, materiales, diagnósticos, diferencias con `Factory.map`
y colisión transformada. La escena conserva 97 registros únicos del par TXT;
no suma sus duplicados ni los de las copias `.map`. El glTF contiene 14 mallas
estáticas, incluidas las instancias recuperadas y el plano generado de lava.
La malla principal conserva 16 submallas y 16.351 triángulos. Soul Reaper y
armourSmall v1.41 tienen además conversiones independientes comprobadas.

Los nombres se resuelven mediante los grupos de `resources.cfg`. Una colisión
de nombres con distintos bytes falla; no se elige el primer resultado de una
búsqueda recursiva. Los datos Lua se analizan como literales restringidos.
Las mallas antiguas se normalizan en copias y un rig no se acepta como estático.

## Contrato de materiales

El formato cocinado de escenas es **versión 3**. El importador, serialización,
residencia, subidas GPU y shaders comparten diez slots y dos juegos de UV.
Las texturas de color usan sRGB; los datos se conservan lineales. Una imagen
usada con semánticas incompatibles provoca un error del cooker.

| Slot | Entrada | Canales / interpretación |
| --- | --- | --- |
| 0 | Base color | RGB sRGB, A opacidad |
| 1 | Metallic/roughness | B metallic, G roughness, lineal |
| 2 | Normal | XY tangente, BC5, Z reconstruida; intensidad configurable |
| 3 | Emisión | RGB sRGB multiplicado por factor HDR |
| 4 | AO | R lineal, intensidad configurable |
| 5 | Peso especular | A lineal por factor especular |
| 6 | Color especular | RGB sRGB por color especular |
| 7 | Anisotropía | RG dirección tangente remapeada a −1…1, B intensidad |
| 8 | Detalle | RGB sRGB multiplicativo |
| 9 | Lightmap | RGB sRGB, modulación de iluminación ambiental |

Se importan `KHR_materials_specular`, `KHR_materials_anisotropy`,
`KHR_materials_emissive_strength` y `KHR_texture_transform`. Cada slot conserva
UV0/UV1, escala, offset y rotación. `extras.gloom` admite `uvScroll`, `lavaWave`,
`additive`, `detailTexture` y `lightmapTexture`; los dos últimos son índices
de textura glTF. Véase `assets/tests/material_surface/scene.gltf` como ejemplo.

Los cuatro modos son opaco, alpha mask, blend y aditivo. Mask recorta también
sombras; blend/aditivo se ordenan de lejos a cerca sin escribir profundidad.
La doble cara es una propiedad del material. El filtrado de texturas y la
anisotropía de reflexión son funciones distintas: el shader implementa esta
última con un lóbulo GGX en espacio tangente.

Los `_SPEC` originales se conservan como color especular dieléctrico, con
metallic=0; la conversión de shininess usa `(2/(shininess+2))^0.25`. Glow
reproduce la multiplicación de difuso por máscara, evitando lava blanca por
usar la máscara como color. UV scroll y ondas reproducen el movimiento de lava
de forma aproximada; el shader HLSL/Cg antiguo no se ejecuta literalmente.

El ambiente usa `factory.ibl`: una sonda HDR estática 32×32 de seis caras y seis
mips GGX, generada desde World y lava en `(-10,6,-15)`. Se combina con luz
direccional, sombras, una luz cálida recuperada de la variante standalone y
bloom controlado. Es una aproximación global: no ofrece reflejos dinámicos,
volúmenes de sondas locales ni iluminación global completa. Los samplers usan
repeat; los modos de sampler individuales de glTF siguen fuera de este subset.
No incluye ejecución de grafos Unreal, clear coat, parallax ni transmisión.

## Escena compartida y red

Una escala **0,15** convierte la cápsula original de altura 12 en 1,8 metros.
Se aplica a mallas, entidades, spawns, luces, lava y colisión. RepX conserva
3.564 puntos y 5.856 triángulos; su pose de actor +90° en X se aplica a física,
sin volver a rotar la malla visual. Las barreras originales pueden sobresalir
del suelo visible; el informe incluye su superposición.

Los nueve spawns tienen soporte Jolt y rayos de suelo coherentes. Movimiento
autoritativo y predicción consultan los mismos triángulos con estado explícito
para repetir inputs sin historia oculta. Los disparos quedan ocluidos por las
paredes originales. El vacío carece del antiguo plano infinito; la lava ocupa
su posición fuente y el daño cubre también una caída por debajo del plano.
El ascensor inventado del blockout no aparece en Factory original.

El protocolo **12** añade una huella de colisión, spawns y lava; clientes con
otra escena son rechazados antes de predecir. No es una firma de seguridad y
no reemplaza la autenticación. Los tests mantienen admisión, perspectiva por
entidad, predicción, reconexión y tráfico real GNS con dos clientes. El dedicado
arranca con la misma Factory mediante su prueba de ejecutable.

## Límites y trabajo siguiente

- El manifiesto conserva pickups, mejoras, jumper y demás objetos con su
  estado de implementación. Recuperar su arte no añade mecánicas jugables.
- Una Minigun con esqueleto queda pendiente de 62; la importación lo diagnostica.
  Falta en origen la textura opcional `ironhellgoat_glow.png`; se conserva el
  resto de su material y se registra esa ausencia.
- Soul Reaper se ha convertido y validado como asset; la sustitución del arma
  FPS y sus anclajes pertenece a 62. Personajes/rigs, animaciones/VFX y HUD son
  62, 63 y 64 respectivamente.
- El vídeo se compara visualmente por geometría y materiales, con cámaras
  distintas. Las pruebas automáticas comparan capturas modernas inspeccionadas;
  no afirman igualdad de píxeles con un vídeo comprimido del antiguo motor.
- El manifiesto y la sonda usan el montaje de fuentes existente. Empaquetado
  autónomo, optimización y distribución siguen en la fase posterior acordada.
