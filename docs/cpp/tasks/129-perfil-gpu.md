# Hito 129: Perfil GPU del renderer actual y optimización justificada

Estado: **no iniciado**. Depende de: **119 y 128**.
Objetivo: **Investigación GPU y, solo si procede, un cambio medido**. Base de investigación: `5b96a91`.
Leer [CONTEXTO](CONTEXTO.md) y esta ficha; [INVENTARIO](INVENTARIO.md) asigna
0 archivos propietarios a este hito. Los cambios de firmas incluyen
todos los callers aunque su cierre final pertenezca a otro hito.

## Evidencia de partida

El 110 da ~2,046 ms GPU en Factory y diferencias CPU pequeñas frente al frame completo. Hay contadores por pases y draw/upload/skin; sombras, opaque,
tone, temporal y presentation deben separarse antes de elegir el cuello real. No se ha demostrado aún qué shader/pase conviene cambiar.

Entradas principales verificadas (no es una lista exhaustiva de callers):

- [src/backends/diligent_renderer.cpp](../../../src/backends/diligent_renderer.cpp)
- [include/gloom/render/renderer.hpp](../../../include/gloom/render/renderer.hpp)
- [apps/gloom/performance.hpp](../../../apps/gloom/performance.hpp)
- [tools/art/measure_hound_budget.py](../../../tools/art/measure_hound_budget.py)

## Archivos responsables de cierre

Revisión transversal de los archivos del inventario; no tiene archivos propietarios nuevos.

Además se adaptan todos los callers afectados por firmas/lifetime, aunque tengan otro responsable de cierre.

## Trabajo concreto, en orden

1. Medir pases GPU mediante timestamps ya existentes o su extensión mínima con lectura diferida; distinguir GPU de espera CPU/present/VSync. Repetir
   Factory y Hound con mismo contenido y todas las métricas de 112.
2. Ordenar costes de sombras/opaque/transparency/temporal/tone/UI, overdraw, draws, upload y skin. Elegir como máximo un cambio pequeño sobre el coste
   dominante que conserve el resultado y hardware. SIMD/culling nuevo, threading gráfico y cambios de shader requieren evidencia, no se dan por
   necesarios.
3. Implementar ese cambio solo si existe cuello atribuible y validación visual suficiente; comparar A/B serial y revertir si empeora o la ganancia
   queda dentro del ruido. Si no hay candidato rentable, cerrar como perfil investigado sin cambiar renderer.
4. Si salen otras oportunidades independientes, formalizarlas en fichas adicionales numeradas, con la misma evidencia y aprobación de alcance cuando
   corresponda. No convertir este hito en una reescritura abierta.

## Contratos que conservar

No cambiar backend, resolución/LOD/contenido, hardware mínimo ni técnica temporal para aparentar mejora. No incorporar mesh shaders, descriptor heaps,
Slang o NoGraphicsAPI por el mero hecho de usarlos la referencia.

## Validación focalizada

CTest existentes: `gloom.material_render`, `gloom.temporal`, `gloom.vulkan_sync`, `gloom.hound_runtime`, `gloom.visual_review`,
`gloom.factory_visual_review`, `gloom.character_visual_review`, `gloom.ui_visual_review`.

Mismas capturas/umbrales y conteos; synchronization validation, resize y recursos en vuelo. Solo repetir builds/suite ampliada si se modifica código;
un perfil sin cambios se valida mediante integridad de medidas.

Añadir únicamente checks significativos que falten para esos contratos. Aplicar
builds/formato y protocolo común según el alcance; los fallos heredados se comparan
por causa, no por total. No cambiar referencias o umbrales para pasar.

## Medida y criterio de cierre

GPU por pase y completa, CPU de presentación, distribución frame y mismo trabajo. Ganancia fuera del ruido según CONTEXTO o perfil que demuestra por
qué no procede un cambio; sin objetivo porcentual inventado.

Perfil con cuello/evidencia concretos; un cambio probado o decisión documentada de no cambiar. Ninguna mejora de FPS se afirma a partir de una etapa aislada.

Entregar informe `reports/cpp-performance-129/README.md`, actualizar esta
ficha/índice y estado; commit local `hito 129: resultado concreto`, verificado
con hash y workspace. Si un contrato no se satisface, documentar bloqueo; no cerrar.

## Encargo para ejecutarlo aisladamente

> Usa Ponytail full. Lee el inicio de docs/ESTADO_ACTUAL.md, AGENTS.md,
> docs/cpp/tasks/CONTEXTO.md y docs/cpp/tasks/129-perfil-gpu.md. Ejecuta solo
> el hito 129 con sus dependencias ya cerradas; conserva cambios ajenos,
> renderer y recursos. Verifica los contratos/pruebas/medidas de la ficha,
> actualiza informe y estado y crea su commit local. No hagas push ni avances
> al siguiente hito automáticamente.
