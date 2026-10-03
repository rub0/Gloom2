# Hito 112: base reproducible, costes y contratos

3 de octubre de 2026. Entrada `517b16c` (111), enviada a origin/main por encargo
del usuario antes de iniciar este hito. El 112 mide y concreta la migración;
renderer, contenido, reglas y dependencias se conservan. Ponytail full.

## Resultado

- Diagnóstico Windows opcional **OFF por defecto**, fases TLS y contadores
  concurrentes de requests/bytes/tamaño máximo, tiempos inclusivos, layouts y
  capacidades. Fuera del juego normal; no se escribe una biblioteca estándar.
- Tres Factory + cuatro Hound por serie, **28 pasadas conservadas**: base de
  entrada, diagnóstico inicial/final y build normal final. Hound null/SDL, v17
  LOD0, 1080p nativo y los mismos conteos/máscaras/paletas.
- Poses: **210 requests/93.728 bytes por frame Factory** y
  **710/419.528 Hound**; snapshots **10/3.856 y 40/18.624**. Eso confirma el orden
  de trabajo del 113. No representa bytes vivos ni todo el heap del proceso.
- Capturas: CullJob **72/8**, wrapper parallel_for **32/8**, loader **152/8** y
  preparación **304/8** (bytes/alineación). Para 114: candidato inline 80/8,
  payloads de carga en contextos estables; saturación de cola con progreso garantizado.
- Contratos explícitos para llamadas síncronas, colas, snapshots, cancelación,
  punteros a componentes y reutilización CPU/GPU; diseños mínimos de sincronización,
  texto/resultado/archivos solo cuando su primer consumidor los necesite.
- Probe sin STL/herencia propias, excepciones ni RTTI en su fuente: filtros Jolt
  incorporados y caso simple entered/stayed/exited con pool de dos workers,
  cabecera C Diligent, construcción fastgltf e init/interfaz flat/shutdown GNS.

[Medidas y comandos](medidas.md) · [Asignaciones/layout/capacidad](asignaciones.md) ·
[Propiedad y diseños](contratos.md) · [Compatibilidad y bloqueos](compatibilidad.md).

## Límites y trabajo pendiente

La mediana Factory normal es **2,549→2,407 ms**, pero el ruido observado es
**11,34 %** frente a ese cambio de **5,57 %**: **no se atribuye mejora de FPS**.
La base y una pasada normal final Hound incumplen p99/máximo de H06. Las 28
distribuciones conservan los picos; H06 no queda recertificado por este hito.
No se relajan límites ni se cambian referencias para pasar.

**Decisión externa pendiente:** los listeners Jolt requieren adaptadores virtuales
y el parser fastgltf exige tipos STL en su frontera. Se solicitó excepción mínima
al usuario; sin respuesta no se aplica. 117/120 no pueden cerrar literalmente
esas partes y 128 no puede certificar cumplimiento global. La investigación del
112 sí admite dejar ese bloqueo concreto documentado; 113 no depende de resolverlo.
No se sustituyen bibliotecas ni se eliminan eventos para eludir la regla.

Legacy conserva STL/excepciones/RTTI en módulos aún no migrados. Las fuentes
nuevas del 112 y los tests nuevos compilan con flags sin excepciones/RTTI.
La prueba de polling no equivale a todos los callbacks del juego; detalles y
fuentes primarias en compatibilidad.md. No se afirma cero asignaciones globales.

## Validación y cierre

Builds completos Release/Debug terminados; focal Release **6/6** antes de las
suites finales. El control de asignaciones comprueba cuatro hilos, ventana cerrada,
8.000 requests exactos y reserva calentada/crecimiento; también metadata y fin
anticipado del scope. Suites completas y formato: ver [validación](validacion.md).

Fuentes nuevas y script pertenecen al 112; instrumentar un módulo legacy no cierra
su migración. Inventario/ficha/índice/estado actualizados con el resultado final.
Commit local de cierre: `rtk git log -1 --oneline --grep='^hito 112:'`.
El push autorizado de entrada envió el 111; el commit nuevo del 112 queda local.
113 y Hound H08 no iniciados.
