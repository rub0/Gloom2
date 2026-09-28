# Hito 102 — rendimiento con ocho combatientes

28 de septiembre de 2026 · **Planificado, pendiente de ejecución**.
El usuario pide registrar este trabajo como un hito independiente de Gloom
y dejarlo fuera de la tarea Hound H06 por el momento. Esta entrega solo fija
el alcance y los criterios de aceptación; no es el cierre técnico del hito.

## Entrada y objetivo

Leer primero [ESTADO_ACTUAL.md](../../docs/ESTADO_ACTUAL.md) y AGENTS.md.
Reutilizar el benchmark del [hito 74](../performance-74/README.md) y las
[medidas H06 del hito 101](../hound-contract-101/results.md), revisando solo
los apartados relevantes y el código vigente al ejecutar el hito.

Objetivo confirmado: **200 FPS a 1920×1080 nativo, hasta ocho combatientes,
en Ryzen 7 3700X y GTX 1070**. El presupuesto del fotograma completo es 5 ms.
La referencia actual usa Factory con dos combatientes y presentación original:
144,17/144,19 FPS a 1080p. La cadencia sugiere una limitación de presentación,
pero no demuestra saturación de la GPU. Ocho copias estáticas del visor no
validan este escenario. Hound v16 aún no tiene texturas ni clips jugables finales.

## Trabajo cuando se encargue su ejecución

1. Aislar el límite aparente de ~144 FPS: separar trabajo CPU/GPU de las
   esperas de Draw/Present y registrar el modo efectivo de presentación,
   VSync y cualquier limitador. Conservar una referencia comparable de dos
   combatientes a 1080p; usar 720p solo como control, no como sustitución.
2. Extender el benchmark existente con ocho presentaciones animadas en Factory,
   posiciones/cámara/distancias repetibles y evidencia de cuántas son visibles.
   Registrar si son ocho cuerpos TPS o jugador local FPS más siete remotos.
   Ejercitar armas, sombras, movimiento y efectos de combate, HUD y audio.
   Reutilizar los sistemas actuales; no ampliar la autoridad/red para crear
   esta prueba de carga local. Separar las comprobaciones GPU de la fixture
   horneada y del clip diagnóstico Hound v16 de la prueba con clips jugables.
3. Medir Release tras cargar recursos y calentar: tiempos CPU de poses/bounds,
   GPU del intervalo completo, draws/pasadas y memoria/residencia. Informar
   media, p50/p95/p99 y máximo, junto con resolución interna/salida, calidad,
   assets/clips, cámara, número visible, duración y exclusiones. Repetir el caso
   para comprobar estabilidad; una consulta GPU final no sustituye una muestra.
4. Identificar el coste que impide alcanzar 5 ms, si lo hay. Aplicar solo ajustes
   justificados por el perfil dentro de este alcance y conservar comparaciones
   antes/después. Si exige cambios mayores, documentarlos como trabajo posterior.
   Devolver a H06 límites medidos de geometría/LODs, materiales y mapas, con
   margen para el resto del juego y límites explícitos del contenido ensayado.

## Límites y aceptación

- No ejecutar ahora: no cambiar motor, gameplay, herramientas ni assets con
  motivo de esta planificación. No crear una tarea automática ni hacer push.
- El alcance es presentación/carga y rendimiento de ocho combatientes. Una
  partida real de ocho clientes, ampliación de servidor/protocolo, matchmaking
  y pruebas de escala de red quedan fuera de este hito.
- La corrección de mapas de normales detectada en H06 es otro frente. No
  modificar la escultura v16 aprobada, iniciar H07 ni cambiar el objetivo de FPS.
- Entregar un caso reproducible con evidencia de animación GPU y visibilidad,
  medidas del fotograma completo con combate/HUD/audio y regresiones pertinentes.
  No aprobar el objetivo usando solo la media ni ocultando picos/exclusiones;
  indicar los incumplimientos de 5 ms y las condiciones de cada resultado.
- La investigación puede terminar con un informe de incumplimiento y siguiente
  acción concreta; eso no certifica los 200 FPS ni aprueba un presupuesto Hound.
  Solo actualizar H06 con límites sustentados por las medidas obtenidas.

## Registro de esta entrega documental

Hito registrado en [ROADMAP.md](../../docs/ROADMAP.md) y en el estado del
proyecto. Ficha, índice y contrato H06 enlazan la dependencia externa aplazada.
Comprobados enlaces locales, coherencia de alcance/estado y diff sin errores
de espacios. Solo Markdown; no se compila ni se repiten pruebas del motor.
La implementación permanece pendiente. Commit local de planificación,
identificable con `git log --oneline --grep='^hito 102:'`; sin push.
