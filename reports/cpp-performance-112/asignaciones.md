# Asignaciones, tamaños y capacidades

GLOOM_ALLOCATION_PROFILE es opcional, Windows y OFF por defecto. Solo el exe
gloom de diagnóstico enlaza el reemplazo de new/new[]; no se instala globalmente
en todos los tests ni en DLLs. Fases TLS para todos los workers, contadores
Interlocked y tablas fijas protegidas por SRWLOCK sin asignar memoria. Reiniciar
con ventana cerrada y trabajo drenado; habilitar después de 120 frames listos,
cerrar cada frame antes de guardar muestras, imprimir fuera de la ventana.

## Cobertura y límites

- Cuenta llamadas a **new/new[] ordinarios resueltos en el ejecutable** y bytes
  solicitados; incluye código upstream enlazado estáticamente en la fase renderer.
  No son exclusivamente asignaciones de código Gloom.
- No intercepta aligned new, CRT dentro de DLLs, malloc/realloc directos, memoria
  comprometida/residente ni heaps GPU. No mide bytes vivos o liberaciones. El
  mayor request no es pico de memoria de la fase. malloc usado por el propio
  reemplazo es el mecanismo de new contabilizado, no otro contador global.
- Los malloc/realloc propios directos anteriores están en winhttp_identity
  (dirección/response); esa ruta no se ejecuta en Factory/Hound. No afirmar cero
  asignaciones HTTP ni cero heap global. Memoria de proceso/GPU en medidas.md.
- Atribuye a la fase más cercana del thread. Un worker siempre entra jobs; su
  trabajo de preparación de assets aparece jobs, sin añadir etiquetas de origen
  a WorkItem. Snapshots contiene otras fases anidadas que se atribuyen a su tag.
- Tiempos de scopes son **inclusivos**, por thread y sumados; la anidación de la
  misma fase no duplica scope, distinta fase sí se solapa. Renderer incluye
  espera/Present. No sumar estas duraciones para inventar tiempo de frame ni CPU
  exclusivo. Las etapas reales comparables están en medidas.md.
- Capacidades/tamaños cubren todo el proceso, incluido arranque, y metadata no se
  reinicia con las llamadas. Pico de count y capacity son máximos independientes.
  Cada tabla admite 128 nombres; el escenario usa menos y no alcanza el límite.
- OFF elimina macros, overload diagnóstico de schedule, profiler y allocator del
  juego. La prueba standalone del contador sigue existiendo y tiene su propio
  allocator; no es evidencia de que el juego normal lo enlace.

Control ejecutable: ventana cerrada cuenta cero; cuatro workers ×1.000 iteraciones
×dos requests cuentan **8.000 llamadas, 200.000 bytes, máximo 33**, atribuidos a
jobs. Array calentado 32 elementos ×1.000 resize cuenta cero; reserve(64) detecta
una llamada de 256 bytes. Scope terminado restaura fase y finish repetido no la
altera. Metadata se registra desde los workers y se imprime fuera de la ventana.
Release y Debug pasan, con timeout que detecta pérdida de progreso.

## Requests por frame: dos escenarios representativos

Se divide cada conteo/bytes entre 360; no se redondea a una supuesta capacidad fija.
Factory-1 y Hound-audio-1 de instrumented-final. Fase assets cero corresponde a
update/request calentados, no a carga inicial.

| Fase | Factory llamadas/frame | Factory bytes/frame | Hound llamadas/frame | Hound bytes/frame |
| --- | ---: | ---: | ---: | ---: |
| other | 42.186 | 2447.406 | 42.486 | 2459.272 |
| poses | 210.000 | 93728.000 | 710.000 | 419528.000 |
| bounds | 0.000 | 0.000 | 0.000 | 0.000 |
| jobs | 3.000 | 344.000 | 4.000 | 416.000 |
| particles | 1.000 | 22972.200 | 1.000 | 19353.000 |
| entities | 0.000 | 0.000 | 0.000 | 0.000 |
| snapshots | 10.000 | 3856.000 | 40.000 | 18624.000 |
| ui | 0.000 | 0.000 | 3.000 | 112.000 |
| assets | 0.000 | 0.000 | 0.000 | 0.000 |
| renderer | 38.000 | 3648.000 | 43.000 | 3865.600 |
| lighting | 0.000 | 0.000 | 0.000 | 0.000 |
| visibility | 0.000 | 0.000 | 0.000 | 0.000 |

Prioridades confirmadas: poses **210/710 requests por frame**, snapshots **10/40**;
no se elimina ni se reduce ningún actor/paleta para medir. El cero local en
lighting/visibility excluye jobs anidados; visibilidad todavía crea un grupo
y callable. Entities cero es lookup estable, no spawn/despawn, construcción o
destrucción. UI cero de Factory significa que ese escenario no dibuja GameUi;
Hound sí mide su HUD. Partículas todavía asigna su vector de salida.

