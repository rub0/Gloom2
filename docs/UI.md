# Interfaz gráfica: hito 64

La aplicación sin argumentos abre el menú gráfico. Conserva las opciones CLI
de herramientas y pruebas. La partida usa Factory, los modelos, animaciones y
efectos del hito 63; esta entrega no añade armas, poderes ni reglas de balance.

Estado: hito 64 completado, **41 pruebas validadas** (40 en la pasada completa y
la regresión UI aprobada al repetirla tras aislar el cursor de sus fixtures).
La evidencia se guarda en [el informe](../reports/ui-2026-09-04/README.md).

## Recursos y presentación

`tools/legacy/import_ui.py` audita los 17 SWF de `Exes/media/gui` y recupera 112
bitmaps JPEG/ARGB. No ejecuta Flash, Hikari ni ActionScript. Conserva las imágenes
extraídas y el inventario de tags. El atlas usa el fondo original del menú,
el logo, retratos de selección, iconos y el marco turquesa de `Hud.swf`.
Las esquinas del marco conservan su proporción; solo se estira el tramo central.
Se rasterizan HammerheadThin para títulos y DejaVuSans para texto desde los TTF
originales. El manifiesto registra los hashes de los 20 archivos fuente.

La capa UI produce quads con color, UV y un único atlas RGBA. Diligent los dibuja
después de tone mapping y TAA, con mezcla alfa y sin profundidad, bloom ni
historial temporal. Se admiten 65.536 vértices por frame. El atlas se carga por
la cola de residencia existente, con su presupuesto y liberación habitual.

Diseño en un área segura de 1280×720, centrada y escalada uniformemente al
drawable físico. Dibujo e hit testing comparten esa transformación, incluido
el factor de píxeles de SDL en pantallas con DPI alto. Ultrawide amplía el fondo
sin estirar texto ni desplazar la retícula del centro de la cámara.

## Pantallas y controles

- Menú: local, navegador, conexión directa, crear sala y salida confirmada.
- Selección: los loadouts existentes de Hound, Archangel y Shadow. No se ofrece
  Screamer sin contenido, ni Berserker como clase independiente.
- Navegador: búsqueda asíncrona, salas paginadas, vacío, error, refresco y acceso.
- Sala: nombres, conexión/listo, elección y confirmación. La autoridad inicia
  la partida cuando ambos jugadores están listos.
- HUD: vida, escudo, retícula/impacto confirmado, arma y disponibilidad,
  habilidad/cooldown, bajas/muertes y cuenta atrás de respawn. No se inventa un
  contador de munición: Soul Reaper no tiene esa reserva en la simulación actual.
- Pausa, pérdida de foco, reconexión y abandono con confirmación. La pausa local
  detiene los ticks; la partida online continúa con input neutro.

Ratón o Tab/Mayús+Tab/flechas arriba-abajo para recorrer controles; Intro activa
el foco y Esc vuelve o abre pausa. Los campos admiten texto, retroceso,
Ctrl+A y pegado Ctrl+V. El cursor queda libre en menús, carga y reconexión, y se
captura al jugar. Al recuperar el control se espera a soltar los botones de
acción para no disparar al pulsar «Continuar».

## Red y autenticación

El navegador utiliza `AsyncSliceMatchBrowser` y `SliceMatchDirectory`. Se mantienen
las variables de [MATCH_SERVICE.md](MATCH_SERVICE.md), [KEYCLOAK.md](KEYCLOAK.md)
y [GAME_AUTH.md](GAME_AUTH.md). El acceso Keycloak muestra dirección y código de
dispositivo dentro de la ventana, permite cancelar y espera la aprobación sin
bloquear el render. Registry usa la credencial configurada. «Acceso» inicia otra
autenticación cuando hay que renovar la sesión.

La selección de una sala se vuelve a resolver antes de conectar. Tickets de
juego, credenciales y tokens de reanudación siguen pasando por las clases
existentes; no se dibujan ni se registran tokens. Errores de ticket o admisión
se presentan con reintento. Reconectar conserva la identidad y limpia el
historial de presentación y efectos antiguos.

«Crear sala local» conserva el host de desarrollo existente: es una ventana
servidor con dos plazas **remotas**, no un tercer jugador. Para publicación y
cuentas verificadas se conserva el servidor dedicado. Al entrar en partida se
cierra la ventana del menú y se crea la del juego; al abandonarla se reconstruye
el menú, conservando el contexto de acceso en memoria.

## Reproducir

```powershell
& 'D:\Dev\CMake\bin\cmake.exe' --preset windows
& 'D:\Dev\CMake\bin\cmake.exe' --build --preset windows-debug
& 'D:\Dev\CMake\bin\ctest.exe' --preset windows-debug --output-on-failure
.\build\windows-vs\Debug\gloom.exe
.\build\windows-vs\Debug\gloom.exe --ui-review .cache/ui-review
```

`--ui-review` captura 16 estados a 1280×720, 1920×1080 y 2560×1080. Los ejemplos
de autenticación, lista y HUD de esa galería son fixtures de presentación;
no acreditan acceso a una cuenta externa. El recorrido `gloom.ui_flow` abre
dos procesos gráficos y un servidor real. Inyecta mouse/teclado en la misma
frontera de input que usa la UI, elige una sala del directorio en memoria
existente y prueba listo, partida, reconexión y abandono. El transporte GNS y
el protocolo son reales. El despliegue público y las cuentas de operador siguen
requiriendo el entorno descrito en GAME_AUTH.md.

Para reproducir los recursos, usar Python/Pillow/numpy:

```powershell
python tools/legacy/import_ui.py --legacy-root D:/Projects/Gloom-Legacy --output-root assets/ui/original
python tools/legacy/verify_ui.py --legacy-root D:/Projects/Gloom-Legacy --reference-root assets/ui/original --work-root .cache/ui-reproduction
```

La copia Legacy y las referencias visuales de hitos anteriores se conservan.
Audio, ajustes amplios, cambios de cuenta, balance y despliegue público quedan
fuera del hito 64.

## Correcciones de aceptación — 5 de septiembre de 2026

La vida y el escudo aparecen en barras verticales fuera del marco de armas,
con los dos símbolos recuperados de `Hud-83.png`. El marco muestra los cinco
iconos originales, con atenuación del inventario no adquirido y la munición
del arma seleccionada. El círculo superior izquierdo contiene la habilidad
y su progreso circular de cooldown; los otros círculos quedan sin habilidad
mientras esos slots no estén implementados. FPS medidos cada medio segundo y
coordenadas XYZ del jugador aparecen arriba a la izquierda.

Factory añade relleno ambiente difuso (0,65 sobre la radiancia de cielo/suelo),
conserva el probe y aumenta un 20 % las luces puntuales y la direccional.
La lava mata en el primer tick de contacto, incluso con escudo.
La evidencia de esta revisión está en `reports/gameplay-fixes-2026-09-05`.
