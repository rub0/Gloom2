# Hito 91 — H01, manos y guanteletes de Hound

16 de septiembre de 2026. Solo H01 del índice; H02 y las demás fichas no se ejecutan.
Entrada: `art/characters/hound/v08/hound-mesh-v08.blend`, base `68e686e`;
índice inicial `60cdf18`. Workspace limpio al empezar.
[Vistas, fuente y vídeo](../../docs/art/hound/mesh-v09/README.md).

## Resultado y decisiones

- Dos carcasas huecas de antebrazo con pared nominal de 7 mm y alivio distal
  de 16 mm; dos aros de muñeca de 4 mm; 28 falanges dorsales de 2,2 mm con
  palma libre y separación articular. Extremos del pulgar retranqueados.
- Guantes conexos: relieve tenar/concavidad palmar y ajuste dorsal bajo la placa.
  Las dos placas frontales de antebrazo conservan topología/pesos y retraen
  progresivamente su extremo inferior hasta 50 mm para liberar pulgar/muñeca.
  La prueba ampliada detectó el contacto que motivó este ajuste.
- 32 componentes reconstruidos y cuatro ajustados. Los otros 60 conservan
  exactamente vértices, caras, materiales, sombreado y pesos. Entre ellos están
  ambas placas dorsales completas en pico, filos/codos, capucha/clavículas,
  rostro, brazos, ropa y piernas. Ningún cambio de huesos, bind pose o claves.
- 23.522 vértices y 46.680 triángulos: +6.800 triángulos, 17,1 % sobre v08.
  Son espesores y superficies de autoría, no presupuesto de producción.
  Sin código de runtime ni asignaciones por frame nuevas; siguen un mesh/skin,
  siete primitivas/materiales y 87 componentes rígidos. Densidad/LODs quedan en H06/H07.

## Ensayo de Soul Reaper: decisión para H08

Geometría original intacta, escala del nodo 0,15 y factor adicional 0,43 tomado
de `src/gameplay/character_animation.cpp`. El socket normalizado inicial
(0,13; −0,20; 0,25), convertido a ejes Blender, no permite cerrar esta mano
sin penetración. No se adopta para Hound v09.

El ensayo entregado apoya los dedos contra la parte posterior de la carcasa:
socket de revisión en ejes Blender (0,10; −0,75; 0,28), factor 0,43;
destino `wrist + down*0.096 - dorsal*0.045`, con Z del arma hacia
el eje transversal de la mano y −Y hacia su dirección distal.
La receta exacta está en `tools/art/hound_h01_review.py::weapon`.

Cero cruces arma/mano en esa pose y mínima distancia muestreada de 0,558 mm.
**Es apoyo de prueba, no empuñadura cerrada:** la palma no apoya en la carcasa.
H08 debe resolver agarre firme, oposición del pulgar, mano de apoyo, socket y
pose de apuntado. No se certifican gatillo, varias armas, disparo o recarga.
No se inventa un mango ni se modifica el arma para ocultar este límite.
La revisión artística global y su aceptación siguen reservadas a H05.

## Validación realizada

- Fuente final reabierta. 36 superficies intervenidas cerradas, conexas,
  manifold, orientadas, con volumen positivo y sin degenerados. Euler 0 en
  carcasas/aros huecos y 2 en falanges, guantes y placas frontales.
  Sin cruces entre caras no adyacentes de esas superficies en reposo.
- 60 componentes exactos; palmas/pulgares y pico dorsal conservados.
  Pesos normalizados, máximo dos influencias; 87 componentes ligados
  rígidamente a su hueso. Los 53 huesos, jerarquía, matrices y claves son iguales.
- 61 muestras del clip: coordenadas finitas, caras no colapsadas y retorno a
  reposo. Error máximo de distancias rígidas: 0,000000687 m.
- Mano abierta, puño, flexión +25°, extensión −25° y apoyo de arma.
  Por cada mano/pose: 121 pares comprobados entre falanges, placa dorsal,
  guante, aro y carcasas/placas/filos/codo del antebrazo. Cero cruces en esos
  1.210 pares evaluados; cero autointersecciones no adyacentes de los guantes.
  Prueba adicional mano derecha/Soul Reaper: cero cruces en el apoyo mostrado.
- glTF reimportado en 13 poses: error máximo 0,000002036 m, un mesh/skin,
  siete primitivas, 53 huesos, 43 nombres Legacy y un clip diagnóstico.
  Sin cámaras, luces, arma o acción de vídeo en la exportación.
