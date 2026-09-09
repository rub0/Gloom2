# Hito 77 — Presentación visual de recogibles

9 de septiembre de 2026. Hito implementado y validado.

## Objetivo

Hacer que los objetos situados en el mapa sean fáciles de reconocer y apetezca
recogerlos, tomando como referencia la lectura visual de los pickups de Quake,
sin alterar la fidelidad funcional del Gloom original.

## Implementación

- Los 73 recogibles oscilan ±8 cm y giran a 0,72 rad/s. La identidad estable del
  mapa distribuye sus fases con el ángulo áureo para evitar movimiento sincronizado.
- Un `PickupPresentation` local detecta el paso de respawning a disponible. Una
  curva smoothstep de 0,45 s lleva escala de 35 % a 100 % y opacidad de cero a
  uno. La primera presentación de los objetos usa la misma entrada visual sin
  retrasar su disponibilidad autoritativa.
- Escudo usa halo azul; armas naranja, munición ámbar, daño rojo y cooldown cian.
  Vida conserva flotación/giro pero omite el halo por decisión del usuario.
- El halo usa un quad de cuatro vértices, una textura radial de 32×32 generada al
  iniciar y un material aditivo compartido. No crea luces, sombras ni la copia de
  color/profundidad reservada a partículas.
- La opacidad temporal fuerza solo esa instancia por el pase blend, conservando
  sus materiales y texturas originales. Al completar la aparición vuelve a su
  pase normal y recupera sombras.

## Límites conservados

- No cambia recompensa, contacto, atracción, disponibilidad, respawn autoritativo
  ni protocolo 22. La posición del snapshot sigue siendo el ancla de cada frame.
- No hay asignación de estado por pickup, tráfico de red, luces dinámicas ni halo
  con sombras. Las instancias adicionales caben en la reserva existente.
- Énfasis por proximidad y destello al recoger quedan fuera; son pulido opcional
  posterior a la revisión humana.

## Validación

Build completa Debug y build del ejecutable Release aprobadas. La suite completa
terminó con **48/48 pruebas**; la nueva `gloom.pickup_presentation` comprueba
límites del bobbing, avance del giro, fases distintas, exclusión de vida y los
tres estados de la interpolación de respawn. Tras extender la misma curva a la
aparición inicial se recompilaron Debug/Release y se repitieron las cuatro pruebas
focales de presentación, recogibles, visibilidad y escena: **4/4 aprobadas**.

Comandos reproducibles:

```powershell
cmake --build build/windows-vs --config Debug --parallel
ctest --test-dir build/windows-vs -C Debug --output-on-failure
cmake --build build/windows-vs --config Release --target gloom --parallel
build/windows-vs/Debug/gloom.exe --pickup-review reports/pickup-presentation-2026-09-09/captures
```

Se inspeccionaron cinco capturas locales: disponible, atracción, recogido,
reaparecido y vida sin halo. Confirman silueta, movimiento entre fases,
seguimiento durante el tirón, desaparición y retorno, además de la excepción
visual de vida. Las imágenes regenerables no se versionan porque `reports/**`
está excluido por `.gitignore`. Queda pendiente la valoración subjetiva del
usuario durante una partida real.