## Las siete pasadas finales de diagnóstico

Todos los valores son por **360 frames**, ms de scope inclusivo. No descartar picos
ni extrapolar estos conteos a toda la stdlib o al heap del proceso.

### factory-1

| Fase | Requests | Bytes | Mayor request | Scopes | Tiempo ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| other | 15187 | 881066 | 8231 | 0 | 0.000000 |
| poses | 75600 | 33742080 | 2880 | 10800 | 43.349900 |
| bounds | 0 | 0 | 0 | 720 | 1.177200 |
| jobs | 1080 | 123840 | 200 | 2520 | 6.108800 |
| particles | 360 | 8269992 | 25095 | 720 | 2.883800 |
| entities | 0 | 0 | 0 | 10800 | 1.037000 |
| snapshots | 3600 | 1388160 | 1800 | 360 | 64.815600 |
| ui | 0 | 0 | 0 | 0 | 0.000000 |
| assets | 0 | 0 | 0 | 4680 | 0.311000 |
| renderer | 13680 | 1313280 | 336 | 1080 | 729.205200 |
| lighting | 0 | 0 | 0 | 360 | 6.561300 |
| visibility | 0 | 0 | 0 | 360 | 10.015900 |

### factory-2

| Fase | Requests | Bytes | Mayor request | Scopes | Tiempo ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| other | 15187 | 881066 | 8231 | 0 | 0.000000 |
| poses | 75600 | 33742080 | 2880 | 10800 | 44.341900 |
| bounds | 0 | 0 | 0 | 720 | 1.209300 |
| jobs | 1080 | 123840 | 200 | 2520 | 6.029600 |
| particles | 360 | 8269992 | 25095 | 720 | 2.971100 |
| entities | 0 | 0 | 0 | 10800 | 1.017400 |
| snapshots | 3600 | 1388160 | 1800 | 360 | 66.419800 |
| ui | 0 | 0 | 0 | 0 | 0.000000 |
| assets | 0 | 0 | 0 | 4680 | 0.431400 |
| renderer | 13680 | 1313280 | 336 | 1080 | 794.389200 |
| lighting | 0 | 0 | 0 | 360 | 6.669900 |
| visibility | 0 | 0 | 0 | 360 | 9.810100 |

### factory-3

| Fase | Requests | Bytes | Mayor request | Scopes | Tiempo ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| other | 15187 | 881066 | 8231 | 0 | 0.000000 |
| poses | 75600 | 33742080 | 2880 | 10800 | 44.089400 |
| bounds | 0 | 0 | 0 | 720 | 1.196100 |
| jobs | 1080 | 123840 | 200 | 2520 | 6.114200 |
| particles | 360 | 8269992 | 25095 | 720 | 2.895000 |
| entities | 0 | 0 | 0 | 10800 | 1.003500 |
| snapshots | 3600 | 1388160 | 1800 | 360 | 66.034700 |
| ui | 0 | 0 | 0 | 0 | 0.000000 |
| assets | 0 | 0 | 0 | 4680 | 0.309800 |
| renderer | 13680 | 1313280 | 336 | 1080 | 728.428500 |
| lighting | 0 | 0 | 0 | 360 | 6.711800 |
| visibility | 0 | 0 | 0 | 360 | 10.118400 |

### hound-null-1

| Fase | Requests | Bytes | Mayor request | Scopes | Tiempo ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| other | 15295 | 885338 | 8231 | 0 | 0.000000 |
| poses | 255600 | 151030080 | 3520 | 41040 | 199.886100 |
| bounds | 0 | 0 | 0 | 19080 | 15.639900 |
| jobs | 1440 | 149760 | 200 | 3237 | 8.862400 |
| particles | 360 | 6967080 | 21615 | 720 | 2.321600 |
| entities | 0 | 0 | 0 | 10872 | 1.099100 |
| snapshots | 14400 | 6704640 | 2200 | 360 | 258.175200 |
| ui | 1080 | 40320 | 48 | 360 | 13.824300 |
| assets | 0 | 0 | 0 | 4680 | 0.297600 |
| renderer | 15480 | 1391616 | 336 | 1080 | 849.149100 |
| lighting | 0 | 0 | 0 | 360 | 6.627900 |
| visibility | 0 | 0 | 0 | 360 | 14.695000 |

### hound-null-2

