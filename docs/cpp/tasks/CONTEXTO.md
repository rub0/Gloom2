# Contrato común de la migración C++

Fecha: 7 de octubre de 2026. Base investigada: `5b96a91`, hito 110.
112 terminado: [medidas/contratos](../../../reports/cpp-performance-112/README.md),
113 terminado: [poses persistentes](../../../reports/cpp-performance-113/README.md),
114 terminado: [trabajos nativos](../../../reports/cpp-performance-114/README.md),
115 terminado: [partículas persistentes](../../../reports/cpp-performance-115/README.md),
116 terminado: [registro estable](../../../reports/cpp-performance-116/README.md),
117 terminado: [pipeline de assets con STL](../../../reports/cpp-performance-117/README.md),
198 archivos/54 CTest; siguiente 118, pendiente de encargo. STL autorizada solo en el alcance del 117.
Diagnóstico opcional OFF; fastgltf desbloqueado por esa excepción. Jolt sigue pendiente en 120/128.
Leer primero el inicio de [ESTADO_ACTUAL](../../ESTADO_ACTUAL.md),
[AGENTS](../../../AGENTS.md), [índice](README.md) y solo la ficha elegida.
No cargar todas las fichas ni el historial. Ponytail full activo: usar lo ya
existente y el mínimo cambio; aplicar la excepción STL de AGENTS exclusivamente
al alcance del 117. Fuera de ese alcance sigue la prohibición del usuario.
No construir una biblioteca estándar completa.

## Alcance y decisiones firmes

- Renderer Diligent/Vulkan actual, Ryzen 7 3700X/GTX 1070 y contrato
  [H06](../../art/hound/H06-contrato-presupuesto.md) conservados. NoGraphicsAPI es
  referencia de C++, no dependencia ni cambio de arquitectura gráfica.
- Propietarios primero: `Array`/almacenamiento estable posee; `Span` solo observa.
  `Span`, `ByteSpan` y `GpuRange` siempre por valor. IDs pequeños como `EntityId`
  (index+generation, 8 bytes) por valor; estructuras grandes por referencia.
  Un upload por valor que se mueve a una cola es transferencia, no copia gratuita
  ni motivo para sustituirlo por una referencia que caduque.
- `auto` deduce el tipo al compilar: retirarlo cumple la regla de estilo, no
  demuestra ahorro en ejecución. Boost/EASTL no se incorporan por una supuesta
  superioridad global. SIMD, lock-free, SoA o arenas solo si un coste real lo exige.
- Tipos propios compartidos ya existen en `core/types.hpp`; Span, Array y clock
  del 110 se reutilizan. FixedFunction aparece con jobs en 114. El 117 conserva
  texto/resultados/archivos STL existentes por autorización expresa; no añade
  sustitutos propios para esa tarea. Los futuros consumidores fuera de su
  alcance se revisan según sus reglas, sin clonar APIs completas por anticipado.
- Corregir usos prohibidos del módulo migrado y todos los callers afectados.
  Los restos heredados de otros módulos siguen registrados hasta 128: no declarar
  cumplimiento global antes ni introducir nuevas infracciones como puente.
- Todas las fuentes C++ propias, apps, utilidades y tests están dentro del alcance.
  Bibliotecas externas conservan su código interno; ello no autoriza uso directo
  de STL/herencia prohibidos en código Gloom fuera de la excepción STL expresa
  del 117. Cada conflicto debe resolverse, no esconderse en vendor ni darse por exceptuado.
- Datos externos (archivo, red, HTTP, credenciales) y fallos de inicialización
  conservan resultados y errores útiles. Las precondiciones de programación
  usan asserts sin abort manual. No quitar validaciones de seguridad ni añadir
  comprobaciones de OOM. No duplicar la validación de Vulkan.
- Las fuentes/exportaciones artísticas v16/v17, referencias visuales, formato de
  assets y protocolo 22 se conservan. Hound H08 no se ejecuta con este plan.

## Propiedad que cada ficha debe concretar al implementar

Las reglas de AGENTS se aplican en cada módulo; esta tabla asigna su cierre:

