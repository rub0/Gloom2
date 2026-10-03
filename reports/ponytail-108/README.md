# Hito 108: simplificación Ponytail

3 de octubre de 2026. Entrada: `bb79cbf` (hito 107). Encargo: aplicar los siete
recortes de la auditoría y comprobar regresiones. No se inicia H08.

## Cambios

- Eliminados grafo, declaración por frame y prueba sintética: el renderer
  descartaba orden, barreras y alias calculados. Se conservan las llamadas
  Diligent que ejecutan las pasadas y sus transiciones de recursos.
- Una función local carga Factory, personajes, armas, habilidades y efectos.
  Conserva prioridades, tickets, propietarios y alternativas visuales. El
  contenido obligatorio original/diagnóstico falla durante inicialización.
- Cooker y catálogo comparten resolución de imágenes y ruta de dependencias.
  Se conserva el rechazo de referencias que escapan del montaje virtual.
- Retirados hooks y opciones FSR/DLSS sin adaptadores. TAA/desactivación,
  parámetros, jitter, historial, resolución dinámica y sharpening conservados.
- Las revisiones de arte reutilizan configuración/cámara H01 y grupos H04.
  `keep_prefix=True` conserva las claves `PART_` de los controles v04–v08.
- Tabla explícita para 21 registros CMake uniformes. Nombres, comandos,
  dependencias, entornos, etiquetas y límites particulares conservados.
- Petición WinHTTP común con API de tipos propios y cabeceras C. Se conservan
  verificación TLS, prohibición de redirects, timeout 3/3/5/5 s, cabeceras,
  sufijos de partidas con query y límites de respuesta de 64 KiB/4 MiB.
  Permisos, renovación, cuotas y tratamiento de códigos HTTP siguen en cada
  cliente. La respuesta adquirida se libera inmediatamente después de copiarla
  a las interfaces existentes; handles y buffers se liberan también al fallar.

Sin dependencias nuevas ni cambios de protocolo, shaders, gameplay, referencias
visuales, fuentes Blender o exportaciones. El recorte elimina trabajo y
asignaciones del grafo por frame; no se atribuye una ganancia de FPS sin medirla.
Las interfaces heredadas del resto del proyecto mantienen sus tipos existentes.
Balance de código, herramientas y pruebas: **839 líneas netas menos**, sin
dependencias añadidas. Se cuentan CMake y las comprobaciones nuevas; se
excluye documentación del cálculo.

## Comprobaciones

- Compilación completa Release y Debug, con los avisos tratados como errores.
- Registro CTest cotejado con el CMake de entrada: 49 pruebas conservadas;
  desaparece exclusivamente `gloom.render_graph`, junto con su implementación.
- Pruebas de rutas compartidas: separadores, `.`/`..`, rechazo de escape y
  layout hexadecimal del cooker. Regresión de cooker/catálogo/loader existente.
- Prueba temporal sin biblioteca estándar C++, con asserts activos en Release:
  TAA, desactivación, cuatro capacidades ausentes, límites, jitter, minimizar
  e histéresis de resolución dinámica.
- Prueba de límites HTTPS nativos para seis URLs inválidas, antes de conexión.
  Las pruebas existentes cubren rechazo de certificado no confiable, OAuth,
  permisos, renovación, revocación, tickets, TLS local y reinicio durable.
- Blender 4.5.13: `verify_shared_helpers.py`, cámara real, setup, grupos vacíos,
  grupos solapados, grupos de huesos y claves con/sin prefijo. También comprueba
  las importaciones de los nueve scripts modificados sin ejecutar sus guardados.
  Sin guardar assets.
- Sintaxis de los 74 scripts de arte comprobada mediante el parser de Python.

La primera batería completa Release conserva 45/49 resultados positivos:
fallan `animation_network` (GNS resultado 25), `ui_visual_review`,
`factory_visual_review` y `character_visual_review`. Vulkan y Hound pasan.
Los mismos cuatro casos fallan sobre `bb79cbf`, compilado con el mismo toolchain,
contenido y configuración. Sus cambios de trabajo se guardaron y restauraron
íntegramente para la comparación. No se modificaron referencias para hacerlos
pasar. Son fallos anteriores a este encargo, no una batería completamente verde.

### Comparación visual contra el código de entrada

Se usan capturas del hito 107 como referencia temporal en caché y el comparador
existente, incluidos sus controles de imagen vacía/primer plano ausente. Los
umbrales permanecen: media ≤4/255, píxeles con diferencia >32 ≤1,5 % y peor
bloque ≤12/255. Métricas sobre las miniaturas 160×90 del comparador.

| Grupo | Capturas | Media máxima /255 | Píxeles >32 | Peor bloque /255 | Resultado |
| --- | ---: | ---: | ---: | ---: | --- |
| Interfaz | 51 | 0 | 0 % | 0 | Pasa |
| Factory | 6 | 1,63771 | 0 % | 4,31556 | Pasa |
| Personajes/arma | 7 | 1,99764 | 0 % | 3,66222 | Pasa |

Las 64 capturas pasan frente a la versión de entrada; este control distingue
el efecto del refactor de los fallos contra las referencias históricas.

### Baterías completas sobre los ejecutables finales

| Configuración | Pasan | Fallan | Tiempo |
| --- | ---: | ---: | ---: |
| Release | 45/49 | Los cuatro casos anteriores | 105,69 s |
| Debug | 46/49 | Las tres comparaciones visuales anteriores | 250,64 s |

`animation_network` pasa en Debug; su fallo Release también aparece en el
código de entrada, siempre con el error GNS 25 observado. Las tres comparaciones
visuales fallan en ambas configuraciones contra las referencias históricas.
Vulkan, Hound, rutas/cooker, TAA, identidad/HTTPS, audio, gameplay y flujo de
interfaz pasan en ambos builds. No se detectan regresiones nuevas en las pruebas
ejecutadas. Los cuatro fallos existentes quedan pendientes de un encargo propio.

Estado actualizado y commit local del hito 108, sin push. Resolver su hash con
`git log -1 --oneline --grep="^hito 108:"`.

## Reproducción

Desde la raíz, `rtk proxy` seguido de:

1. `D:/Dev/CMake/bin/cmake.exe --build --preset windows-release --parallel 6`
   y el preset equivalente `windows-debug`.
2. `D:/Dev/CMake/bin/ctest.exe --preset windows-release --output-on-failure`
   y el preset equivalente `windows-debug`. Ejecutar secuencialmente.
3. `.cache/blender/blender-4.5.13-windows-x64/blender.exe --background
   --factory-startup --python-exit-code 1 --python tools/art/verify_shared_helpers.py`.

Builds, logs, capturas y copias temporales de comparación quedan en `.cache/`
y `build/`, excluidos de Git. Informe y estado forman parte del commit local.