| Fase | Requests | Bytes | Mayor request | Scopes | Tiempo ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| other | 15295 | 885338 | 8231 | 0 | 0.000000 |
| poses | 255600 | 151030080 | 3520 | 41040 | 230.687700 |
| bounds | 0 | 0 | 0 | 19080 | 15.693700 |
| jobs | 1440 | 149760 | 200 | 3233 | 6.994600 |
| particles | 360 | 6967080 | 21615 | 720 | 2.227500 |
| entities | 0 | 0 | 0 | 10872 | 1.007000 |
| snapshots | 14400 | 6704640 | 2200 | 360 | 286.832600 |
| ui | 1080 | 40320 | 48 | 360 | 11.611600 |
| assets | 0 | 0 | 0 | 4680 | 0.379400 |
| renderer | 15480 | 1391616 | 336 | 1080 | 821.013700 |
| lighting | 0 | 0 | 0 | 360 | 6.230500 |
| visibility | 0 | 0 | 0 | 360 | 12.050400 |

### hound-audio-1

| Fase | Requests | Bytes | Mayor request | Scopes | Tiempo ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| other | 15295 | 885338 | 8231 | 0 | 0.000000 |
| poses | 255600 | 151030080 | 3520 | 41040 | 187.952800 |
| bounds | 0 | 0 | 0 | 19080 | 15.683300 |
| jobs | 1440 | 149760 | 200 | 3237 | 5.957600 |
| particles | 360 | 6967080 | 21615 | 720 | 2.239200 |
| entities | 0 | 0 | 0 | 10872 | 1.017300 |
| snapshots | 14400 | 6704640 | 2200 | 360 | 240.672100 |
| ui | 1080 | 40320 | 48 | 360 | 9.948100 |
| assets | 0 | 0 | 0 | 4680 | 0.292700 |
| renderer | 15480 | 1391616 | 336 | 1080 | 876.816000 |
| lighting | 0 | 0 | 0 | 360 | 6.110800 |
| visibility | 0 | 0 | 0 | 360 | 10.979700 |

### hound-audio-2

| Fase | Requests | Bytes | Mayor request | Scopes | Tiempo ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| other | 15295 | 885338 | 8231 | 0 | 0.000000 |
| poses | 255600 | 151030080 | 3520 | 41040 | 190.128800 |
| bounds | 0 | 0 | 0 | 19080 | 15.835600 |
| jobs | 1440 | 149760 | 200 | 3237 | 6.088600 |
| particles | 360 | 6967080 | 21615 | 720 | 2.195100 |
| entities | 0 | 0 | 0 | 10872 | 0.956400 |
| snapshots | 14400 | 6704640 | 2200 | 360 | 243.498300 |
| ui | 1080 | 40320 | 48 | 360 | 10.156800 |
| assets | 0 | 0 | 0 | 4680 | 0.286200 |
| renderer | 15480 | 1391616 | 336 | 1080 | 869.585000 |
| lighting | 0 | 0 | 0 | 360 | 6.134000 |
| visibility | 0 | 0 | 0 | 360 | 11.269500 |

## Capturas reales del runtime y wrapper

| Callable/estado | Bytes | Alineación | Fuente |
| --- | ---: | ---: | --- |
| AsyncAssetLoader::request lambda | 152 | 8 | Game ON |
| AssetResidencyCoordinator::update preparación | 304 | 8 | Game ON |
| VisibilitySystem::CullJob | 72 | 8 | Game ON |
| main physics_to_render callable | 16 | 8 | Game ON |
| main snapshot callable | 32 | 8 | Game ON |
| parallel_for wrapper (owner + begin/end) | 32 | 8 | gloom.jobs ON, rango 10.000/grain 97 |
| TaskGroup::State | 184 | 8 | Game ON y gloom.jobs |
| JobSystem::WorkItem | 80 | 8 | Game ON y gloom.jobs |

El fixture jobs también registra nested/reused/throw callables: 16/8, 8/8, 1/1 y 8/8; callable original del fixture parallel_for 8/8. No confundir WorkItem con capacidad inline de la futura FixedFunction.

## Componentes reales construidos por el runtime

| Tipo | Bytes | Alineación |
| --- | ---: | ---: |
| gloom::gameplay::TransformComponent | 24 | 4 |
| gloom::gameplay::CharacterPhysicsComponent | 56 | 8 |
| gloom::gameplay::HealthComponent | 12 | 4 |
| gloom::gameplay::ShieldComponent | 4 | 4 |
| gloom::gameplay::CharacterMovementComponent | 20 | 4 |
| gloom::gameplay::WeaponComponent | 528 | 8 |
| gloom::gameplay::AbilityComponent | 28 | 4 |
| gloom::gameplay::CharacterLoadoutComponent | 3 | 1 |
| gloom::gameplay::AuthorityComponent | 24 | 8 |
| gloom::gameplay::ReplicationComponent | 24 | 8 |
| gloom::gameplay::CharacterPresentationComponent | 4 | 4 |
| gloom::gameplay::ScoreComponent | 8 | 4 |
| BasicBodyComponent<0,0> | 32 | 8 |
| gloom::gameplay::DamageVolumeComponent | 4 | 4 |
| BasicBodyComponent<0,1> | 32 | 8 |

