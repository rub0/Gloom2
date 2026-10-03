# Hito 109: formato C++ uniforme

Fecha: 3 de octubre de 2026. Base: `81a05d0` (hito 108).

## Cambios

- `.clang-format`: sangrado de cuatro espacios, objetivo de 160 columnas,
  llaves en la misma línea y cuerpos no vacíos separados en líneas. Conserva
  orden de includes y declaraciones `using`, comentarios y literales.
- 183 archivos C++ propios revisados; 181 reformateados en `include`, `src`,
  `apps` y `tests`. Cambian espacios y saltos de línea; no instrucciones.
- Dos objetivos opcionales de CMake: `gloom_format` aplica el estilo;
  `gloom_format_check` comprueba sin escribir y devuelve error si hay diferencias.
  Usa clang-format ya instalado, localizado en PATH o en el Visual Studio
  seleccionado por CMake. Sin paquetes nuevos ni pasos adicionales al build normal.
- Uso documentado en [DEVELOPING.md](../../DEVELOPING.md).

El objetivo de columnas admite los límites del formateador: cuatro líneas de
HLSL dentro de literales y una expresión con lambda superan 160 columnas.
Se conserva su contenido; no se fuerza una transformación de instrucciones
para ajustar la presentación. El formato no comprueba las reglas semánticas
de `AGENTS.md`; no se activan reescrituras automáticas de clang-tidy.

## Comprobaciones

- clang-format **19.1.5**, incluido en Visual Studio 2022 instalado.
- Comparación léxica antes/después de los 183 archivos: **324.356 tokens**
  iguales y en el mismo orden, incluyendo identificadores, operadores,
  números, caracteres y cadenas normales/raw. También se comparan comentarios
  y directivas, ignorando sus espacios de presentación. No se añaden includes,
  tipos, conversiones, estados, asignaciones, ramas ni recursos.
- El objetivo `gloom_format_check` pasa. Una declaración deliberadamente mal
  espaciada enviada por entrada estándar se rechaza con salida distinta de cero.
- El objetivo `gloom_format` pasa; repetirlo conserva exactamente los hashes
  de los 183 archivos, comprobando que el formato es estable.
- Builds Release y Debug completos correctos, con los controles de warnings
  existentes.
- Suite Release completa: **45/49** pasan, **105,88 s**. Fallan únicamente
  `gloom.animation_network` (GNS 25), `gloom.ui_visual_review`,
  `gloom.factory_visual_review` y `gloom.character_visual_review`, el mismo
  conjunto heredado del hito 108.
- Suite Debug completa: **46/49** pasan, **255,00 s**. Fallan las mismas tres
  comparaciones visuales heredadas; animación en red y dedicado pasan.
- Vulkan/sincronización, Hound runtime, cooker/assets, TLS/identidad, física,
  movimiento, audio y flujo de UI pasan en ambos builds. No se detectan
  regresiones nuevas. Las suites completas no están verdes por los fallos
  previos descritos, y sus umbrales y referencias se conservan.
- `git diff --check` pasa.

La comparación léxica y las copias previas de trabajo están en `.cache`;
no forman parte del código, las dependencias ni los artefactos publicados.
Las instrucciones para repetir la comprobación cotidiana están en DEVELOPING.

## Alcance y rendimiento

El orden de evaluación, las llamadas de API, las políticas de lifetime y los
parámetros Span/ByteSpan/GpuRange se conservan. No hay cambios de algoritmos,
asignaciones de memoria o representación de datos; no se atribuye una mejora
de rendimiento al formato. Se conservan fuentes artísticas, exportaciones,
referencias visuales, scripts y shaders en archivos separados. H08 sigue sin iniciar.

Los fallos heredados registrados en el [hito 108](../ponytail-108/README.md)
siguen siendo limitaciones conocidas; no se modifican sus referencias ni se
relajan sus umbrales como parte de esta tarea.

Commit local de cierre; consultar `git log -1 --oneline --grep='^hito 109:'`.
No se ha realizado push.
