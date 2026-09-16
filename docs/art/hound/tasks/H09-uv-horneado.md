# H09 — UVs y horneado

## Entrada y alcance
Leer [contexto](CONTEXTO.md) e [índice](README.md); requiere H08 y H06.
Usar la malla/rig estabilizados y la escultura maestra aprobada de H05.
No volver a una versión sin pesos/sockets ya corregidos.

Preparar UVs, costuras, densidad de texel y padding según resolución/mips de H06.
Decidir simetría/solapes de forma explícita; evitar duplicar desgaste distintivo.
Hornear detalle de escultura a la malla de producción con jaula y tangentes
consistentes. Generar normal y demás mapas auxiliares que realmente se utilizarán.
Mantener UVs/horneado coherentes con los LODs y separar material pintado de iluminación.

## Fuera de alcance
Cambios de topología/silueta, animaciones, pintura final o nuevos shaders.
No fijar 4K/8K por intuición ni asumir que un mapa de Blender tiene la convención del motor.

## Comprobaciones y entrega
- Checker de UVs, costuras, solapes declarados, estiramiento y padding a mips.
- Horneado sin rayos cruzados, costuras visibles o normales invertidas; comparar
  escultura y malla con luz neutra y normal map en Gloom.
- Verificar que pesos, sockets y diagnóstico de H08 siguen intactos.
- Fuente, UVs, mapas externos y ajustes de bake reproducibles; registrar resolución,
  canales, color space, convención de normal y licencia/procedencia. Cierre común.

## Criterio de cierre
Detalle relevante transportado sin artefactos a distancias previstas, dentro
del presupuesto H06, con exportación/cocción comprobadas. H10 recibe mapas reales,
no nodos procedurales sin hornear ni capturas del viewport como textura.
