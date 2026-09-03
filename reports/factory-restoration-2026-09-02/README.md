# Entrega de Factory: hitos 59–61

**33/33 pruebas Debug superadas el 2 de septiembre de 2026.** Se recuperan
Factory y sus objetos estáticos desde las fuentes originales, se amplía el
pipeline de materiales y se comparte la colisión con movimiento, disparos y
predicción. Los cambios previos se conservan y la copia legacy permanece limpia.

## Comparación con el vídeo

Referencia: [Gloom, primeros dos minutos](https://www.youtube.com/watch?v=yJPoulcfIAg).
El montaje alterna Factory, Dungeons y demostraciones de materiales. El
fotograma 0:45 muestra Factory; los interiores de Dungeons no se han usado para
inventar partes de este mapa. La cámara original no está recuperada: las dos
imágenes muestran posiciones distintas y no constituyen un test de igualdad
de píxeles.

![Vídeo original y Factory restaurada](video-comparison.png)

La comparación verifica las pasarelas y pilares originales, suelos industriales
texturados, respuesta especular y lava emisiva naranja. La nueva iluminación
es más cálida y clara que la del vídeo; no se afirma una reproducción exacta
del antiguo shader. El arma gris visible sigue siendo la presentación FPS
provisional del hito 58; su recuperación está prevista en 62.

![Seis cámaras de aceptación](factory-contact.png)

Se inspeccionaron planta, spawn bajo, dos spawns altos, lava y pasarela central.
La vista aérea sale por encima del recinto y puede mostrar el cielo diagnóstico;
las cámaras interiores muestran los recorridos originales.

## Colisión y materiales

![Superposición de malla visual y colisión](collision-overlay.png)

Azul: geometría visual; naranja: RepX transformado; negro: nueve spawns.
La escala 0,15 es común. Los tests comprueban soporte de los nueve spawns,
repetición determinista de movimiento y oclusión de disparos contra paredes.
La barrera de colisión original no siempre coincide con el borde de la malla
visible; no se ha sustituido por cajas aproximadas.

![Controles de materiales](material-contact.png)

Las 14 capturas de materiales prueban anisotropía y rotación, vuelta exacta a
isotropía con intensidad cero, especular, AO, máscara alfa, UV transformadas,
UV1, emisión, transparencia y aditivo. Invertir el orden de envío de objetos
transparentes produce el mismo resultado. Los logs se validan además para
rechazar errores de Vulkan.

## Evidencias reproducibles

| Evidencia | Resultado |
| --- | --- |
| [Suite completa](tests-final.log) | 33/33, 90,31 s |
| [Reproducción offline](import-verification.json) | 80 archivos idénticos, 111 hashes, 4 controles negativos |
| [Regresión Factory](captures/comparison.txt) | 6/6 vistas; error medio máximo 0,901/255 |
| [Render Factory](captures/render.log) | Sin errores de validación Vulkan |
| [Render de materiales](materials/render.log) | Aceptación GPU superada, sin errores de validación |
| Referencias anteriores | 9/9 vistas de hito 58 pasan sin regenerarse |
| Red | Predicción/reconexión de Factory y dos clientes reales GNS; dedicado arranca |

El manifiesto contiene 97 entidades únicas y el glTF 14 mallas estáticas.
La malla principal conserva sus 16 submallas y 16.351 triángulos. Dos
diagnósticos quedan explícitos: una Minigun con rig pendiente de 62 y una
textura glow opcional de IronHellGoat ausente del origen. Los pickups visuales
no se presentan como mecánicas nuevas.

Quedan para 62–64 personajes y arma FPS originales, rigs/animaciones/VFX e
interfaz gráfica. La sonda de reflejos es estática y global. No se ha implementado
ejecución de materiales Unreal ni equivalencia exacta de iluminación con Ogre.
Véase [documentación técnica y comandos](../../docs/FACTORY_RESTORATION.md).
