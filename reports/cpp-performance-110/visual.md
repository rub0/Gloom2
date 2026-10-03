# Comparación visual antes/después

Métricas de píxeles a resolución completa (1280×720), sin reducir.

| group | capture | mean_channel_difference | changed_pixels_percent | max_channel_difference |
| --- | --- | --- | --- | --- |
| factory | factory-central-walkway.ppm | 1.5620540364583333 | 79.23014322916667 | 241 |
| factory | factory-lava.ppm | 1.859361617476852 | 80.19010416666667 | 244 |
| factory | factory-overview.ppm | 1.6827094184027778 | 73.73524305555556 | 237 |
| factory | factory-spawn-1.ppm | 1.064759837962963 | 73.00043402777779 | 214 |
| factory | factory-spawn-8.ppm | 0.963236400462963 | 74.0078125 | 170 |
| factory | factory-spawn-9.ppm | 1.330130931712963 | 76.67708333333333 | 157 |
| character | archangel-back.ppm | 3.2552083333333335e-06 | 0.0009765625 | 1 |
| character | archangel-front.ppm | 3.978587962962963e-06 | 0.001193576388888889 | 1 |
| character | shadow-back.ppm | 7.233796296296296e-07 | 0.00021701388888888888 | 1 |
| character | shadow-front.ppm | 2.170138888888889e-06 | 0.0006510416666666667 | 1 |
| character | soul-reaper-down.ppm | 0.0 | 0.0 | 0 |
| character | soul-reaper-forward.ppm | 0.0 | 0.0 | 0 |
| character | soul-reaper-up.ppm | 0.0 | 0.0 | 0 |

## Comparador existente de miniaturas

Factory y personajes pasan sus umbrales sin modificarlos.

El comparador reduce a 160×90 y cuenta como `changed` los píxeles con diferencia
media de canales >32/255. Sus límites son media ≤4/255, `changed` ≤1,5 % y peor
tile ≤12/255. Un `changed=0%` no implica igualdad píxel a píxel; esa comparación
se recoge por separado en la tabla a resolución completa.

### factory

```text
factory-overview.ppm: mean=0.543866/255 changed=0% worst_tile=1.09667/255
factory-spawn-1.ppm: mean=0.471736/255 changed=0% worst_tile=1.13667/255
factory-spawn-8.ppm: mean=0.386528/255 changed=0% worst_tile=0.683333/255
factory-spawn-9.ppm: mean=0.591713/255 changed=0% worst_tile=1.30444/255
factory-lava.ppm: mean=0.692083/255 changed=0% worst_tile=1.79111/255
factory-central-walkway.ppm: mean=0.539282/255 changed=0% worst_tile=1.67111/255
```

### character

```text
archangel-front.ppm: mean=0/255 changed=0% worst_tile=0/255
archangel-back.ppm: mean=0/255 changed=0% worst_tile=0/255
shadow-front.ppm: mean=0/255 changed=0% worst_tile=0/255
shadow-back.ppm: mean=0/255 changed=0% worst_tile=0/255
soul-reaper-forward.ppm: mean=0/255 changed=0% worst_tile=0/255
soul-reaper-up.ppm: mean=0/255 changed=0% worst_tile=0/255
soul-reaper-down.ppm: mean=0/255 changed=0% worst_tile=0/255
```
