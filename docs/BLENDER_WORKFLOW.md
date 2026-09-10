# Blender → Gloom

10 de septiembre de 2026 · Hito 79. Puente técnico validado. El hito 80 entrega
el primer volumen de Hound y el hito 81 su [revisión 02](art/hound/blockout-v02/README.md),
aprobada por el usuario en el hito 82. Guía 0.1 y boceto 2D 01 también aprobados.

## Instalación local validada

| Componente | Versión y ubicación |
| --- | --- |
| Blender portable | 4.5.13 LTS, `.cache/blender/blender-4.5.13-windows-x64/` |
| Python aislado | 3.12.10, `.cache/blender-mcp-env/` |
| Blender MCP | 1.9.1; addon incluido 1.6, protocolo 5 |
| SDK MCP | 1.30.0; el campo serverInfo.version anuncia el SDK, no la versión de blender-mcp |
| Perfil de Blender | `.cache/blender-profile/`; separado de las preferencias personales |
| Registro en Codex | `.codex/config.toml`, solo este proyecto, rutas absolutas |

Fuentes: [Blender 4.5 oficial](https://download.blender.org/release/Blender4.5/),
[Blender MCP](https://github.com/ahujasid/blender-mcp),
[paquete 1.9.1](https://pypi.org/project/blender-mcp/1.9.1/) y
[configuración MCP de Codex](https://learn.chatgpt.com/es-419/docs/extend/mcp).
Blender es software libre y el conector tiene licencia MIT. No se han usado
Hyper3D, Hunyuan3D ni otros servicios de generación 3D de pago; no hay claves ni
suscripciones nuevas. Esto no cambia los límites de uso de Codex.

El ZIP oficial `blender-4.5.13-windows-x64.zip` se verificó con SHA-256:

```text
B5FDF800CE65FA2F209E8F68D02667E4D720FA1C42F247C72D1882AB04DECBA6
```

Para reconstruir la instalación en este checkout, descargar ese ZIP del
directorio oficial, verificar `Get-FileHash -Algorithm SHA256` y extraer su
carpeta en `.cache/blender/`. Crear el entorno usando Python 3.12:

```powershell
Set-Location D:/Projects/Gloom
py -3.12 -m venv .cache/blender-mcp-env
& .cache/blender-mcp-env/Scripts/python.exe -m pip install -r tools/art/requirements.txt
```

En esta máquina se usó `.cache/legacy-tools/Scripts/python.exe -m venv` como
intérprete de origen, sin modificar sus paquetes. Los dos paquetes principales
están fijados; no es un lock completo de dependencias transitivas. Binarios,
entornos, descargas y preferencias se excluyen del repositorio mediante `.cache/`.
Si cambia la ruta del checkout, ajustar `.codex/config.toml` y las rutas de
exportación de `tools/art/create_bridge_probe.py`.

## Arranque y conexión

```powershell
Set-Location D:/Projects/Gloom
./tools/art/start_blender.ps1
```

El lanzador abre una instancia nueva con configuración de fábrica, copia el
addon desde el paquete fijado y usa el perfil local del proyecto. No carga ni
sobrescribe archivos de trabajo existentes. Reutilizar esa instancia para las
llamadas MCP; no lanzar una segunda mientras el puerto 9876 esté ocupado.
El mensaje `GLOOM_BLENDER_READY` confirma que el addon escucha.

La configuración está registrada y aparece habilitada en `codex mcp list`.
La lista de herramientas de una tarea ya iniciada puede requerir recargar los
servidores MCP o reiniciar Codex. En el hito 79 las llamadas se probaron mediante
el cliente stdio real de `check_blender_mcp.py`; no se afirma que las herramientas
Blender ya aparezcan en el manifiesto de la tarea actual.

Comprobar sin alterar la escena:

```powershell
& .cache/blender-mcp-env/Scripts/python.exe tools/art/check_blender_mcp.py --goal 'Comprobar la conexión local con Blender'
```

El cliente hace initialize, list_tools y get_scene_info. `--script ARCHIVO.py`
envía un script revisado a execute_blender_code; `--screenshot ARCHIVO.png`
guarda la imagen devuelta por get_viewport_screenshot. No son una API propia
de pago ni una conexión directa al socket que eluda el modo seguro del MCP.

## Límites de seguridad

- Socket del addon: solo `localhost:9876`. No abrir puertos ni exponerlo a la red.
- Telemetría desactivada en servidor y addon. Integraciones externas desactivadas.
- `BLENDER_MCP_SAFE_MODE=1`: restringe el código que pasa por el servidor MCP.
- Solo se habilitan cuatro herramientas en Codex: escena, objeto, captura del
  viewport y ejecución de scripts Blender. La prueba CLI enumera las herramientas
  del servidor completo; esa enumeración no habilita proveedores en Codex.
- El modo seguro no es un aislamiento del sistema operativo. `bpy` puede guardar,
  exportar y modificar escenas. El socket local del addon no está autenticado;
  otros procesos locales podrían conectarse directamente sin pasar por el filtro
  del MCP. Usar solo código de confianza y cerrar Blender al terminar la sesión.
- No modificar el `.blend` del artista ni escenas ajenas: trabajar con copias
  versionadas y guardar antes de operaciones de reemplazo.

## Contrato de exportación comprobado

Usar **glTF 2.0 separado**: `.gltf` + `.bin` + imágenes PNG externas. El cooker
actual rechaza imágenes embebidas en GLB: el primer intento lo detectó y no se
ha ampliado el importador para ocultar esa limitación.

Metros, Z-up en Blender y `export_yup=True` para Gloom. Aplicar escalas de los
objetos, exportar solo la selección, conservar normales y UV0. Usar Principled
BSDF con base color, metallic/roughness y emisión; hornear a texturas externas
los nodos procedurales del acabado, no asumir que el motor interpreta nodos Blender.
La prueba verifica metal, rugosidad, emisión y una textura de color. No demuestra
todavía normal maps, alpha, varias UVs, esqueletos ni clips animados desde Blender.

## Prueba reproducible

Ejecutar la generación únicamente en la instancia nueva de prueba. El script
reemplaza sus objetos y vuelve a guardar `bridge-probe.blend` y la fixture; se
niega a trabajar sobre otro `.blend` guardado. No usarlo sobre trabajo sin guardar.

```powershell
New-Item -ItemType Directory -Force .cache/blender-bridge/cooked | Out-Null
& .cache/blender-mcp-env/Scripts/python.exe tools/art/check_blender_mcp.py --goal 'Validar el puente Blender a Gloom' --script tools/art/create_bridge_probe.py --screenshot .cache/blender-bridge/blender-viewport.png
& .cache/blender-mcp-env/Scripts/python.exe tools/art/verify_bridge_probe.py
cmake --build --preset windows-release --target gloom_scene_viewer gloom_asset_cooker gloom_asset_tests gloom_gpu_asset_tests --parallel 8
& build/windows-vs/Release/gloom_asset_cooker.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/blender-bridge/cooked game:/tests/blender_bridge/bridge-probe.gltf cache:/bridge-probe.gasset
& build/windows-vs/Release/gloom_scene_viewer.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/blender-bridge/cooked game:/tests/blender_bridge/bridge-probe.gltf cache:/bridge-probe.gasset 1 D:/Projects/Gloom/.cache/blender-bridge/gloom-preview.ppm
ctest --preset windows-release -R '^gloom\.(assets|gpu_assets)$' --output-on-failure
```

Detener la secuencia si un comando devuelve código distinto de cero. La fixture
está en `assets/tests/blender_bridge/`: 3 mallas, 424 triángulos, 3 materiales y
1 PNG de 8 × 8. No representa la calidad de un personaje; solo prueba el transporte
de datos. El `.blend` se regenera en `.cache/blender-bridge/`.

El argumento final del visor es opcional. Con él captura después de 32 frames
con escena residente y sale por la ruta normal de liberación del renderizador;
limita la espera a 10000 frames. Sin ese argumento conserva el visor interactivo.
La captura es un archivo del renderizador real, no una imagen generada por IA.

## Hound básico aprobado; siguiente entrega: rig de prueba

Entregado como volumen 01 en el hito 80, corregido en v02 en el hito 81 y
aprobado en el hito 82. Las herramientas Blender ya se han usado de forma nativa.
Conservar las fuentes v01/v02; preparar malla y rig de prueba en una versión nueva.

Mantener silueta, escala y correcciones aprobadas. Revisar topología de zonas
deformables y separación de placas rígidas; evaluar el rig existente antes de
reutilizarlo. Entregar pruebas de caminar, apuntar y Bite, con revisión de
penetraciones, conservación de volumen y agarres. El rig de 43 huesos es candidato
a reutilización, no compatibilidad demostrada por las pruebas estáticas.
UVs/texturas finales y sustitución del personaje jugable quedan para sus fases
posteriores. Esta aceptación no cambia gameplay ni autoriza un push.
