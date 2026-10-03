# Hito 121: Audio sin propietarios compartidos ni temporales de mezcla

Estado: **no iniciado**. Depende de: **114 y 117**.
Objetivo: **CPU, memoria y propiedad**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
12 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

Clip posee samples vector y voces comparten shared_ptr; Output es virtual con fábricas unique_ptr de SDL/nulo. Mixer se ejecuta en el hilo principal y
SDL posee su cola: ningún callback lee Mixer. Hay límite de voces por defecto 64 y caminos con/sin dispositivo.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [include/gloom/audio/mixer.hpp](../../../include/gloom/audio/mixer.hpp)
- [src/audio/mixer.cpp](../../../src/audio/mixer.cpp)
- [src/backends/sdl_audio.cpp](../../../src/backends/sdl_audio.cpp)
- [src/gameplay/audio_events.cpp](../../../src/gameplay/audio_events.cpp)
- [src/gameplay/audio_presentation.cpp](../../../src/gameplay/audio_presentation.cpp)
- [apps/gloom/audio_review.hpp](../../../apps/gloom/audio_review.hpp)

## Archivos responsables de cierre

- [apps/gloom/audio_review.hpp](../../../apps/gloom/audio_review.hpp)
- [include/gloom/audio/events.hpp](../../../include/gloom/audio/events.hpp)
- [include/gloom/audio/mixer.hpp](../../../include/gloom/audio/mixer.hpp)
- [include/gloom/gameplay/audio_events.hpp](../../../include/gloom/gameplay/audio_events.hpp)
- [include/gloom/gameplay/audio_presentation.hpp](../../../include/gloom/gameplay/audio_presentation.hpp)
- [src/audio/mixer.cpp](../../../src/audio/mixer.cpp)
- [src/backends/sdl_audio.cpp](../../../src/backends/sdl_audio.cpp)
- [src/gameplay/audio_events.cpp](../../../src/gameplay/audio_events.cpp)
- [src/gameplay/audio_presentation.cpp](../../../src/gameplay/audio_presentation.cpp)
- [tests/audio_backend_tests.cpp](../../../tests/audio_backend_tests.cpp)
- [tests/audio_network_tests.cpp](../../../tests/audio_network_tests.cpp)
- [tests/audio_tests.cpp](../../../tests/audio_tests.cpp)

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Dar a clips un dueño estable de catálogo/escena y a voces handles/referencias cuya vida cubra reproducción. Una recarga o stop_scene no libera
   samples mientras una voz los lee; al apagar detener voces, completar pump y cerrar output antes de retirar catálogo.
2. Migrar samples/voices/cues/presentation a arrays reservados y Span; mantener PCM decoding, resampling, spatial gains, cursor/loop, prioridades,
   buses, pausa y métricas. Render escribe en la salida del llamador sin array temporal.
3. Usar salida concreta con selección explícita SDL/nula, sin jerarquía/factory/PIMPL. Mantener diagnóstico e inicio fallido por dispositivo externo;
   no mover mezcla al callback ni añadir un sistema de audio nuevo.

## Contratos que conservar

No cambiar volumen/atenuación ni silenciar efectos para pasar presupuesto. Errores de archivos/dispositivo siguen siendo recuperables; precondiciones
internas y estados de voices usan asserts.

## Validación focalizada

CTest existentes: `gloom.audio`, `gloom.audio_no_device`, `gloom.audio_network`, `gloom.animation_vfx`, `gloom.vertical_slice_smoke`.

Clips corruptos/truncados, mono/estéreo, frecuencias, loop y final, stop_scene/reload, 0/64/exceso de voces, pausa y dispositivo ausente. Comparar
muestras/tolerancias actuales y escuchar el escenario de revisión existente.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

1.000 bloques de mezcla y pump dentro de reserva sin asignaciones propias de Mixer; CPU audio con dispositivos nulo y real, samples y underruns/colas.
Los internals de SDL se contabilizan aparte.

Propietarios de clips y caducidad de voces claros; tests con/sin device correctos y ruta propia sin STL/shared_ptr/virtual/PIMPL/excepciones/RTTI.

Entregar informe `reports/cpp-performance-121/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 121: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/121-audio.md. Ejecuta solo
> el hito 121 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
