# Hito 77 — Presentación visual de recogibles

## Estado

Hito definido y pendiente de implementación. Este documento no implica que se
hayan modificado código, shaders, materiales ni recursos.

## Objetivo

Hacer que los objetos situados en el mapa sean fáciles de reconocer y apetezca
recogerlos, tomando como referencia la lectura visual de los pickups de Quake,
sin alterar la fidelidad funcional del Gloom original.

## Alcance aprobado

- Flotación vertical suave alrededor de la posición original.
- Giro lento continuo sobre el eje vertical.
- Halo aditivo coloreado para todos los recogibles excepto vida.
- Vida sin brillo; puede conservar flotación y giro si la primera revisión visual
  no la hace demasiado llamativa.
- Animación de aparición y respawn mediante escala y opacidad.
- Como extensiones opcionales: énfasis al acercarse y destello breve al recoger.

## Restricciones

- La animación debe ser presentación local y no formar parte del estado
  autoritativo ni del protocolo de red.
- No usar luces dinámicas que proyecten sombras por pickup.
- Anclar la animación al transform original para evitar deriva y conservar el
  culling/visibilidad existente.
- Mantener bajo el coste de transparencias, partículas y overdraw.

## Aceptación prevista

Capturas deterministas y revisión jugable comprobarán fase independiente del
framerate, límites de desplazamiento, giro continuo, colores por arquetipo,
ausencia de halo en vida, aparición/respawn mediante escala y opacidad y
desaparición al recoger. Después se decidirá si se incluyen las extensiones
opcionales.
