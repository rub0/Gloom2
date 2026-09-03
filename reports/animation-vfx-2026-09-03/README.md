# Hito 63: animación, skinning, FPS/TPS y partículas

Entrega del 3 de septiembre de 2026 en `D:\Projects\Gloom`.
Estado de aceptación final: **completado**, con **38/38 pruebas superadas**,
revisiones temporales y comparación con los bocetos y el vídeo indicado.
Alcance acordado: [traspaso original](../../docs/HITO_63_HANDOFF.md).
El hito 64 y las ampliaciones posteriores quedan fuera de esta entrega.

## Resultado implementado

- Cuatro clips originales de Archangel/Hound, mezcla de locomoción, salto y
  capas de combate. Shadow conserva sus 17 huesos y recibe movimiento
  procedural de torso/cola: el original no contenía clips.
- Escenas versión 5, validación TRS, evaluación CPU y skinning GPU con ocho
  influencias, normales/tangentes, pose anterior, sombras y bounds deformados.
- Soul Reaper y brazos FPS de geometría original, IK de agarre, equipar,
  recoil y anclajes evaluados. Los cortes de vida/identidad/reconnect invalidan
  el historial; la muerte termina en una pose inmóvil hasta el respawn.
- Catorce recetas configurables con emisores continuos y ráfagas, vida finita,
  semillas reproducibles, billboards/estelas, alfa/aditivo, profundidad suave,
  HDR/bloom y distorsión sobre una copia del color/profundidad opacos.
- Protocolo 14 para pitch y disparos confirmados identificables, sin duplicar
  efectos al repetir snapshots ni reproducirlos al reconectar. Se conservan
  IDs, cápsulas, rayos de cámara, daño 80, cooldown 0,5 s y respawn 4 s.

Detalles, comandos y decisiones en [ANIMATION_VFX.md](../../docs/ANIMATION_VFX.md)
y [ADR 0064](../../docs/architecture/0064-animation-and-combat-effects.md).
El número del ADR no cambia el alcance del hito: esta entrega es el **63**.

## Evidencia técnica

La verificación inicial se hizo antes de editar. Aunque el hito 62 registraba
35/35, esta ejecución obtuvo 34/35: `gloom.match_https` falló por asumir que una
cuota agotada seguía vacía después de una petición TLS. Su repetición aislada
sin cambios pasó. Se conserva tanto [la ejecución inicial](tests-baseline.log)
como [la repetición](tests-baseline-https-rerun.log).

Se corrigió únicamente esa prueba: un reloj fijo verifica que renovar no
reinicia la cuota; el recorrido HTTPS permite solo los tokens recargados por
el tiempo realmente transcurrido. Las cuotas de producción no cambian.
El smoke de GPU también adapta su presupuesto al nuevo tamaño real de vértice
para conservar su comprobación de cargas diferidas. Se guarda la ejecución
anterior de 37/38 en [tests-before-smoke-budget-fix.log](tests-before-smoke-budget-fix.log).

La suite final pasa **38/38 en 177,53 s**, incluidas todas las regresiones
visuales anteriores: [tests-final.log](tests-final.log) y
[salidas completas](tests-detail.log);
la compilación completa, en [build-final.log](build-final.log).
Las tres pruebas nuevas cubren:

| Prueba | Comprobaciones relevantes |
| --- | --- |
| `gloom.animation_vfx` | Clips reales, entradas malformadas, serialización, bind, cinco influencias de Shadow, deformación, agarre, bounds, cortes, cadáver inmóvil, poses 30/60/144, partículas limitadas y deduplicación/limpieza |
| `gloom.animation_network` | Recorrido de Factory original, dos disparos, daño/muerte, cuatro segundos de respawn, host/join GNS, presentación CPU, reconciliación y reconnect |
| `gloom.animation_dedicated` | Servidor ejecutable separado y dos clientes GNS independientes, mismo combate y reconexión sin repetición de efectos |

