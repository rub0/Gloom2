# Aceptación del hito 66

Fuente: checkout legado limpio `fe59e723594cc13e0fc95a1f390d0f81f58dc45b`,
`assets/gameplay/legacy_rules.json` y los componentes originales. Las siluetas y
el comportamiento se contrastaron con el vídeo original fijado por la
restauración artística.

Sniper, ShotGun, MiniGun e IronHellGoat se recuperaron desde sus `.mesh` y se
cocinan junto al Soul Reaper. `weapon-import.log` conserva la conversión offline.
FPS y TPS seleccionan escenas por snapshot.

La aceptación automática cubre tablas, inventario, topes, selección, munición,
cadencias, carga/soltado, retorno magnético, guiado, proyectiles, réplica, daño,
dos clientes, muerte, respawn y reanudación. La batería completa queda en
`tests.log`. La primera batería detectó que la
excepción de pose base de MiniGun había entrado también en Factory. Tras
restringirla al asset de arma, `tests-final.log` aprobó Factory y las otras cinco
regresiones afectadas. La repetición completa `tests-clean.log` aprobó
**43/43 en 217,00 s**.

La reproducción offline obtuvo **93 archivos idénticos, 116 hashes de fuente,
cuatro controles negativos y 19 mallas auditadas** (`asset-verification.log`).
La auditoría de gameplay volvió a confirmar cinco armas, catorce tipos de objeto
y 73 ubicaciones en cada una de las tres variantes de Factory.
