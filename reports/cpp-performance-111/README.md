# Hito 111: investigación y plan formal de C++

Fecha: 3 de octubre de 2026. Base técnica: `5b96a91` (110).

Se formaliza el trabajo en [19 fichas, 112–130](../../docs/cpp/tasks/README.md),
con fuentes verificadas, orden, dependencias, contratos, pruebas CTest, métricas
y criterios de cierre. Se entrega [contexto común](../../docs/cpp/tasks/CONTEXTO.md)
y [inventario exhaustivo](../../docs/cpp/tasks/INVENTARIO.md): 188 archivos C++,
34.961 líneas y 50 pruebas existentes. Cada archivo tiene responsable de cierre.

## Hallazgos que determinan el plan

- Poses reconstruyen jerarquía y arrays y asignan SkinPose compartidos; conservar
  pose anterior y compartir hermanos de un skin exige dueño por actor/rig/enlace.
- Jobs asigna grupos/callable compartidos; nested wait, reutilización y el mutex
  del predicado no pueden perderse al usar buffers/FixedFunction y APIs nativas.
- Partículas ya reserva sus datos, pero devuelve un vector render nuevo cada vez.
- Registry tiene typeid/mapas/pools virtuales; Combatant guarda doce punteros.
  Migrar a un array que se mueve sin adaptar esos caches sería incorrecto.
- Loader/residencia y uploads son diferidos: eliminar shared_ptr/future necesita
  conservar dueño y generación hasta drenar trabajos y último uso GPU.
- La migración alcanza también cold-path cooker/VFS, física/audio, red/gameplay,
  directory/storage/auth/HTTP, UI/runtime, aplicaciones y tests; no solo hot loops.
- Jolt exige callbacks por interfaces virtuales en la integración actual. No hay
  excepción autorizada: 112 debe probar una alternativa equivalente o registrar
  incompatibilidad explícita antes de 120/128. No cambiar motor ni omitir eventos.
- El renderer conserva todos sus pases; optimización GPU se elige tras perfil
  en 129, no por imitación de la arquitectura de NoGraphicsAPI.

Los beneficios medidos siguen siendo los del [110](../cpp-performance-110/README.md).
No se han ejecutado nuevos benchmarks, builds ni suites en esta tarea documental.
Las búsquedas lexicales muestran señales, no costes ni pruebas de cumplimiento.
No se inicia el 112 ni se modifican fuentes/runtime/render/arte/dependencias.

## Investigación y verificación del hito

Se revisaron declaraciones, implementaciones, callers y pruebas de los frentes
anteriores, CMake/flags/presets, herramientas del sobre H06 y los headers Jolt
instalados. Referencia NoGraphicsAPI fijada a commit
`b6d49590a7b8279fb726f8c2d199f17e8c74e021`, solo como C++:
[pautas](https://github.com/sebbbi/NoGraphicsAPI/blob/b6d49590a7b8279fb726f8c2d199f17e8c74e021/AGENTS.md).

Comprobaciones para este cambio documental: cobertura única de los 188 archivos,
19 IDs consecutivos y dependencias anteriores, todas las fuentes enlazadas
existentes, nombres focalizados pertenecientes a los 50 CTest reales, enlaces
locales válidos y diff sin whitespace. El resultado y commit se verifican antes
de entregar. No se da por cerrado un hito técnico pendiente por tener ficha.

Resultado de la comprobación documental: **19/19 fichas**, cobertura **188/188**
con un dueño por archivo, nombres de los **50 CTest** comprobados y **580 enlaces
locales válidos** en las fichas, índice, contexto, inventario e informe. Cada ficha
incluye su lista completa de archivos responsables para ejecutarla con contexto reducido.

Siguiente encargo ejecutable: [112, medidas y contratos](../../docs/cpp/tasks/112-base-y-contratos.md).
El usuario puede ejecutar cada ficha por separado con su encargo copiable.
Este hito termina la investigación/formalización; todos los 112–130 quedan no iniciados.