| Regla/frente | Responsable y comprobación |
| --- | --- |
| STL según AGENTS y excepción 117; sin mapas/algorithms/shared ownership propios | Hito de cada módulo; 128 revisa toda la cobertura y el alcance de la excepción, incluido tests/utilidades. |
| Tipos enteros propios; Span/ByteSpan/GpuRange por valor; structs grandes por referencia | Base 112, revisión de firmas en cada cambio y cierre 128. |
| auto ordinario, variables triviales, lambdas/templates complejos y wrappers triviales | Cada módulo elimina usos afectados; 127/128 revisan residuos, también structs de un solo campo. |
| Sin virtual/herencia/PIMPL/excepciones/RTTI | Cada dueño de módulo, conflicto Jolt en 112/120, flags efectivos de todos los targets en 128. |
| Precondiciones/asserts, inicialización y errores de datos externos; sin OOM checks | Cada módulo y sus tests negativos; rollback de arranque/cierre en 127. |
| Reservas, salidas persistentes y ausencia de temporales medidos | 113–116/118/119/121/122/126/127; capacidad y intervalo fijados en 112. |
| Defaults útiles/designated initializers, loops simples, 160 columnas | Cada cambio y formato; revisión semántica global 128. |
| Destrucción GPU tras último uso, drain antes del dispositivo; create/destroy juntos | Contrato central en 119 y todas las aplicaciones/pruebas en 127/130. |
| Shader VS/PS juntos, fuentes distintas separadas y header compartido correspondiente | 119 conserva layout/contenido y prueba imágenes; 128 revisa cobertura. |
| Revisión de rendimiento, pruebas, informe, estado y commit local de cada hito | Cada ficha; consolidación funcional/visual/medidas en 130. |

| Datos | Dueño y condición de reutilización |
| --- | --- |
| Vistas de frame/initializer list | Caducan al reconstruir/destruir dueño; initializer list solo durante la llamada. Nunca retenerlas en async. |
| Poses actual/anterior | Dueño por actor+rig generation+skin binding; anterior inmutable hasta último consumidor CPU. GPU usa su almacenamiento con fence separado. |
| Grupo/job/contexto | Dueño vive hasta wait/drain; grupo no reutilizable con pendientes; publicación entre hilos sincronizada. |
| Componentes | Punteros estables, o todos los caches sustituidos por handles antes de mover almacenamiento. |
| Solicitud/load/upload | Dueño cubre cola, job y publicación; cancel invalida generación y mantiene contexto hasta finalizar. |
| Clip/voice | Samples viven mientras una voz los lee; cierre de output y voces precede a retirar dueño. |
| Snapshot/paquete/UI | Memoria vive hasta último consumidor; dato diferido en propiedad estable, no Span de temporal. |
| Recurso GPU | Todo uso grabado/enviado protegido; liberar tras valor de timeline/fence del último uso; drain antes de destruir dispositivo. |

Inicialización/reserva/cambio de escena pueden asignar: las fichas que dicen cero
asignaciones lo exigen solo dentro de capacidad, tras preparar y en el intervalo
declarado. Capacidad no autoriza pérdida silenciosa ni límites funcionales nuevos.
Array del 110 construye la capacidad y mueve por asignación; comprobar tipo/vida
útil antes de usarlo. `qsort` solo para registros triviales o índices, nunca dueños.

## Conflicto conocido: callbacks Jolt

`src/backends/jolt_world.cpp` define BroadPhaseLayers, ObjectLayerPairs,
ObjectVsBroadPhase, ContactCollector y CharacterContactCollector heredados de Jolt.
Los headers instalados de Jolt confirman `SetContactListener(ContactListener*)`
y `CharacterVirtual::SetListener(CharacterContactListener*)`. No es una hipótesis
de estilo: el comportamiento actual depende de esos callbacks.

