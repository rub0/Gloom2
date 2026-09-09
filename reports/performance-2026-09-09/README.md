# Hito 71, primera pasada de rendimiento

Fecha: 9 de septiembre de 2026.

## Resultado

El renderer ya consumía `RenderBatch`, pero ignoraba esa información y hacía un
`draw_batch` por instancia. Ahora los draws opacos se agrupan por mesh/material;
los transparentes siguen ordenándose por distancia y los snapshots antiguos sin
batches conservan un camino de compatibilidad.

La construcción de iluminación dejó de crear una `std::vector` por cada una de
las 3456 celdas de la rejilla en cada frame. Usa un conteo plano, prefijo y una
segunda pasada para rellenar las referencias. También se reservaron las paletas
de skinning antes de rellenarlas.

Se añadió telemetría al smoke vertical para separar frame CPU, visibilidad,
iluminación y timestamps GPU.

## Medición reproducible

Comando:

```powershell
build/windows-vs/Debug/gloom.exe --vertical-slice-smoke
```

Resultado instrumentado, 360 frames, Debug, Vulkan, 1280×720:

| Métrica | Resultado |
| --- | ---: |
| Frame CPU extremo a extremo | 9,24366 ms / 108,182 FPS |
| Visibilidad | 0,108311 ms |
| Iluminación CPU | 0,275218 ms |
| GPU filtrada | 1,12734 ms |
| GPU sombras | 0,275456 ms |
| GPU opaco | 0,580608 ms |
| GPU tone map | 0,150528 ms |
| GPU temporal | 0,103424 ms |
| Render/output | 1280×720 / 1280×720 |
| Memoria device-local pico | 171,29 / 192 MB |
| Memoria host-visible pico | 7,02 / 16 MB |

Una ejecución previa no instrumentada tardó 10,994 s para los 360 frames; esa
cifra incluye arranque y compilación/carga inicial y no debe compararse como un
FPS sostenido. La cifra de 108 FPS es la referencia interna del smoke, no una
garantía para 1920×1080 ni para una partida humana con todos los efectos.

## Validación

- Build Debug de `gloom` correcto.
- Suite completa: **47/47 pruebas**.
- Smoke vertical incluido en la suite: correcto.
- Smoke Vulkan/sincronización, material y revisiones visuales: correctos.

Logs locales regenerables: `reports/performance-baseline-2026-09-09/` (ignorados
por Git).

## Siguiente paso

Medir una sesión jugable a la resolución real del usuario y perfilar el coste de
presentación: `skin_pose`/`skinned_bounds`, copias de `RenderInstance`, creación
de vectores por frame y coste de sombras por instancia. La cifra del smoke ya
supera 100 FPS, pero todavía no representa el caso que reporta 13 FPS.