- Regresión v08 del verificador compartido de roundtrip: pasa, mismo error máximo.
- Cooker: escena `9345383975249807383`, cero dependencias externas.
  Visor Diligent/Vulkan: 7/7 piezas, siete batches, salida 0; captura estática
  inspeccionada. No demuestra reproducción animada dentro del motor.
- CTest Release `gloom.assets`/`gloom.gpu_assets`: 2/2.
  Debug `gloom.animation_vfx`: 1/1. Binarios existentes, sin cambios C++.
- 13 PNG reales inspeccionados; comparativas a igual cámara/escala/luz.
  Vídeo H.264 decodificado: 151 frames a 30 fps, 800×1000, 5,033333 s.
  Vistas clave de apertura/cierre y flexión/extensión inspeccionadas.
- Sintaxis de scripts, líneas de código hasta 160 caracteres, enlaces y diff revisados.

Los tests BVH comprueban cruces de superficies de los pares nombrados; no
certifican separación continua en todo movimiento ni contención volumétrica.
No se incluyen pares armadura/piel del antebrazo, contactos globales o colisión
de combate. El apoyo del arma no prueba una empuñadura de producción.

## Reproducción

Todos los comandos se ejecutaron con prefijo `rtk`. Los logs intermedios
y el cocinado están en `.cache/hound-mesh-v09/` y no se versionan.
El generador rechaza una v09 existente: reproducir desde v08 en una copia
sin salida v09, conservando las fuentes y cualquier edición manual.

```powershell
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v08/hound-mesh-v08.blend --python-exit-code 1 --python tools/art/refine_hound_mesh_v09.py --python tools/art/review_hound_mesh_v09.py
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v09/hound-mesh-v09.blend --python-exit-code 1 --python tools/art/verify_hound_mesh_v09.py --python tools/art/verify_hound_rig_roundtrip_v03.py -- v09
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v09/hound-mesh-v09.blend --python-exit-code 1 --python tools/art/animate_hound_hands_v09.py
rtk proxy .cache/blender/blender-4.5.13-windows-x64/blender.exe --background art/characters/hound/v08/hound-mesh-v08.blend --python-exit-code 1 --python tools/art/verify_hound_rig_roundtrip_v03.py -- v08
rtk proxy build/windows-vs/Release/gloom_asset_cooker.exe assets .cache/hound-mesh-v09/cooked game:/characters/hound_rig/v09/hound-rig.gltf cache:/hound-rig.gasset
rtk proxy build/windows-vs/Release/gloom_scene_viewer.exe D:/Projects/Gloom/assets D:/Projects/Gloom/.cache/hound-mesh-v09/cooked game:/characters/hound_rig/v09/hound-rig.gltf cache:/hound-rig.gasset 1 D:/Projects/Gloom/.cache/hound-mesh-v09/gloom-preview.ppm
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Release --output-on-failure -R '^gloom[.](assets|gpu_assets)$'
rtk proxy D:/Dev/CMake/bin/ctest.exe --test-dir build/windows-vs -C Debug --output-on-failure -R '^gloom[.]animation_vfx$'
```

Se usó Blender CLI local. MCP no estaba conectado; no se abrió otro puerto
ni se cambió el modo seguro. El sandbox no iniciaba procesos, por lo que las
ejecuciones se revisaron fuera de él. Una interrupción por límite de uso del
revisor se retomó tras el encargo del usuario; no se omitió la validación.

## Traspaso

Fuente acumulada: `art/characters/hound/v09/hound-mesh-v09.blend`.
Exportación: `assets/characters/hound_rig/v09/hound-rig.gltf` y `hound-rig.bin`.
Escena `Hound_Mesh_v09`, malla `H09_DeformMesh`, rig `Hound09_Rig`,
acción `Hound09_joint_check`, colección `HOUND_v09_EXPORT`.
Evidencia: `docs/art/hound/mesh-v09/README.md`.
Las métricas JSON detalladas se regeneran localmente con el verificador;
la regla de `.gitignore` para resultados de informes las excluye del commit.
Los resultados y límites necesarios para el traspaso están resumidos arriba.

H01 hecha con los límites diagnósticos descritos. H02 permanece pendiente y
no se inicia. H05/H08 siguen siendo las puertas de revisión global/rig y agarres.
Commit local del hito 91; resolver con `git log --oneline --grep='^hito 91:'`.
Sin push.