Los dos clientes de aceptación del dedicado son adaptadores reales de
transporte/protocolo dentro de un mismo proceso de prueba; no son dos ventanas
gráficas. El servidor es otro proceso. Los [logs del dedicado](dedicated/)
identifican admisiones, combate y reanudación: dos disparos, una muerte en tick
1239, respawn en 1479, ocho eventos visuales y reconexión de la entidad 1.
Las secuencias Vulkan de abajo
verifican la presentación por separado; no se presentan como vídeo de esa
partida de red. El dedicado no instancia animadores ni partículas.

## Procedencia y fidelidad

[reproduction-characters.log](reproduction-characters.log) acredita 20 archivos
idénticos y 17 fuentes verificadas. [effects-reproduction.log](effects-reproduction.log)
acredita 11 archivos idénticos y 358 fuentes verificadas: 349 scripts/materiales
auditados, siete texturas recuperadas y 14 adaptaciones modernas. Ogre se usa
solo offline. Las conversiones no modifican `D:\Projects\Gloom-Legacy`.

[clip-audit.json](clip-audit.json) registra tiempos, claves y costuras de los
cuatro clips, con 129 canales TRS por clip. La mayor diferencia entre extremos
es la traslación de `forward`: 0,047842 unidades fuente, unos 5,76 mm a escala
visual. Se conservan las claves fuente; el controlador suprime la traslación
visual de pelvis para que nunca conduzca la cápsula. Las costuras de rotación
y escala son nulas dentro de la precisión medida.

El [manifiesto](manifest.json) contiene hashes de capturas, vídeos, fuentes y
ejecutables, además de la comparación entre frecuencias. [preservation.json](preservation.json)
comprueba archivos anteriores conservados, referencias visuales sin cambios y
el original limpio en `fe59e723594cc13e0fc95a1f390d0f81f58dc45b`.
Las iteraciones previas se guardan aparte; no son la evidencia final.

## Secuencias temporales y aceptación visual

Cada vídeo principal contiene 40 segundos simulados y 1.200 imágenes a 30 Hz.
El render avanza a paso fijo de 30, 60 o 144 Hz; estas cifras **no son una
medición de rendimiento en tiempo real**. Los primeros 20 segundos usan luces
de inspección; los siguientes, la iluminación normal de Factory. Cada personaje
tiene un bloque de diez segundos con locomoción, salto/aterrizaje, disparo,
daño/escudo, muerte de cuatro segundos, cámara arriba/abajo y respawn.

- [Vídeo 30 Hz](videos/animation-30hz.mp4)
- [Vídeo 60 Hz](videos/animation-60hz.mp4)
- [Vídeo 144 Hz](videos/animation-144hz.mp4)
- [Lava y calor de Factory](videos/lava.mp4)
- [Muestras de sangre, explosión y calor](videos/effects-samples.mp4)

Las tres ejecuciones completan 41 eventos, 5.330 nacimientos, 4.666 expiraciones
y cero descartes por capacidad. Los cinco logs terminan correctamente, sin
errores de validación Vulkan. Hay 3.780 capturas finales, con hashes individuales.

MAE de color RGB en escala 0–255, comparando los mismos instantes nominales:

| Comparación | Imagen completa, media | Cuerpo, media | FPS, media | Imagen completa, p95 | Máximo de imagen completa |
| --- | ---: | ---: | ---: | ---: | ---: |
| 30 frente a 60 Hz | 1,657 | 2,330 | 2,612 | 3,655 | 4,977 |
| 30 frente a 144 Hz | 2,598 | 3,465 | 3,561 | 6,189 | 27,423 |

El pico de 144 Hz cae en el fotograma 259 (8,633 s), durante el barrido rápido
de cámara: la cuantización de captura de hasta 6,94 ms desplaza las aristas del
suelo ajedrezado. Se inspeccionaron [los casos de mayor diferencia](rate-worst-cases.png),
incluidos equipar y la caída. Se observan diferencias de fase y del historial
temporal, sin pérdida de agarre, deformaciones explosivas ni cambio sustancial
de los efectos. Las pruebas CPU contrastan por separado las poses a igual
tiempo y la identidad/capacidad de partículas entre las tres frecuencias.
Las cifras completas están en [sequence-comparison.log](sequence-comparison.log).

