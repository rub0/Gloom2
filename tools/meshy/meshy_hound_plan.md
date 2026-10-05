# Meshy MCP — plan de Hound

## Objetivo

Usar Meshy por MCP, en una tarea futura, para generar un modelo 3D de Hound fiel
al concept maestro canónico aprobado. Este archivo prepara el trabajo; no lanza generación.

## Inputs previstos

Rutas relativas a la raíz de Gloom:

| Entrada | Ruta | Estado |
| --- | --- | --- |
| Hound_master | `assets-source/hound/ref/Hound_master.jpg` | Disponible; canon visual aprobado |
| Hound_front | `assets-source/hound/ref/Hound_front.png` | Pendiente |
| Hound_side | `assets-source/hound/ref/Hound_side.png` | Pendiente |
| Hound_back | `assets-source/hound/ref/Hound_back.png` | Pendiente |
| Hound_34 | `assets-source/hound/ref/Hound_34.png` | Pendiente |
| hound_notes.md | `assets-source/hound/docs/hound_notes.md` | Disponible, contenido completo |

Referencia adicional suministrada: `assets-source/hound/ref/Hound_turnaround_sheet.jpg`.
Todavía no se han separado ni validado sus vistas como entradas individuales.
El master define el diseño; el turnaround debe ser coherente con él.

## Output esperado

Modelo 3D completo y fiel al diseño canónico, preferiblemente GLB o FBX, con
materiales y texturas asociados cuando los entregue el proveedor.
Resultados intermedios en `assets-source/hound/work/<iteracion>/` y exportaciones
revisadas en `assets-source/hound/export/<version>/`.
Registrar inputs, prompt, parámetros reales, identificador de tarea, outputs
y revisión para poder reproducir y comparar cada iteración.

## Criterios de validación

- Fidelidad de silueta: figura ancha, poderosa y pesada; proporciones de torso y brazos del master.
- Cara / capucha / pelo: rostro severo reconocible, piel ceniza, ojos naranjas, capucha negra y pelo negro largo.
- Hombros y picos: conservar volumen, formas angulosas, distribución y lectura agresiva frontal y posterior.
- Gauntlets: grandes, peligrosos y afilados; mantener manos y volúmenes sin simplificación excesiva.
- Faldón: tela marrón rota, cosida y remendada; conservar su estructura y contraste.
- Piernas y botas: placas pesadas, rodillas y espinillas legibles; botas grandes y contundentes.
- Tono dark fantasy: desaturado, sombrío, erosionado, pétreo/óseo; sin acabado pulido ni futurista.
- Coherencia entre las cuatro vistas y el master: comprobar todos los puntos de `hound_notes.md`.

Comparar frente, perfil, espalda, tres cuartos y detalles con las entradas.
Marcar discrepancias y revisar el candidato si falla la regla de validación de las notas.
Antes de integración en Gloom, aplicar el contrato `docs/art/hound/H06-contrato-presupuesto.md`
y comparar con las fuentes v16/v17 conservadas; rig, UVs y animaciones requieren sus tareas.

## Preparación de la futura ejecución

1. Preparar las cuatro referencias individuales sin cambiar el diseño; revisar su coherencia y actualizar el estado.
2. Con un nuevo encargo de generación, comprobar las herramientas MCP disponibles y su esquema real.
3. Confirmar en esa ejecución qué entradas y formatos admite la herramienta; no asumir que acepta cinco imágenes o un Markdown.
4. Usar las notas para el prompt y la revisión; enviar las imágenes por el mecanismo que admita la herramienta.
5. Guardar resultados en una iteración nueva, revisarlos y documentar los criterios anteriores.

No se configuran proveedores, conexiones, credenciales, solicitudes ni scripts de ejecución en este setup.
El [prompt futuro](meshy_hound_prompt.txt) es un borrador guardado, no ejecutado.

## Estado actual

- waiting for turnaround completion
- do not call MCP yet
- Plan y prompt disponibles; generación pendiente de completar el set y de un encargo posterior.
