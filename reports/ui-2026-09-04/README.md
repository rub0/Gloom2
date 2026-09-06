# Aceptación del hito 64 — interfaz gráfica

Fecha: 4 de septiembre de 2026. Base conservada: `effe13f`.
La comprobación previa a las ediciones pasó las **38/38 pruebas** del hito 63
en 174,18 s (`tests-baseline.log`). No se han sustituido referencias visuales
anteriores ni modificado la copia Legacy.

**Hito 64 completado: 41 pruebas validadas.** La pasada completa terminó con
40 aprobadas en 216,48 s; la galería detectó que el hover dependía del cursor
dejado por otras pruebas. Tras neutralizar el input solo en sus fixtures,
`ctest --rerun-failed` aprobó la prueba restante en 18,54 s, sin cambiar
referencias ni tolerancias. No quedan fallos pendientes.

## Entrega

La aplicación sin argumentos abre el menú gráfico. Incluye juego local,
navegador asíncrono, conexión directa, sala de desarrollo, selección, listo,
HUD, pausa, carga, errores, reintento, acceso, reconexión y salida confirmada.
Usa los servicios y reglas existentes; no cambia el protocolo 14 ni añade
armas, habilidades o clases. [Documentación](../../docs/UI.md) y
[ADR 0065](../../docs/architecture/0065-original-graphical-interface.md).

![Menú gráfico](menu.png)

## Fuentes y aceptación visual

La extracción offline recupera **112 bitmaps de 17 SWF**, además del fondo y
las dos fuentes originales. La reproducción independiente verifica **117 archivos
idénticos y 20 fuentes** (`reproduction.log`). El inventario y hashes están en
[manifest.json](../../assets/ui/original/manifest.json), junto a las imágenes
recuperadas. No se incorpora un runtime Flash/Hikari.

El HUD se contrastó con el fotograma **0:45**, dentro del tramo autorizado
0:00–2:00 del vídeo original. Recupera el marco ornamental turquesa con extremos
proporcionados, retícula e iconos originales. La información se reorganiza para
vida/escudo, arma y habilidad actuales; no reproduce munición ni temporizador
de reglas que la simulación no implementa. Los menús se contrastan con los
recursos SWF originales, dado que el vídeo no ofrece una referencia suficiente.
Los personajes y sus materiales conservan la aceptación por bocetos del hito 63.

![HUD original, 0:45](original-hud-0045.png)
![HUD actual en partida con Archangel](game-archangel.png)

Se revisaron **16 estados × 3 resoluciones = 48 capturas Vulkan**: 1280×720,
1920×1080 (150%) y 2560×1080. Texto, paneles, marco y controles permanecen en
el área segura; la retícula se mantiene centrada. Las referencias nuevas viven
en `tests/ui-visual-references`. El comparador incluye controles negativos de
imagen vacía y cuadrante ausente. No se reajustaron umbrales anteriores.

- [Galería 1280×720](gallery-1280.jpg).
- [Galería 1920×1080](gallery-1920.jpg).
- [Galería ultrawide 2560×1080](gallery-2560.jpg).

Las galerías incluyen HUD, respawn, lista vacía y poblada, error, conexión directa,
sala, selección, pausa, carga, autenticación por dispositivo y confirmación.
Los datos de autenticación/listado de la galería son fixtures de presentación.

## Recorrido y validación

`gloom.ui_flow` inicia un servidor dedicado y **dos procesos gráficos distintos**,
Nyx/Archangel y Rook/Shadow. Inyecta eventos en la frontera de input de la UI:
buscar → elegir sala → seleccionar → listo → jugar → reconectar → abandonar.
El directorio es el existente en memoria; GNS, admisión y reanudación usan el
transporte real. Ambos clientes comprueban que conservan la entidad al reconectar.
No se presenta esta prueba como una sesión autenticada en un servicio público.

![Recorrido Archangel](flow-archangel.jpg)
![Recorrido Shadow](flow-shadow.jpg)

Las pruebas unitarias comprueban foco, Tab/Intro, ausencia de repetición por
tecla sostenida, ventana sin foco, ratón, transformación DPI/ultrawide y edición
de texto UTF-8. Las capturas y el recorrido rechazan errores de validación Vulkan.
La reconstrucción de render targets espera a sus usuarios antes de cambiar
descriptores TAA/tone mapping; redimensionar al mismo tamaño no recrea recursos.

Los registros finales están en `tests-final.log`, `tests-rerun.log`, `build-all.log`, `ui-tests.log`,
`reproduction.log` y `evidence.json`. Las capturas completas y logs de cada proceso
se regeneran bajo `build/windows-vs/ui-review/Debug` y `ui-flow/Debug`.

## Reproducción y límites

```powershell
& D:/Dev/CMake/bin/cmake.exe --preset windows
& D:/Dev/CMake/bin/cmake.exe --build --preset windows-debug
& D:/Dev/CMake/bin/ctest.exe --preset windows-debug --output-on-failure
.\build\windows-vs\Debug\gloom.exe
```

La navegación con teclado y ratón se valida en la capa de input; no se afirma
una prueba física en un monitor DPI distinto. Se renderizan realmente las tres
resoluciones. El atlas cubre Latin-1; otros caracteres usan `?`. Menú y partida
recrean su ventana al cambiar de contexto, conservando el acceso en memoria.
Crear sala sigue siendo el host de desarrollo con dos jugadores remotos.
Las cuentas públicas requieren el entorno de GAME_AUTH.md. Audio, ajustes amplios,
balance, nuevo contenido y despliegue quedan aplazados.