Referencias suministradas: [bocetos](../../docs/art/characters/README.md) y el
[vídeo original, solo 0:00–2:00](https://www.youtube.com/watch?v=yJPoulcfIAg).
La revisión del intervalo y sus contactos usa Factory como referencia de
movimiento/efectos, sin sustituir el mapa por Dungeons. Los instantes 0:23,
0:36, 0:45 y 1:59 muestran ráfagas, movimiento del arma, lava y distorsión.

| Elemento | Resultado de la inspección y evidencia |
| --- | --- |
| Archangel | Conserva armadura dorada y emisión cian; las partículas de alas acompañan la pose sin tapar el contorno. [Comparación con boceto](concept-comparison.png) |
| Shadow | Conserva metal oscuro y ojos rojos; la cola se curva, el humo nace bajo ella y las estelas siguen la cabeza. [Detalle temporal](shadow-closeup.png) |
| FPS/TPS | Manos y arma siguen la pose; arriba/abajo mantienen el agarre y los cortes de hombro fuera de cámara. Factory sigue siendo más oscuro que la inspección. [Vistas](fps-lighting-contact.png) |
| Disparo | Halo breve delante del cañón, seguido de desaparición; impacto y escudo usan eventos separados. [Fogonazo en seis fotogramas](muzzle-closeup.png), [efectos](effects-contact.png) |
| Muerte/respawn | Caída de 0,65 s y pose final inmóvil hasta reaparecer; el historial no mezcla vidas. [Secuencia de movimiento](motion-contact.png) |
| Lava | Plano y materiales originales, brasas pequeñas y calor integrado con profundidad. [Capturas de Factory](lava-contact.png) |
| Sangre/explosión/calor | Muestras acotadas visibles en zona despejada, con disipación y distorsión suave. [Contacto de muestras](effects-samples-contact.png) |
| Vídeo original | Se conservan mapa, ritmo breve de ráfagas e intención de glow/lava/calor; no se copia el HUD ni otras armas del vídeo. [Instantes de referencia](video-reference-contact.png) |

## Límites explícitos

Las capas procedurales y la traducción de Particle Universe no pretenden ser
reproducción exacta de Ogre. La geometría original limita los dedos y el detalle
de los brazos. La iluminación normal conserva las superficies oscuras del mapa.
Los fogonazos esperan confirmación autoritativa, con su latencia correspondiente.
Sangre y explosión solo tienen muestras de revisión porque no hay un evento
jugable apropiado; no se han inventado armas, habilidades ni recarga.
Capacidad: 64 emisores y 2.048 partículas; se descarta el nacimiento nuevo al
saturarse. La distorsión lee opacos, no refracta otras partículas transparentes.

HUD, menús, lobby, audio, balance y los demás hitos siguen pendientes.

## Regenerar esta evidencia

Los comandos de compilación, pruebas y capturas están en
[ANIMATION_VFX.md](../../docs/ANIMATION_VFX.md). Para regenerar este paquete en
el entorno local utilizado, después de las capturas:

```powershell
$python = 'C:\Users\rubo8\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$env:PYTHONPATH = (Resolve-Path .cache/legacy-review/python).Path
& $python tools/legacy/verify_characters.py --legacy-root D:/Projects/Gloom-Legacy --reference-root assets/characters/original --work-root .cache/character-reproduction
& $python tools/legacy/verify_effects.py --legacy-root D:/Projects/Gloom-Legacy --reference-root assets/effects --work-root .cache/effects-reproduction-final
& $python tools/review/animation_report.py --report reports/animation-vfx-2026-09-03 --ffmpeg .cache/legacy-review/bin/ffmpeg.exe --source-root . --reference-video .cache/legacy-review/reference.mp4
```

Python/Pillow/numpy, el runtime offline de Ogre y ffmpeg son herramientas de
conversión/revisión. El build y el juego usan los assets convertidos del repo.
