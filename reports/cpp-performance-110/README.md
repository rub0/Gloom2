# Hito 110: primera migración C++ y buffers reutilizables

Fecha: 3 de octubre de 2026. Base: `973f5de` (hito 109).

## Resultado

Visibilidad e iluminación conservan sus buffers entre frames. No se cambia
de renderer, shaders, contenido, dependencias ni hardware objetivo. Se usa
[NoGraphicsAPI](https://github.com/sebbbi/NoGraphicsAPI) como referencia de C++,
sin incorporar su API gráfica ni sus requisitos de GPU.

- Base propia mínima: `Span<T>` (puntero/tamaño, arrays e initializer lists),
  `Array<T>` (reserva explícita, crecimiento geométrico, sin copia implícita)
  y reloj monotónico nativo compartido con el benchmark existente.
- Los resultados pertenecen al llamador y se reconstruyen por referencia.
  Juego y viewer los mantienen fuera del loop. Snapshots/views vencen al
  reconstruir o destruir su resultado; los resultados independientes conservan
  sus datos. Entradas y resultados no pueden solaparse; asserts documentados.
- Visibilidad mantiene un array de índices y resúmenes por trabajo. No copia
  las instancias descartadas ni las mueve durante la ordenación: ordena registros
  triviales con `qsort` de C y copia cada instancia aceptada una vez al resultado.
  Material/mesh determinan el orden; los empates conservan el índice de entrada.
  Se mantienen frustum, escala reflejada/no uniforme, LODs y batches.
- Iluminación calcula los límites de cada luz una sola vez. Usa los propios
  rangos como contadores/cursor y los restaura después del relleno. Desaparecen
  los arrays temporales de conteos y offsets. Se mantiene la prioridad de luces,
  el límite por celda y el significado de `saturated_clusters`.
- Tipos enteros propios, `Span` por valor, estructuras grandes por referencia,
  sin `auto`, lambdas ni uso directo de STL en ambas implementaciones. Las
  precondiciones de programación se comprueban con asserts.
- Reloj, visibilidad, iluminación y sus tres pruebas se compilan sin excepciones
  ni RTTI. MSVC emite D9025 al sustituir las opciones de excepciones heredadas;
  las opciones finales se verifican en los proyectos generados.

## Medidas

Ryzen 7 3700X / GTX 1070, Release, Factory nativo 1920×1080, VSync desactivado,
120 frames de calentamiento y 360 medidos. Tres pasadas seriales antes y tres
después, sin builds/pruebas simultáneos. El escenario es el benchmark existente
de un TPS y un FPS; no certifica una partida humana de ocho jugadores.

| Mediana de las tres pasadas | Antes | Después |
| --- | ---: | ---: |
| CPU visibilidad, media por pasada | 36 µs | 27 µs (-25 %) |
| CPU iluminación, media por pasada | 38 µs | 18 µs (-52,6 %) |
| CPU ambas etapas | 74 µs | 45 µs (-39,2 %) |
| Frame completo, media por pasada | 2,338 ms | 2,337 ms |
| FPS | 427,66 | 427,84 |

No se atribuye una mejora del frame completo: la diferencia es despreciable.
Las seis distribuciones (media/p50/p95/p99/máximo) y las etapas se conservan en
[medidas.md](medidas.md), incluyendo los máximos de 3,996 y 3,552 ms.
Todas las pasadas: 249 instancias, 96 visibles, 36 batches y 340 draws;
cero meshes/texturas ausentes, cero frames medidos >5 ms, sin errores Vulkan.

`gloom.storage` intercepta `new/new[]`: 1.000 llamadas de iluminación calentada
no asignan memoria; la reserva que crece actúa como control de la instrumentación.
También comprueba 1.000 ciclos de resize/reserve, initializer-list durante su
llamada, conversión a vista const y liberación de propietarios al reducir,
crecer y destruir el array. Visibilidad todavía crea grupos/trabajos mediante
el JobSystem heredado: no se afirma cero asignaciones para esa etapa completa.

## Validación

- Builds completos Release y Debug correctos. Cinco pruebas focalizadas
  (storage, jobs, render_scene, visibility, lighting) pasan en ambos builds,
  repetidas tras el último ajuste del control de asignaciones.
- Release completa: **46/50**, 106,60 s. Solo fallan los cuatro casos heredados:
  animation_network (GNS 25), ui_visual_review, factory_visual_review y
  character_visual_review. Las referencias y los umbrales se conservan.
- Debug completa: **47/50**, 262,97 s. Solo fallan las mismas tres comparaciones
  visuales heredadas. Animation_network, Vulkan y Hound pasan; Vulkan/Hound
  también pasan en Release. No se detectan regresiones nuevas.
- Las pruebas amplían repetición/growth/shrink, salida vacía, resultados
  independientes, 1.025 instancias entre trabajos, agrupación determinista,
  escalas reflejadas y saturación exacta (256 celdas, primeras dos luces).
- Comparación antes/después: **13/13** capturas pasan los umbrales existentes.
  Los resultados del comparador están en [visual.md](visual.md).
  No son 13 imágenes idénticas.
- A resolución completa, las tres capturas del arma son idénticas; las cuatro
  de personajes cambian ≤0,001194 % de píxeles, máximo un nivel de canal.
  Factory cambia 73–80 % de píxeles con medias de 0,963–1,859/255; sus miniaturas
  dan medias de 0,387–0,692/255 y pasan. La vista general se inspecciona antes y
  después, sin pérdida visible de geometría. Las métricas completas están en
  [visual.md](visual.md).
- Revisión de rendimiento, ownership y parámetros Span por valor realizada.
  Comprobación de formato y `git diff --check` pasan.

## Alcance pendiente

Es la primera etapa de la migración. Scene/RenderInstance, sonda de entorno,
JobSystem y otros módulos aún contienen STL/shared_ptr, excepciones y RTTI.
La salida al RenderSnapshot existente usa su vista de compatibilidad sin copia.
No se afirma que todo el repositorio cumpla ya AGENTS.md. `FixedFunction` aún
no se añade: su consumidor real será la migración del JobSystem. El siguiente
frente es revisar el registro de entidades y después el almacenamiento y vida
de jobs/poses, con mediciones antes/después. H08 no iniciada.

Comandos reproducibles:

```powershell
rtk proxy D:/Dev/CMake/bin/cmake.exe --build --preset windows-release --parallel 6
rtk proxy D:/Dev/CMake/bin/cmake.exe --build --preset windows-debug --parallel 6
rtk proxy D:/Dev/CMake/bin/ctest.exe --preset windows-release --output-on-failure
rtk proxy D:/Dev/CMake/bin/ctest.exe --preset windows-debug --output-on-failure
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-1080p
rtk proxy D:/Dev/CMake/bin/cmake.exe --build --preset windows-release --target gloom_format_check
```

Logs y capturas de diagnóstico en `.cache/hito110` y `build/windows-vs`,
excluidos del commit. Commit local de cierre: consultar
`git log -1 --oneline --grep='^hito 110:'`. Sin push.
