# Contexto mínimo común — Hound

Leer una vez por tarea junto al índice y la ficha elegida. No hace falta el chat.

## Identidad y decisiones que no se deben reinventar

Hound es un humanoide bípedo robusto de Gloom, no un perro ni un robot cuadrúpedo.
Dirección: fantasía oscura/industria pesada como Unreal Tournament 3, con un toque
de claridad de formas/materiales de Overwatch 2; conservar la identidad original.

Referencias visuales, en este orden:
1. [Concept original elegido](../../characters/hound-original-concept.jpg).
2. [Boceto 01 aprobado](../hound-concept-v01.png).
3. [Diseño 3D v02 aprobado](../blockout-v02/README.md), especialmente hombros/capucha/manos.
4. [Propuesta v08](../mesh-v08/README.md), punto de partida técnico, no nueva aprobación.

Conservar hombros/deltoides libres y filos anclados a clavícula/cuello; capucha
adelantada que oculta parte del rostro; piel ceniza, ojos naranja pequeños,
hierro oscuro y tela carbón/granate. Palmas hacia dentro en reposo, pulgares
bien orientados y placa dorsal que cubre toda la mano y acaba en pico.
Altura visual de referencia: 1,80 m. No cambiar proporciones/silueta aprobadas
sin enseñar la propuesta y pedir aceptación. Sin microdetalle uniforme ni
nuevas armas/clases. La guía ampliada está en [DIRECCION_ARTISTICA.md](../../../DIRECCION_ARTISTICA.md).

## Base conocida, no calidad final certificada

La fuente inicial es `art/characters/hound/v08/hound-mesh-v08.blend`.
Escena `Hound_Mesh_v08`, malla `H08_DeformMesh`, armature `Hound08_Rig`,
acción `Hound08_joint_check`, colección `HOUND_v08_EXPORT`.
20.130 vértices, 39.880 triángulos, 96 grupos de componentes PART, 87 rígidos,
7 materiales/primitivas, 1 mesh/skin, 53 huesos y máximo 2 influencias actuales.
Son datos de autoría, **no presupuestos ni requisitos inmutables de producción**.

V08 refina placas, relieve de brazos/rostro y tela. Quedan carcasas de guanteletes/
grebas, dedos/botas, anatomía facial, ropa/uniones y contactos/agarres del conjunto.
No tiene UVs/texturas finales, controles de producción, rig facial ni clips finales.
El clip de seis segundos solo prueba articulaciones. Hay 43 nombres Legacy,
pero cambian ejes/longitudes/jerarquía: no copiar directamente clips antiguos.

Comprobado en v08: malla, 61 muestras diagnósticas, cuatro poses extra de cuello,
roundtrip glTF en 13 poses y visor Vulkan **estático**. Esto no demuestra todos
los contactos, anatomía correcta ni reproducción animada en el motor.
[Informe preciso de v08](../../../../reports/hound-art-89/README.md), solo si hace falta reproducirlo.

## Trabajo seguro y versión de salida

- Leer AGENTS.md y el inicio de ESTADO_ACTUAL; `git status` antes de editar.
  Todos los comandos de shell llevan prefijo `rtk`.
- La entrada real es la fuente acumulada indicada en el índice. Conservar
  originales y versiones anteriores; no regenerar v08 sobre cambios manuales.
- Crear una carpeta de versión nueva no ocupada, sin asumir que H01 equivale
  a v09. Registrar nombres de escena/malla/rig/acción y rutas en la entrega.
- Carpeta de autoría: `art/characters/hound/<version>/`; exportación:
  `assets/characters/hound_rig/<version>/`; evidencias:
  `docs/art/hound/<revision>/`. No borrar lo previo.
- Durante H01–H05 conservar huesos, bind pose y claves diagnósticas salvo
  impedimento demostrado y autorizado. Ajustar pesos provisionales solo donde
  la geometría nueva lo necesite. Separar rígido/deformable.
- No tocar gameplay, protocolo, cápsula, cámara, movimiento, daño, cooldowns,
  audio/HUD o personajes ajenos. H13 cambia presentación, no esas reglas.
- Blender local basta. No usar generación 3D externa de pago ni habilitar
  proveedores/puertos. No desactivar el modo seguro del MCP.

## Herramientas y pruebas, solo cuando correspondan

Blender disponible al redactar: `.cache/blender/blender-4.5.13-windows-x64/blender.exe`.
Comprobar existencia al empezar; las cachés no se versionan.
Usar MCP disponible o CLI Blender; no abrir otra instancia sobre el mismo puerto.
[BLENDER_WORKFLOW.md](../../../BLENDER_WORKFLOW.md) solo para arranque/exportación.

Salida probada: glTF 2.0 separado + BIN + PNG externos, metros, Blender Z-up,
exportación Y-up. Exportar solo el modelo/rig, sin cámaras/luces de revisión.
No asumir que nodos, constraints, shape keys o materiales del viewport viajan al motor.
H06 fija el contrato concreto; la prueba inicial del puente no lo prueba todo.

Herramientas reutilizables en `tools/art/`: `review_hound_mesh_v06.py`,
`verify_hound_mesh_v06.py`, `verify_hound_rig_roundtrip_v03.py` y
`animate_hound_rig_review_v03.py`. Aceptan v08, **no versiones futuras todavía**.
Adaptarlas mínimamente cuando proceda; revisar sus aserciones específicas antes
de reutilizarlas. No cambiar simplemente los recuentos para esconder una regresión.
Si cambian scripts compartidos, ejecutar también la versión anterior afectada.

Reabrir fuente final, verificar geometría/pesos, exportar/reimportar y comparar
poses cuando se modifica la malla/rig. Inspeccionar renders reales frontal,
perfil, espalda y tres cuartos, detalles afectados y vídeo diagnóstico pertinente.
Indicar qué pares/contactos se comprobaron; no declarar colisión exhaustiva.

Cooker/visor existentes: `build/windows-vs/Release/gloom_asset_cooker.exe` y
`gloom_scene_viewer.exe`. Recetas exactas en el informe v08 y BLENDER_WORKFLOW.
Tests acotados: CTest Release `gloom.assets`/`gloom.gpu_assets`, Debug
`gloom.animation_vfx`; enumerar con `ctest -N` antes de elegir otros.
Si cambia C++, compilar los objetivos afectados antes de probar, respetando AGENTS.
Una pasada solo documental verifica enlaces/consistencia/diff; no necesita Blender/build.

## Cierre autocontenido de cada ficha

Actualizar el índice con estado, rutas de entrada/salida, evidencia, informe,
decisiones/limitaciones y commit; actualizar ESTADO_ACTUAL sin borrar trabajo ajeno.
Informe breve en `reports/<nombre-del-hito>/README.md`: qué se hizo, qué no,
comandos realmente ejecutados/resultados y siguiente entrada exacta.
Elegir el siguiente número de hito libre en el estado/git, no a partir del ID Hxx.
Crear commit local `hito N: resultado`, verificarlo y entregar su hash.
Para registrar ese hash en el índice sin autorreferencia, usar “commit del hito N;
resolver con git log” en el mismo commit; completar hash en la siguiente entrega.
No añadir cachés, logs de build, secretos o copias .blend1. **No hacer push** salvo encargo.
Si falta aprobación, dejar “en revisión” y pedir solo esa decisión, sin fingir cierre.
