# Escaleras, momento y salto

Se corrigen las observaciones posteriores a la revisión del HUD, conservando el trabajo previo.

- Se restaura la deceleración original: retención 0,8 por 16 ms en suelo sin entrada y 0,98 en vuelo, normalizadas al tiempo del tick. Al aterrizar una esquiva se conserva el momento y se frena gradualmente.
- La consulta temporal CharacterVirtual recupera sus contactos antes de ExtendedUpdate. Este necesita conocer el apoyo inicial para ejecutar StickToFloor al descender. No se añade un salto en el aire ni se amplía artificialmente el área de suelo.
- Los contactos de superficies transitables dejan de proyectar repetidamente el momento horizontal. Jolt sigue resolviendo el desplazamiento sobre la geometría; paredes y techos siguen cancelando el impulso hacia el obstáculo. Se ignoran contactos descartados y sensores.
- El impulso descendente constante en suelo se sustituye por apoyo y seguimiento del suelo de Jolt. Así, tras la deceleración, el personaje queda quieto sobre una pendiente.
- Impulso del salto normal reducido al 90 % en todos los personajes: Hound 15,1875 m/s, Archangel 11,8125 m/s, Shadow 12,65625 m/s, antes de integrar gravedad. La fuerza de esquiva no cambia.
- La huella de simulación de Factory cambia para impedir mezclar clientes con las reglas anteriores. Formato de protocolo sin cambios.

## Validación

Compilados motor, juego, servidor y ejecutables de prueba afectados (`build-engine.log`, `build.log`). **7/7 pruebas aprobadas en 17,04 s** (`tests.log`): física Jolt, movimiento original, replicación, simulación de partida, red de partida, transporte y smoke gráfico jugable. No se repite la suite gráfica completa porque esta revisión no modifica render ni HUD.

Regresiones sobre la geometría real de la escalera de Factory, cuyo collider es una rampa entre aproximadamente (-44,-0,3,-29) y (-24,7,-29):

- Subir durante 90 ticks, manteniendo más de 10 m/s de momento horizontal después de acelerar; avanzar más de 10 m y subir más de 3 m.
- Bajar durante 60 ticks y comprobar en cada uno que sigue apoyado y admite un salto.
- Al soltar movimiento durante la bajada, conservar velocidad reducida progresivamente; tras frenar, mantener posición durante 600 ticks adicionales.
- Comparación con las fórmulas originales de 16 ms para los tres perfiles, respuesta a 30/60/144 Hz, salto reducido un 10 %, doble Espacio y momento al aterrizar la esquiva.
- Sigue pasando la regresión de techo: cancela velocidad ascendente, conserva la tangencial y desciende en el siguiente tick.

La verificación de movimiento fue automatizada sobre el mapa real; no se afirma una sesión manual de recorrido. `diff-check.log` no presenta errores de espacios en código/documentación, respetando CRLF.
