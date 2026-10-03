# Validación del 112

Builds completos Release y Debug terminados correctamente con diagnóstico OFF.
Build Release de diagnóstico ON y targets focales también correctos. Ninguna
fuente de arte, exportación, referencia visual o dependencia cambiada.

| Comprobación | Resultado | Evidencia local |
| --- | --- | --- |
| Build Release inicial sin cambios técnicos | Correcto | .cache/hito112/build-baseline-release.log |
| Build Release diagnóstico inicial/final | Correcto | build-profile-release.log, build-probes.log y build-probes-final.log; véase nota |
| Build final completo Release | Correcto | .cache/hito112/build-release.log |
| Build final completo Debug | Correcto | .cache/hito112/build-debug.log |
| Focal Release ON | 6/6 | .cache/hito112/focal-release.log |
| Suite final Release OFF | **48/52**, 105,25 s | .cache/hito112/suite-release.log |
| Suite final Debug OFF | **49/52**, 250,31 s | .cache/hito112/suite-debug.log |
| Formato | Correcto | .cache/hito112/format-check.log |
| Revisión de diff/contratos/performance | Completada | Informes y diff del commit 112 |

Nota de build: durante preparación se corrigió la ausencia de settings de
plataforma del probe Diligent. Dos invocaciones intermedias nombraron mal el target
jobs y terminaron con MSB1009; sus logs no se presentan como builds completos
correctos. El target correcto `gloom_job_tests`, el probe y los builds completos
posteriores terminan y sus pruebas pasan. No se conserva un fallo de compilación
en el resultado entregado.

Los dos CTest nuevos, allocation_profile y dependency_contract, pasan en ambos
builds. También storage/jobs/visibility/lighting, vulkan_sync y hound_runtime;
el último es probe v16, separado de las pasadas de presupuesto v17 del informe.

## Fallos heredados, comparados por causa

| Prueba | 110 Release/Debug | 112 Release/Debug | Causa |
| --- | --- | --- | --- |
| gloom.animation_network | Falla / pasa | Falla / pasa | GameNetworkingSockets send failed result 25; misma posición/cadencia de la simulación de la base. |
| gloom.ui_visual_review | Falla / falla | Falla / falla | Comparador contra referencias actuales; conserva referencias y umbrales. |
| gloom.factory_visual_review | Falla / falla | Falla / falla | factory-spawn-1 worst tile **31,2544/255**, frente a límite 12. Las seis métricas Release coinciden con el log 110. |
| gloom.character_visual_review | Falla / falla | Falla / falla | Imágenes contra referencias ya divergentes. archangel-front Release 110: mean 6,92431/255, changed 3,65278%, worst tile 19,9933; 112: 6,85396/255, 3,55556%, 19,9933. |

Se cotejaron `.cache/hito110/suite-release.log` y suite-debug.log. El cambio de
50 a 52 tests corresponde únicamente a las dos pruebas nuevas que pasan. No
se afirma que los tests heredados estén verdes, que todas las imágenes sean
idénticas o que sus diferencias queden resueltas por este hito de medición.
No se cambian referencias/umbrales para conseguir un total verde.

## Revisión de la instrumentación

- Nuevos C++ sin headers/facilities STL propios, auto ordinario, herencia,
  virtuales, PIMPL, lambdas, excepciones/RTTI o comprobaciones de OOM.
  Cabeceras nativas C/Windows y dependencias conservan sus tipos internos.
- `/GR- /EHs-c-` por fuente en los objetos nuevos del diagnóstico y probe,
  verificado en proyectos generados Release/Debug. Los módulos legacy aún tienen
  otros flags; el cierre global pertenece a 128.
- Contadores atómicos probados desde cuatro hilos, TLS por fase, metadata con
  lock nativo. Las tablas no asignan ni retienen nombres temporales. Scope no
  copiable, finaliza en orden inverso de anidación y puede terminar antes.
- Juego normal no incluye allocation_new.cpp ni allocation_profile.cpp, no
  define GLOOM_ALLOCATION_PROFILE y no emite reporte del diagnóstico en las
  siete pasadas normales finales. Macro OFF no cambia algoritmos ni firmas.
- Firmas nuevas no introducen referencias a Span/ByteSpan/GpuRange. Las llamadas
  instrumentadas conservan sus parámetros y su ownership; no se convierte un
  owner de una cola en una vista que caduque.
- Informe fuera de ventana. Array calentado + crecimiento y metadata de workers
  comprueban que el contador distingue cero real local de medición desconectada.

Las 28 pasadas verifican 1080p, samples=360, ausencia de errores de GPU y recursos,
y el mismo contenido; sus distribuciones completas y picos se conservan en
[medidas](medidas.md). Los fallos de presupuesto ya presentes en la base siguen
registrados. No se certifica cumplimiento estable H06 ni ahorro de FPS del 112.

Los bloqueos de frontera externa se permiten como resultado explícito de
investigación del 112, según su ficha. El código pendiente de 117/120/128 no se
certifica ni se inicia en este commit. `git diff --check` y revisión de enlaces,
inventario de 193 archivos y 52 tests forman parte del cierre local. La comprobación
de resultado pasa: 193 propietarios únicos, 603 enlaces locales válidos, contenido
de las 28 pasadas conservado y conjuntos exactos de fallos heredados.
