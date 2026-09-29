# Hito 104 — presentación y skinning de ocho combatientes

29 de septiembre de 2026 · **Preparado para otra tarea; no iniciado**.
Este documento es el encargo de ejecución y el registro de su preparación.

## Entrada y objetivo

Leer primero [ESTADO_ACTUAL.md](../../docs/ESTADO_ACTUAL.md) y AGENTS.md.
Después, el [informe 102](../eight-combatants-102/README.md) y el apartado de
trabajo pendiente del [contrato H06](../../docs/art/hound/H06-contrato-presupuesto.md).
No hace falta cargar el historial ni todos los informes artísticos.

Base al preparar el encargo: `38477c3`, investigación 102; normales corregidas
en `576eda4`. Revisar el estado real de Git antes de editar y conservar cambios
posteriores. Objetivo vigente: **200 FPS, 1920×1080 nativo, hasta ocho combatientes,
Ryzen 7 3700X y GTX 1070**; 5 ms por fotograma completo.

Resolver los dos bloqueos medidos del motor y repetir la validación:

- `Skin constant mapping failed` con siete Hound v16 TPS más FPS local,
  antes de completar el calentamiento.
- Cadencia variable de MAILBOX/Present: una ejecución ronda 144 FPS con GPU
  ~2,3 ms; otra supera 200 FPS de media, pero p95/p99 incumplen 5 ms.

El hito 102 terminó la investigación, no certificó el objetivo ni aprobó H06.

## Trabajo acotado

1. Reproducir ambos casos con los modos existentes del hito 102. Trazar todas
   las cargas de paletas en pasadas opacas, sombras y FPS. Medir mapeos/bytes,
   frames en vuelo y vida de los datos antes de elegir el arreglo. Corregir la
   causa compartida del fallo de skinning; no ocultarlo omitiendo personajes,
   sombras o animación ni aumentar memoria sin explicar el límite observado.
2. Separar trabajo CPU/GPU, espera de Present y latencia de cola. Controlar y
   registrar modo efectivo, VSync, tamaño de ventana y resolución interna/salida.
   Comparar los modos de presentación necesarios manteniendo el mismo contenido.
   Aplicar el ajuste mínimo respaldado por esas medidas; no cambiar globalmente
   el controlador o el equipo para dar por corregido el juego.
3. Cuando siete Hound completen la prueba, medir poses, skin/bounds, draws,
   sombras y residencia/evicciones. Optimizar `skinned_bounds` solo si el perfil
   lo justifica; conservar bounds que cubran todas las poses y probarlos contra
   el cálculo de referencia para evitar desapariciones o sombras recortadas.
4. Repetir los casos de dos, ocho originales, un Hound y siete Hound más FPS
   en Release/1080p, en serie, tras carga y calentamiento. Conservar HUD/disparo
   y declarar si el audio usa salida nula o dispositivo real; medir este último
   cuando esté disponible. Registrar media, p50/p95/p99, máximo y frames >5 ms,
   además de tiempos GPU del intervalo completo y visibilidad/animación efectiva.
   720p sirve como control; no sustituye el objetivo de 1080p.
5. Devolver a H06 las medidas y los límites que permitan fundamentar un
   presupuesto, separando datos comprobados de estimaciones. Usar v16 y contenido
   existente: todavía no hay mapas ni clips finales de Hound. No exigir crearlos
   para comenzar este arreglo ni inventar un presupuesto final para cerrar H06.

## Referencias concretas

- [Backend gráfico](../../src/backends/diligent_renderer.cpp): `skin_constants`,
  `Skin constant mapping failed`, pasadas de sombras/opacos y Present.
- [Composición y benchmark](../../apps/gloom/main.cpp): modos
  `--vertical-slice-performance-two-full-1080p`,
  `--vertical-slice-performance-eight-1080p`,
  `--vertical-slice-performance-hound-1080p` y
  `--vertical-slice-performance-hound-eight-1080p`.
- [Animación](../../src/assets/animation.cpp): `skinned_bounds` y sus llamadas.
- Fuente inmutable: `art/characters/hound/v16/hound-mesh-v16.blend`.
  Exportación: `assets/characters/hound_rig/v16/hound-rig.gltf` y BIN.
  Clip diagnóstico: `Hound16_joint_check`. Hashes en el manifiesto v16.

## Aceptación, límites y entrega

Siete Hound más FPS deben completar carga, calentamiento y medida sin error
de mapeo, manteniendo animación visible, sombras y datos actual/anterior válidos.
Dejar una regresión reproducible que detecte el fallo original. Reutilizar las
pruebas de sincronización Vulkan, escena, visibilidad y animación pertinentes.
Comparar capturas y tiempos antes/después; ejecutar benchmarks GPU en serie.

No certificar 200 FPS por la media de una pasada rápida. Si el objetivo sigue
incumplido, registrar el coste restante y el siguiente paso preciso. Diferenciar
la corrección técnica del motor de la aceptación del contenido final de Hound.
No ampliar red/autoridad a ocho jugadores, modificar la escultura, crear mapas
o clips de producción, iniciar H07 ni reabrir normales sin una regresión concreta.

Al terminar: actualizar este informe con resultados y reproducción, el estado,
roadmap y contrato/índice H06. Crear y verificar el commit local según AGENTS.md;
entregar hash y estado del workspace. Sin push ni inicio de la siguiente tarea.

Preparación actual: solo documentación; rutas, enlaces, dependencias y diff
comprobados. No se han ejecutado benchmarks ni cambiado código o assets.
Commit local de preparación; no representa la ejecución o cierre técnico del 104.