El probe del 112 confirma filtros incorporados y un caso simple con dos workers,
pero no demuestra equivalencia completa de listeners. fastgltf también exige STL
en su API de parsing; el usuario la autorizó para el 117 el 7 de octubre de 2026.
Ambas fronteras están documentadas con versión/API/prueba; solo Jolt sigue bloqueado.
112 debía demostrar una alternativa sin herencia propia que conserve filtros,
eventos entered/stayed/exited y multithreading. Si no la hay, el cierre literal
de 120/128 requiere una decisión expresa del usuario sobre esa incompatibilidad.
El plan no concede la excepción, no cambia Jolt ni elimina eventos. El bloqueo
de física no impide investigar o ejecutar frentes independientes; se registra
con nombres de API, versión y prueba. Una excepción autorizada se enumerará en
el informe final; nunca se presentará como cumplimiento literal de la regla.

## Protocolo de medida y aceptación

1. Guardar commit, configuración, hardware/driver, presentación/audio, flags y
   contenido de cada serie. Release para rendimiento; Debug para precondiciones.
   No medir durante builds, otras pruebas o procesos gráficos de la tarea.
2. Factory existente: `--vertical-slice-performance-1080p`, presentación inmediata,
   1920×1080 nativo, 120 frames de calentamiento y 360 medidos. Tres pasadas antes
   y tres después, seriales, mismo commit base para cada comparación local.
   Guardar media/p50/p95/p99/máximo, no solo FPS o promedio agregado.
3. Sobre Hound: herramienta `tools/art/measure_hound_budget.py --v17`, preparación
   existente de `.cache/hound-production-v17`, dos pasadas con audio nulo y dos
   con dispositivo real. Se fuerza LOD0 y las mismas máscaras: 53 skinned visibles,
   TPS 127, weapon 31, compresión BC5/BC7 y 360 samples por pasada. No regenerar
   arte para preparar el probe. No certifica ocho jugadores humanos.
   `gloom.hound_runtime` prueba el probe v16: no sustituye la serie de presupuesto
   v17. Mantener también su contrato de paletas compartidas (`peak_maps=9`).
4. Hound conserva p99≤4 ms, máximo≤5 ms, 0/360 >5 ms, residency≤384 MiB y cero
   missing meshes/textures, evictions y budget_limited_frames medidos según H06.
   El presupuesto residente configurable de runtime sigue siendo 512 MiB.
5. Comparar la mediana de cada etapa entre pasadas. Estimar ruido relativo como
   el mayor `(máximo-mínimo)/mediana` de ambas series; mejora/regresión solo se
   atribuye si supera ese ruido y la resolución del contador. Si queda dudosa,
   repetir cinco pasadas seriales por versión y mostrar datos. Si sigue solapando,
   no atribuir mejora ni cerrar un cambio de rendimiento por una ganancia incierta.
   Un cambio exigido por reglas puede conservarse por cumplimiento, declarando
   ausencia de mejora; una regresión atribuible se corrige antes de cerrar.
6. Para cero asignaciones de una ruta: 1.000 repeticiones calentadas con entradas
   idénticas dentro de capacidad, contadores en todos los threads de esa ruta y
   control de crecimiento positivo. Informar ámbito new/new[], malloc/realloc,
   bytes, peaks y tipos externos aparte. Log/archivo fuera del intervalo medido.
7. Visuales: conservar comparador, referencias y umbrales existentes. El comparador
   actual usa miniaturas 160×90: media≤4/255, changed≤1,5 % (diferencia RGB media
   >32/255), worst tile≤12/255. Mantener también capturas completas y revisarlas:
   pasar miniaturas no demuestra igualdad de todos los píxeles.

## Pruebas, informes y cierre

Usar los presets `windows-release` y `windows-debug`, comandos con prefijo `rtk`
según RTK.md. Formato: target `gloom_format_check`; pruebas por nombres CTest de
la ficha. Toda edición que cambie tipos públicos compila todos sus consumidores.
Al migrar un módulo, desactivar excepciones/RTTI en sus fuentes y pruebas en ambas
configuraciones y verificar las opciones efectivas, como transición del 110;
128 elimina las opciones heredadas de todos los targets. Si un header de otro
módulo impide compilar, adaptar ese consumidor/contrato en el mismo hito y registrar
el alcance: no quitar pruebas ni fingir que el flag ya está aplicado.
Las suites completas se ejecutan en 112/128/130 y cuando el impacto transversal,
un fallo o cambios nuevos lo justifique. No repetirlas solo por rutina tras checks
correctos sin cambios. Pruebas de estado inválido por assert solo en Debug/proceso
hijo; no convertir todos los tests en death tests ni producir aborts manuales.

