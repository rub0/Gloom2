# Pautas del proyecto

- Leer primero `docs/ESTADO_ACTUAL.md` y seguir sus pautas de contexto reducido.
- Conservar los cambios existentes; revisar `git status` antes de editar.
- Al terminar cada hito, ejecutar las comprobaciones pertinentes, actualizar
  `docs/ESTADO_ACTUAL.md` y su informe, y crear un commit local con el hito terminado.
  Esta es una instrucción permanente del usuario: no pedir confirmación de nuevo
  para ese commit. No presentar un hito como cerrado sin haberlo creado, salvo
  bloqueo explícito, que debe comunicarse.
- Usar un mensaje como `hito N: descripción del resultado`. Incluir código,
  recursos necesarios, pruebas y documentación del hito; excluir cambios ajenos,
  secretos y artefactos de build/caché. Respetar `.gitignore`.
- Verificar el commit y el estado del workspace, e indicar el hash al entregar.
  Crear un commit no implica hacer push: subir a GitHub requiere un encargo que
  incluya esa acción.
