# Migración de física original — hito 65 completado

Base: hito 64, con sus cambios sin commit conservados. Antes de editar se
ejecutó la batería completa: **41/41 aprobadas en 226,22 s** (`baseline.log`).

## Física recuperada

- Perfiles de Hound, Archangel y Shadow desde `archetypes.txt`.
- Ecuaciones de inercia, frenado, control aéreo, salto, esquiva y caída de
  `AvatarController.cpp`, cuya unidad es desplazamiento por tick de 16 ms.
- Conversión ×0,15 a metros y normalización temporal, conservando los 60 Hz
  de autoridad y predicción actuales.
- Cápsula y escalón originales sobre la colisión Jolt de Factory.
- Doble pulsación WASD en menos de 300 ms; solo desde suelo. Acciones almacenadas
  hasta el siguiente tick, sin repetición por tecla sostenida.
- Bit de esquiva redundante y confirmado en protocolo 15. Perfil determinado
  por la selección autorizada, misma función en cliente y servidor.

Los detalles y las diferencias deliberadas frente a PhysX están en
[GAMEPLAY_MIGRATION.md](../../docs/GAMEPLAY_MIGRATION.md) y
[ADR 0066](../../docs/architecture/0066-original-character-motion.md).
No se afirma una clonación bit a bit del motor PhysX antiguo.

## Pruebas

`gloom.legacy_movement` compara cada paso contra una transcripción independiente
de las ecuaciones originales a 16 ms. Comprueba aceleración/frenado, aire,
gravedad, límite vertical, perfiles, diagonales, respuesta a 30/60/144 Hz,
salto/aterrizaje en Factory y rechazo de tiempo no finito.

La misma prueba descarta el primer datagrama de esquiva, entrega el siguiente
con redundancia y después lo duplica: la autoridad ejecuta una sola esquiva,
confirma los comandos y el cliente reconcilia. Los bits reservados se rechazan.
Comprueba además que una pulsación sobreviva a frames sin tick y se borre al
entrar en un menú.

La batería existente incluye combate y reanudación con dos clientes GNS, servidor
dedicado, flujo de dos ventanas gráficas y capturas de Factory, personajes y UI.
Los logs se conservan en `tests.log`, `movement.log` y `tests-final-input.log`.

## Fuente preparada para arsenal y objetos

La auditoría offline cubre **28 fuentes, cinco armas, catorce tipos de objeto
y 73 ubicaciones por mapa** (standalone, servidor y cliente). Se guarda en
[legacy_rules.json](../../assets/gameplay/legacy_rules.json); la reproducción
está en `source-verification.log`. No ejecuta Lua y no depende de Assimp para
extraer estas tablas. El parser literal se comparte con la importación de Factory.

El hito 66 implementa el arsenal sobre esta auditoría; su aceptación está en
`reports/arsenal-2026-09-05`. El hito 67 conserva los recogibles como siguiente
alcance.

Los 165 archivos de atlas/recursos UI y referencias del hito 64 mantienen sus
hashes (`preservation.log`). Las referencias de hitos anteriores no se han
actualizado. La copia `D:/Projects/Gloom-Legacy` sigue limpia en
`fe59e723594cc13e0fc95a1f390d0f81f58dc45b`.

## Reproducir

```powershell
& D:/Dev/CMake/bin/cmake.exe --build --preset windows-debug
& D:/Dev/CMake/bin/ctest.exe --preset windows-debug --output-on-failure
python tools/legacy/audit_gameplay.py --legacy-root D:/Projects/Gloom-Legacy --output assets/gameplay/legacy_rules.json --verify
.\build\windows-vs\Debug\gloom.exe
```

Elegir juego local y personaje; WASD para movimiento, Espacio para salto y doble
pulsación de la misma tecla WASD para esquiva. El hito 65 introdujo protocolo
15; el arsenal posterior requiere protocolo 16 en ambos pares.