Comandos existentes, desde la raíz de Gloom (cambiar únicamente el nombre de
prueba para la ficha; el benchmark no se ejecuta en paralelo):

```powershell
rtk proxy D:/Dev/CMake/bin/cmake.exe --build --preset windows-release --parallel 6
rtk proxy D:/Dev/CMake/bin/cmake.exe --build --preset windows-debug --parallel 6
rtk proxy D:/Dev/CMake/bin/cmake.exe --build --preset windows-release --target gloom_format_check
rtk proxy D:/Dev/CMake/bin/ctest.exe --preset windows-release -R '^gloom\.storage$' --output-on-failure
rtk proxy D:/Dev/CMake/bin/ctest.exe --preset windows-debug -R '^gloom\.storage$' --output-on-failure
rtk proxy D:/Dev/CMake/bin/ctest.exe --preset windows-release --output-on-failure
rtk proxy D:/Dev/CMake/bin/ctest.exe --preset windows-debug --output-on-failure
rtk proxy build/windows-vs/Release/gloom.exe --vertical-slice-performance-1080p --present=immediate
rtk proxy python tools/art/prepare_hound_budget.py --v17 --size 2048
rtk proxy python tools/art/measure_hound_budget.py --v17
```

La preparación escribe solo diagnóstico/cooked en cache/build y comprueba fuentes;
los recursos usados en una serie se fijan al comienzo. Después de cambiar cooker o
formato, preparar ambas versiones con los mismos inputs y verificar equivalencia,
sin reutilizar ciegamente un cooked producido por otra versión. Revisar antes los
comandos/paths si el entorno o los targets han cambiado.

Estado heredado 110: Release 46/50, Debug 47/50. En ambos fallan ui_visual_review,
factory_visual_review y character_visual_review; Release además animation_network
(GNS 25). 112 debe reproducir/clasificar su estado actual; los totales históricos
no justifican un nuevo fallo. No actualizar referencias o desactivar tests para
pasar. Un fallo diferente de los heredados debe resolverse o declararse bloqueo.

Cada hito entrega `reports/cpp-performance-N/README.md` con alcance, antes/después,
pruebas, límites y pendientes; solo Markdown se versiona bajo reports. Actualizar
esta ficha/índice y ESTADO_ACTUAL; revisar performance y contratos de parámetros.
Crear commit local `hito N: resultado`, incluyendo solo sus cambios, verificar
commit/workspace y entregar hash. Ningún hito técnico cerrado sin commit; no push.

## Fuentes y evidencia

Código local investigado en `5b96a91`, [inventario](INVENTARIO.md) y
[medidas 110](../../../reports/cpp-performance-110/medidas.md).
Referencia NoGraphicsAPI fijada a `b6d49590a7b8279fb726f8c2d199f17e8c74e021`:
[pautas C++](https://github.com/sebbbi/NoGraphicsAPI/blob/b6d49590a7b8279fb726f8c2d199f17e8c74e021/AGENTS.md)
y [README](https://github.com/sebbbi/NoGraphicsAPI/blob/b6d49590a7b8279fb726f8c2d199f17e8c74e021/README.md).
Se toma su preferencia por datos simples y propiedad clara, sin adoptar API gráfica
ni su hardware objetivo. Para la incompatibilidad externa, las APIs oficiales
[PhysicsSystem](https://github.com/jrouwe/JoltPhysics/blob/master/Jolt/Physics/PhysicsSystem.h)
y [CharacterVirtual](https://github.com/jrouwe/JoltPhysics/blob/master/Jolt/Physics/Character/CharacterVirtual.h)
apoyan lo observado; la decisión se verifica sobre la versión local instalada,
no se asume equivalencia de master con esa versión.
