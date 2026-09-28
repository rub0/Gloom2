# Hito 103 — normales correctas en la ruta RGBA8

28 de septiembre de 2026. Corrección técnica encargada por el usuario,
independiente de H06. El hito 102 de ocho combatientes sigue pendiente.

## Causa y corrección

El cooker usa KTX 4.4.2 y UASTC con `normalMap`: almacena X/Y como RRRG,
con Y en alfa. Es el empaquetado descrito por la
[especificación KTX](https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html).
La transcodificación BC5 de la biblioteca toma R/A y ya conserva ambos canales.
RGBA8 expande literalmente RRRG; el shader esperaba X/Y en R/G. El visor de
este equipo selecciona RGBA8, confirmado por su nuevo diagnóstico de ruta.

Esto precisa el diagnóstico 101: la pérdida ocurría al interpretar la expansión
RGBA8, no en el PNG ni en BC5. En diez casos iniciales, 54/60 comprobaciones de
R/G crudo RGBA fallaban (error máximo 112/255); BC5 pasaba 60/60.

El decoder común reconoce el descriptor UASTC RRRG **antes** de transcodificar
y copia A a G en cada mip RGBA8. La operación se hace en el buffer de carga
existente; no añade asignaciones ni trabajo por fotograma. No cambia otros
descriptores, BC5, cooker, shader ni formato de archivo. Los recursos cocinados
existentes funcionan con el decoder corregido, sin migración ni nueva cocción.

## Comprobaciones

- Regresión incorporada al test existente `gloom.assets`: canales X/Y
  independientes en RGBA8 y bloques BC5, incluyendo el mip 1×1. La versión
  inicial de la comprobación falla sin el arreglo con `Normal XY changed in
  RGBA8 fallback or its mip chain`. Se reforzó además la imagen para que también
  R/G sean distintos en el mip 1×1 (128/191); pasa con la corrección.
- Release: `assets`, `gpu_assets`, `material_render`, **3/3**. Después de reforzar
  el mip se repitió `assets`, correcto. Debug: `assets`, **1/1**.
- **12/12** comparaciones Blender → glTF → cooker → visor de H06 pasan,
  incluidos los dos controles de normales que antes fallaban. Los umbrales
  originales se conservan: equivalencia <3 y cambio visible >5 en la región
  interior x530–709/y180–359, diferencia media por canal de 0 a 255.
- **144/144** controles KTX: doce casos × seis mips × dos salidas. Comprueban
  X/Y del RRRG crudo en R/A y su transcodificación BC5, error máximo observado
  **0/255** (tolerancia 4). Esta prueba C API verifica el empaquetado; la
  corrección RGBA del runtime se prueba en C++ y en las capturas GPU.
- **9/9** comparaciones GPU adicionales: X/Y independientes, reflexiones U/V
  con signos compensados, y costuras al separar triángulos en islas UV con
  desplazamientos de tiles enteros. Derivan de la esfera Blender de H06.
  Mantienen geometría/normales; el cambio de islas no debe cambiar el sombreado.
  No certifican todavía las costuras de un bake final de Hound.
- Capturas inspeccionadas. Ruta gráfica observada: **RGBA8**. BC5 validado por
  sus valores decodificados; no se afirma una captura GPU BC5.
- Juego Debug/Release recompilado; herramientas y pruebas Release compiladas.
  No se ha ejecutado la suite completa ni el benchmark de ocho combatientes.
  Los tres SHA-256 de fuente/glTF/BIN v16 coinciden con su manifiesto.

Resultados conservados aquí; imágenes/JSON/logs son regenerables e ignorados.

| Comparación H06 | Diferencia media interior | Resultado |
| --- | ---: | --- |
| Base / normal variable en Y | 33,172078 | Pasa (>5); antes 0,536204 |
| Base / normal X | 23,953158 | Pasa (>5) |
| Base / normal Y | 81,492006 | Pasa (>5); antes 0,536204 |
| Color / factor lineal | 0,169763 | Pasa (<3) |
| ORM / factores | 1,059516 | Pasa (<3) |
| Emisión / factor | 0,044383 | Pasa (<3) |

Las otras seis comparaciones existentes (ORM, AO, emisión, UV0/UV1, MASK,
BLEND) pasan sin cambiar sus criterios.

| Control adicional | Diferencia media interior | Resultado |
| --- | ---: | --- |
| X / U reflejada, signo X compensado | 0,000000 | Pasa (<3) |
| Y / U reflejada | 0,250226 | Pasa (<3) |
| X / V reflejada | 0,791245 | Pasa (<3) |
| Y / V reflejada, signo Y compensado | 0,447160 | Pasa (<3) |
| X / islas separadas con costuras | 0,777531 | Pasa (<3) |
| Y / islas separadas con costuras | 0,650473 | Pasa (<3) |
| Plana / X | 23,983241 | Pasa (>5) |
| Plana / Y | 80,965473 | Pasa (>5) |
| X / Y | 79,696698 | Pasa (>5) |

## Reproducción

Desde la raíz del proyecto, con las dependencias ya instaladas:

```powershell
rtk proxy D:/Dev/CMake/bin/cmake.exe --build build/windows-vs --config Release --target gloom gloom_asset_cooker gloom_scene_viewer gloom_asset_tests gloom_gpu_asset_tests gloom_material_render_tests --parallel 8
rtk proxy D:/Dev/CMake/bin/cmake.exe --build build/windows-vs --config Debug --target gloom gloom_asset_tests --parallel 8
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Release --output-on-failure -R '^gloom[.](assets|gpu_assets|material_render)$'
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Debug --output-on-failure -R '^gloom[.]assets$'
```

Si faltan las muestras Blender H06, generarlas en un proceso desechable:

```powershell
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background --factory-startup --python-exit-code 1 --python tools/art/create_hound_h06_probe.py
```

Ejecutar las comprobaciones GPU secuencialmente:

```powershell
rtk proxy .cache/legacy-tools/Scripts/python.exe tools/art/verify_hound_h06_probe.py
rtk proxy .cache/legacy-tools/Scripts/python.exe tools/art/verify_normal_maps.py
```

El primer script ahora termina correctamente; el fallo que documenta el
informe histórico 101 ya no se espera. El segundo escribe las capturas/logs
en `.cache/hound-normals-103/` y resultados en este directorio. `--cpu-only`
omite capturas y comprueba únicamente el empaquetado KTX y BC5.

H06 queda pendiente del presupuesto y de la reproducción animada del hito 102,
aplazado por el usuario. H07 no iniciada. Commit local del hito 103, sin push;
resolver su hash con `git log --oneline --grep='^hito 103:'`.