15 tipos actuales; Slot 8 bytes. Estos son los tipos del juego observado, no una garantía del máximo de componentes ni los tipos sintéticos de tests. Inventario exhaustivo de archivos en docs/cpp/tasks/INVENTARIO.md.

## Picos de almacenamiento

Cada fila proviene del proceso completo del caso indicado, no solo steady state.

| Caso | Owner | Pico count | Pico capacity | Bytes por elemento |
| --- | --- | ---: | ---: | ---: |
| factory-1 | entity_slots | 4 | 4 | 8 |
| factory-1 | render_instances | 18 | 18 | 232 |
| factory-1 | complete_instances | 249 | 530 | 232 |
| factory-1 | previous_animated_instances | 4 | 64 | 232 |
| factory-1 | visible_instances | 99 | 138 | 232 |
| factory-1 | visibility_indices | 249 | 282 | 24 |
| factory-1 | particles | 108 | 2048 | 56 |
| factory-1 | emitters | 20 | 64 | 80 |
| factory-2 | entity_slots | 4 | 4 | 8 |
| factory-2 | render_instances | 18 | 18 | 232 |
| factory-2 | complete_instances | 249 | 530 | 232 |
| factory-2 | previous_animated_instances | 4 | 64 | 232 |
| factory-2 | visible_instances | 99 | 138 | 232 |
| factory-2 | visibility_indices | 249 | 282 | 24 |
| factory-2 | particles | 108 | 2048 | 56 |
| factory-2 | emitters | 20 | 64 | 80 |
| factory-3 | entity_slots | 4 | 4 | 8 |
| factory-3 | render_instances | 18 | 18 | 232 |
| factory-3 | complete_instances | 249 | 530 | 232 |
| factory-3 | previous_animated_instances | 4 | 64 | 232 |
| factory-3 | visible_instances | 99 | 138 | 232 |
| factory-3 | visibility_indices | 249 | 282 | 24 |
| factory-3 | particles | 108 | 2048 | 56 |
| factory-3 | emitters | 20 | 64 | 80 |
| hound-null-1 | entity_slots | 4 | 4 | 8 |
| hound-null-1 | render_instances | 18 | 18 | 232 |
| hound-null-1 | complete_instances | 291 | 530 | 232 |
| hound-null-1 | previous_animated_instances | 61 | 64 | 232 |
| hound-null-1 | visible_instances | 161 | 268 | 232 |
| hound-null-1 | visibility_indices | 291 | 548 | 24 |
| hound-null-1 | particles | 93 | 2048 | 56 |
| hound-null-1 | emitters | 18 | 64 | 80 |
| hound-null-2 | entity_slots | 4 | 4 | 8 |
| hound-null-2 | render_instances | 18 | 18 | 232 |
| hound-null-2 | complete_instances | 291 | 530 | 232 |
| hound-null-2 | previous_animated_instances | 61 | 64 | 232 |
| hound-null-2 | visible_instances | 161 | 268 | 232 |
| hound-null-2 | visibility_indices | 291 | 548 | 24 |
| hound-null-2 | particles | 93 | 2048 | 56 |
| hound-null-2 | emitters | 18 | 64 | 80 |
| hound-audio-1 | entity_slots | 4 | 4 | 8 |
| hound-audio-1 | render_instances | 18 | 18 | 232 |
| hound-audio-1 | complete_instances | 291 | 530 | 232 |
| hound-audio-1 | previous_animated_instances | 61 | 64 | 232 |
| hound-audio-1 | visible_instances | 161 | 268 | 232 |
| hound-audio-1 | visibility_indices | 291 | 548 | 24 |
| hound-audio-1 | particles | 93 | 2048 | 56 |
| hound-audio-1 | emitters | 18 | 64 | 80 |
| hound-audio-2 | entity_slots | 4 | 4 | 8 |
| hound-audio-2 | render_instances | 18 | 18 | 232 |
| hound-audio-2 | complete_instances | 291 | 530 | 232 |
| hound-audio-2 | previous_animated_instances | 61 | 64 | 232 |
| hound-audio-2 | visible_instances | 161 | 268 | 232 |
| hound-audio-2 | visibility_indices | 291 | 548 | 24 |
| hound-audio-2 | particles | 93 | 2048 | 56 |
| hound-audio-2 | emitters | 18 | 64 | 80 |

Decisiones y límites de FixedFunction, contexto propietario, cola y entidades en [contratos](contratos.md).
