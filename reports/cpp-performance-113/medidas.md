# Medidas completas del hito 113

Windows x64, Ryzen 7 3700X/GTX 1070, driver 581.29, MSVC 14.44/VS 2022. Misma carga del 112; 1080p nativo, Immediate, 120 frames listos de calentamiento + 360 medidos, Hound v17 LOD0, audio null/SDL. Series seriales sin builds/tests simultáneos. La base fue capturada el 3 de octubre y el después el 4; no se afirma mejora de FPS a partir de diferencias pequeñas entre días.

La preparación de skin ahora está dentro de poses; antes se medía junto a bounds. Se compara la suma de ambas etapas de aplicación, no la suma de scopes inclusivos del profiler.

## Serie before-normal

Exe SHA256: `e70fa7235b6c1c4abebe8ec695baa6983ca16ea2cc33fe2740431a59c0b9e0c3`. Diagnóstico: False.
HEAD de entrada: `268db47042853bc4405a68766366dad81831045d`; diff de la medición: `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`. Después se mide workspace del 113 aún sin commit; los diffs pueden incluir documentación en preparación.

| Pasada | Frame medio ms | FPS | p50 | p95 | p99 | Máximo | >5 ms | Poses | Bounds | Suma CPU |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| factory-1 | 2.400 | 416.66 | 2.370 | 2.555 | 2.616 | 3.956 | 0 | 0.119 | 0.018 | 0.137 |
| factory-2 | 2.352 | 425.18 | 2.350 | 2.373 | 2.487 | 2.508 | 0 | 0.118 | 0.020 | 0.138 |
| factory-3 | 2.359 | 423.97 | 2.353 | 2.400 | 2.472 | 2.532 | 0 | 0.125 | 0.018 | 0.143 |
| hound-null-1 | 3.297 | 303.34 | 3.278 | 3.511 | 3.526 | 3.550 | 0 | 0.612 | 0.103 | 0.715 |
| hound-null-2 | 3.391 | 294.88 | 3.466 | 3.740 | 3.814 | 3.849 | 0 | 0.534 | 0.121 | 0.655 |
| hound-audio-1 | 3.396 | 294.49 | 3.466 | 3.802 | 3.835 | 3.973 | 0 | 0.521 | 0.096 | 0.617 |
| hound-audio-2 | 3.282 | 304.70 | 3.276 | 3.510 | 3.524 | 3.579 | 0 | 0.518 | 0.096 | 0.614 |

| Pasada | Etapas CPU restantes y GPU | Proceso y uploads/residencia |
| --- | --- | --- |
| factory-1 | CPU stages ms: update=0.153 begin=0.027 presentation=0.180 visibility=0.028 lighting=0.019 draw=0.470 end_present=1.524; GPU full render: samples=359 mean=2.093 p50=2.050 p95=2.238 p99=2.253 max=2.271 ms | Process memory: private=1088.47 MiB working_set=569.70 MiB peak_working_set=674.67 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| factory-2 | CPU stages ms: update=0.140 begin=0.018 presentation=0.179 visibility=0.028 lighting=0.019 draw=0.457 end_present=1.511; GPU full render: samples=359 mean=2.060 p50=2.060 p95=2.067 p99=2.072 max=2.076 ms | Process memory: private=1097.11 MiB working_set=573.87 MiB peak_working_set=683.40 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| factory-3 | CPU stages ms: update=0.163 begin=0.023 presentation=0.188 visibility=0.029 lighting=0.019 draw=0.492 end_present=1.445; GPU full render: samples=359 mean=2.057 p50=2.056 p95=2.063 p99=2.068 max=2.218 ms | Process memory: private=1090.10 MiB working_set=571.51 MiB peak_working_set=676.29 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| hound-null-1 | CPU stages ms: update=0.241 begin=0.020 presentation=0.767 visibility=0.033 lighting=0.018 draw=0.710 end_present=1.507; GPU full render: samples=359 mean=2.991 p50=2.983 p95=3.213 p99=3.222 max=3.231 ms | Process memory: private=1772.27 MiB working_set=992.99 MiB peak_working_set=1324.70 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-null-2 | CPU stages ms: update=0.239 begin=0.020 presentation=0.716 visibility=0.041 lighting=0.019 draw=0.734 end_present=1.623; GPU full render: samples=359 mean=3.077 p50=2.990 p95=3.443 p99=3.552 max=3.569 ms | Process memory: private=1772.78 MiB working_set=991.53 MiB peak_working_set=1324.91 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-audio-1 | CPU stages ms: update=0.226 begin=0.019 presentation=0.661 visibility=0.030 lighting=0.017 draw=0.681 end_present=1.761; GPU full render: samples=359 mean=3.089 p50=2.984 p95=3.531 p99=3.567 max=3.573 ms | Process memory: private=1787.05 MiB working_set=991.80 MiB peak_working_set=1326.11 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-audio-2 | CPU stages ms: update=0.225 begin=0.018 presentation=0.660 visibility=0.031 lighting=0.018 draw=0.677 end_present=1.654; GPU full render: samples=359 mean=2.986 p50=2.978 p95=3.209 p99=3.215 max=3.225 ms | Process memory: private=1774.15 MiB working_set=991.93 MiB peak_working_set=1325.71 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |

## Serie after-normal

Exe SHA256: `658da5476d8a730f7710732241e7d3982e6a8e148c4c63050682c388a4662691`. Diagnóstico: False.
HEAD de entrada: `268db47042853bc4405a68766366dad81831045d`; diff de la medición: `6b0fc1b6d7800a5beb18355cf01268a14c5db073627f32ee5b157a67a82a6991`. Después se mide workspace del 113 aún sin commit; los diffs pueden incluir documentación en preparación.

| Pasada | Frame medio ms | FPS | p50 | p95 | p99 | Máximo | >5 ms | Poses | Bounds | Suma CPU |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| factory-1 | 2.342 | 427.01 | 2.336 | 2.358 | 2.381 | 4.340 | 0 | 0.076 | 0.004 | 0.080 |
| factory-2 | 2.342 | 427.01 | 2.340 | 2.372 | 2.407 | 2.476 | 0 | 0.079 | 0.004 | 0.083 |
| factory-3 | 2.340 | 427.34 | 2.338 | 2.368 | 2.465 | 2.586 | 0 | 0.079 | 0.004 | 0.083 |
| hound-null-1 | 3.271 | 305.68 | 3.269 | 3.503 | 3.528 | 3.584 | 0 | 0.517 | 0.052 | 0.569 |
| hound-null-2 | 3.276 | 305.25 | 3.267 | 3.501 | 3.555 | 3.654 | 0 | 0.521 | 0.053 | 0.574 |
| hound-audio-1 | 3.272 | 305.59 | 3.270 | 3.505 | 3.559 | 3.967 | 0 | 0.509 | 0.051 | 0.560 |
| hound-audio-2 | 3.280 | 304.90 | 3.270 | 3.502 | 3.517 | 3.547 | 0 | 0.497 | 0.052 | 0.549 |

| Pasada | Etapas CPU restantes y GPU | Proceso y uploads/residencia |
| --- | --- | --- |
| factory-1 | CPU stages ms: update=0.149 begin=0.029 presentation=0.110 visibility=0.027 lighting=0.019 draw=0.462 end_present=1.546; GPU full render: samples=359 mean=2.043 p50=2.044 p95=2.051 p99=2.067 max=2.076 ms | Process memory: private=1094.55 MiB working_set=571.71 MiB peak_working_set=670.92 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| factory-2 | CPU stages ms: update=0.153 begin=0.020 presentation=0.113 visibility=0.028 lighting=0.019 draw=0.469 end_present=1.540; GPU full render: samples=359 mean=2.046 p50=2.046 p95=2.058 p99=2.067 max=2.077 ms | Process memory: private=1093.61 MiB working_set=571.72 MiB peak_working_set=677.87 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| factory-3 | CPU stages ms: update=0.155 begin=0.020 presentation=0.114 visibility=0.027 lighting=0.019 draw=0.468 end_present=1.538; GPU full render: samples=359 mean=2.046 p50=2.046 p95=2.056 p99=2.071 max=2.077 ms | Process memory: private=1094.39 MiB working_set=571.06 MiB peak_working_set=676.85 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| hound-null-1 | CPU stages ms: update=0.238 begin=0.022 presentation=0.601 visibility=0.033 lighting=0.018 draw=0.719 end_present=1.640; GPU full render: samples=359 mean=2.977 p50=2.974 p95=3.205 p99=3.224 max=3.226 ms | Process memory: private=1768.13 MiB working_set=993.76 MiB peak_working_set=1327.18 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-null-2 | CPU stages ms: update=0.246 begin=0.021 presentation=0.606 visibility=0.034 lighting=0.019 draw=0.724 end_present=1.626; GPU full render: samples=359 mean=2.983 p50=2.973 p95=3.204 p99=3.224 max=3.265 ms | Process memory: private=1770.48 MiB working_set=993.54 MiB peak_working_set=1327.32 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-audio-1 | CPU stages ms: update=0.240 begin=0.020 presentation=0.592 visibility=0.031 lighting=0.018 draw=0.702 end_present=1.669; GPU full render: samples=359 mean=2.977 p50=2.973 p95=3.205 p99=3.219 max=3.224 ms | Process memory: private=1768.91 MiB working_set=995.54 MiB peak_working_set=1328.47 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-audio-2 | CPU stages ms: update=0.238 begin=0.019 presentation=0.579 visibility=0.031 lighting=0.018 draw=0.702 end_present=1.692; GPU full render: samples=359 mean=2.985 p50=2.973 p95=3.203 p99=3.213 max=3.248 ms | Process memory: private=1777.43 MiB working_set=997.95 MiB peak_working_set=1330.53 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |

## Serie profile-final

Exe SHA256: `e07756cf0430a4f1c6f94a9fb49c6e5c0c5559abc659644f60af24ab24d534f4`. Diagnóstico: True.
HEAD de entrada: `268db47042853bc4405a68766366dad81831045d`; diff de la medición: `6e7d4669cde3317d05fddd30981504c0c78a3181b058c3a247a4437566f4f6ad`. Después se mide workspace del 113 aún sin commit; los diffs pueden incluir documentación en preparación.

| Pasada | Frame medio ms | FPS | p50 | p95 | p99 | Máximo | >5 ms | Poses | Bounds | Suma CPU |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| factory-1 | 2.351 | 425.34 | 2.350 | 2.380 | 2.413 | 2.444 | 0 | 0.079 | 0.004 | 0.083 |
| factory-2 | 2.348 | 425.97 | 2.345 | 2.386 | 2.426 | 2.522 | 0 | 0.078 | 0.004 | 0.082 |
| factory-3 | 2.366 | 422.68 | 2.359 | 2.415 | 2.479 | 2.568 | 0 | 0.089 | 0.004 | 0.093 |
| hound-null-1 | 3.291 | 303.90 | 3.283 | 3.521 | 3.561 | 3.773 | 0 | 0.533 | 0.055 | 0.588 |
| hound-null-2 | 3.288 | 304.10 | 3.284 | 3.521 | 3.582 | 3.993 | 0 | 0.527 | 0.054 | 0.581 |
| hound-audio-1 | 3.288 | 304.17 | 3.280 | 3.522 | 3.578 | 4.234 | 0 | 0.504 | 0.054 | 0.558 |
| hound-audio-2 | 3.284 | 304.49 | 3.281 | 3.514 | 3.542 | 3.569 | 0 | 0.523 | 0.054 | 0.577 |

| Pasada | Etapas CPU restantes y GPU | Proceso y uploads/residencia |
| --- | --- | --- |
| factory-1 | CPU stages ms: update=0.153 begin=0.020 presentation=0.113 visibility=0.028 lighting=0.019 draw=0.468 end_present=1.551; GPU full render: samples=359 mean=2.053 p50=2.054 p95=2.062 p99=2.079 max=2.088 ms | Process memory: private=1088.12 MiB working_set=570.83 MiB peak_working_set=683.53 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| factory-2 | CPU stages ms: update=0.153 begin=0.020 presentation=0.113 visibility=0.029 lighting=0.019 draw=0.473 end_present=1.540; GPU full render: samples=359 mean=2.051 p50=2.051 p95=2.062 p99=2.073 max=2.077 ms | Process memory: private=1076.68 MiB working_set=571.81 MiB peak_working_set=701.43 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13 |
| factory-3 | CPU stages ms: update=0.183 begin=0.026 presentation=0.129 visibility=0.031 lighting=0.020 draw=0.520 end_present=1.456; GPU full render: samples=359 mean=2.055 p50=2.055 p95=2.070 p99=2.082 max=2.135 ms | Process memory: private=1088.73 MiB working_set=570.20 MiB peak_working_set=672.97 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| hound-null-1 | CPU stages ms: update=0.257 begin=0.022 presentation=0.622 visibility=0.035 lighting=0.019 draw=0.743 end_present=1.593; GPU full render: samples=359 mean=2.990 p50=2.985 p95=3.219 p99=3.242 max=3.267 ms | Process memory: private=1775.30 MiB working_set=994.30 MiB peak_working_set=1327.19 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-null-2 | CPU stages ms: update=0.253 begin=0.022 presentation=0.615 visibility=0.035 lighting=0.018 draw=0.741 end_present=1.603; GPU full render: samples=359 mean=2.992 p50=2.986 p95=3.217 p99=3.228 max=3.238 ms | Process memory: private=1775.02 MiB working_set=993.32 MiB peak_working_set=1327.63 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-audio-1 | CPU stages ms: update=0.243 begin=0.021 presentation=0.587 visibility=0.031 lighting=0.018 draw=0.715 end_present=1.673; GPU full render: samples=359 mean=2.989 p50=2.981 p95=3.213 p99=3.234 max=3.258 ms | Process memory: private=1792.24 MiB working_set=997.05 MiB peak_working_set=1317.12 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=31 |
| hound-audio-2 | CPU stages ms: update=0.266 begin=0.022 presentation=0.609 visibility=0.032 lighting=0.018 draw=0.732 end_present=1.604; GPU full render: samples=359 mean=2.986 p50=2.981 p95=3.210 p99=3.218 max=3.223 ms | Process memory: private=1776.61 MiB working_set=997.91 MiB peak_working_set=1330.74 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |

## Serie normal-final

Exe SHA256: `023efa580503148f9ced483fd8a777ea48137e6373ec2e35ea73c18969a78046`. Diagnóstico: False.
HEAD de entrada: `268db47042853bc4405a68766366dad81831045d`; diff de la medición: `aa98f7183d6bba786face617f9828fd7fd6a56c08c90c504942277995d447397`. Después se mide workspace del 113 aún sin commit; los diffs pueden incluir documentación en preparación.

| Pasada | Frame medio ms | FPS | p50 | p95 | p99 | Máximo | >5 ms | Poses | Bounds | Suma CPU |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| factory-1 | 2.341 | 427.14 | 2.339 | 2.369 | 2.407 | 2.454 | 0 | 0.077 | 0.004 | 0.081 |
| factory-2 | 2.340 | 427.28 | 2.338 | 2.374 | 2.413 | 2.471 | 0 | 0.075 | 0.003 | 0.078 |
| factory-3 | 2.355 | 424.67 | 2.349 | 2.405 | 2.469 | 2.513 | 0 | 0.084 | 0.004 | 0.088 |
| hound-null-1 | 3.278 | 305.08 | 3.270 | 3.512 | 3.535 | 3.689 | 0 | 0.542 | 0.052 | 0.594 |
| hound-null-2 | 3.229 | 309.70 | 3.267 | 3.502 | 3.531 | 3.964 | 0 | 0.533 | 0.052 | 0.585 |
| hound-audio-1 | 3.282 | 304.73 | 3.271 | 3.519 | 3.740 | 4.062 | 0 | 0.524 | 0.052 | 0.576 |
| hound-audio-2 | 3.275 | 305.36 | 3.272 | 3.506 | 3.548 | 4.125 | 0 | 0.510 | 0.051 | 0.561 |

| Pasada | Etapas CPU restantes y GPU | Proceso y uploads/residencia |
| --- | --- | --- |
| factory-1 | CPU stages ms: update=0.157 begin=0.022 presentation=0.112 visibility=0.027 lighting=0.020 draw=0.469 end_present=1.534; GPU full render: samples=359 mean=2.045 p50=2.045 p95=2.055 p99=2.076 max=2.085 ms | Process memory: private=1095.71 MiB working_set=571.53 MiB peak_working_set=678.51 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| factory-2 | CPU stages ms: update=0.146 begin=0.019 presentation=0.110 visibility=0.026 lighting=0.019 draw=0.456 end_present=1.563; GPU full render: samples=359 mean=2.044 p50=2.044 p95=2.054 p99=2.070 max=2.076 ms | Process memory: private=1087.40 MiB working_set=570.45 MiB peak_working_set=675.77 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| factory-3 | CPU stages ms: update=0.169 begin=0.025 presentation=0.122 visibility=0.029 lighting=0.019 draw=0.497 end_present=1.493; GPU full render: samples=359 mean=2.046 p50=2.046 p95=2.056 p99=2.067 max=2.077 ms | Process memory: private=1090.50 MiB working_set=571.32 MiB peak_working_set=673.04 MiB; Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244; GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12 |
| hound-null-1 | CPU stages ms: update=0.252 begin=0.022 presentation=0.632 visibility=0.038 lighting=0.019 draw=0.745 end_present=1.570; GPU full render: samples=359 mean=2.979 p50=2.971 p95=3.205 p99=3.222 max=3.232 ms | Process memory: private=1770.97 MiB working_set=993.82 MiB peak_working_set=1327.76 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-null-2 | CPU stages ms: update=0.246 begin=0.022 presentation=0.623 visibility=0.039 lighting=0.019 draw=0.743 end_present=1.537; GPU full render: samples=359 mean=2.931 p50=2.969 p95=3.201 p99=3.211 max=3.227 ms | Process memory: private=1814.93 MiB working_set=999.10 MiB peak_working_set=1283.53 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-audio-1 | CPU stages ms: update=0.247 begin=0.022 presentation=0.607 visibility=0.031 lighting=0.018 draw=0.724 end_present=1.632; GPU full render: samples=359 mean=2.980 p50=2.970 p95=3.203 p99=3.218 max=3.449 ms | Process memory: private=1774.70 MiB working_set=996.03 MiB peak_working_set=1328.88 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |
| hound-audio-2 | CPU stages ms: update=0.244 begin=0.022 presentation=0.593 visibility=0.031 lighting=0.018 draw=0.716 end_present=1.652; GPU full render: samples=359 mean=2.974 p50=2.972 p95=3.204 p99=3.219 max=3.235 ms | Process memory: private=1775.30 MiB working_set=994.97 MiB peak_working_set=1328.86 MiB; Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464; GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30 |

## Interpretación

Factory: mediana poses + bounds **0.1380 → 0.0810 ms (41.3 % menos)**; mediana del frame medio 2.3590 → 2.3410 ms.
Hound: mediana poses + bounds **0.6360 → 0.5805 ms (8.7 % menos)**; mediana del frame medio 3.3440 → 3.2765 ms.

Se conserva la primera serie después y la normal final, junto con las distribuciones completas. El descenso de CPU de animación es consistente; los cambios pequeños del frame total están dentro de la variación de las series base y no prueban una mejora general de FPS. H06 se revisa por p99/máximo y memoria en sus pasadas, sin excluir outliers ni extender una pasada aprobada a toda la aplicación.

## Asignaciones instrumentadas

Base del 112: poses Factory 210 llamadas/93.728 bytes solicitados por frame, Hound 710/419.528. En las siete pasadas después, poses y bounds dan cero durante 360 frames. La etiqueta snapshots también queda a cero; no demuestra migración de red/snapshots: antes varias copias internas del animador ocurrían fuera del scope de las funciones de poses, dentro del scope de presentación/snapshots del caller. Ahora el scope cubre el update completo.

| Pasada | Fase | Calls/360 | Bytes/360 | Mayor request | Scopes | ms inclusivos acumulados |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| factory-1 | other | 15187 | 881066 | 8231 | 0 | 0.000000 |
| factory-1 | poses | 0 | 0 | 0 | 720 | 28.150400 |
| factory-1 | bounds | 0 | 0 | 0 | 720 | 1.162000 |
| factory-1 | jobs | 1080 | 123840 | 200 | 2518 | 6.266000 |
| factory-1 | particles | 360 | 8269992 | 25095 | 720 | 2.448800 |
| factory-1 | entities | 0 | 0 | 0 | 10800 | 0.994300 |
| factory-1 | snapshots | 0 | 0 | 0 | 360 | 40.577700 |
| factory-1 | ui | 0 | 0 | 0 | 0 | 0.000000 |
| factory-1 | assets | 0 | 0 | 0 | 4680 | 0.378800 |
| factory-1 | renderer | 13680 | 1313280 | 336 | 1080 | 733.884300 |
| factory-1 | lighting | 0 | 0 | 0 | 360 | 6.575000 |
| factory-1 | visibility | 0 | 0 | 0 | 360 | 9.838700 |
| factory-2 | other | 15187 | 881066 | 8231 | 0 | 0.000000 |
| factory-2 | poses | 0 | 0 | 0 | 720 | 27.940500 |
| factory-2 | bounds | 0 | 0 | 0 | 720 | 1.154700 |
| factory-2 | jobs | 1080 | 123840 | 200 | 2520 | 7.079500 |
| factory-2 | particles | 360 | 8269992 | 25095 | 720 | 2.600600 |
| factory-2 | entities | 0 | 0 | 0 | 10800 | 0.966700 |
| factory-2 | snapshots | 0 | 0 | 0 | 360 | 40.817100 |
| factory-2 | ui | 0 | 0 | 0 | 0 | 0.000000 |
| factory-2 | assets | 0 | 0 | 0 | 4680 | 0.394700 |
| factory-2 | renderer | 13680 | 1313280 | 336 | 1080 | 731.613400 |
| factory-2 | lighting | 0 | 0 | 0 | 360 | 6.801800 |
| factory-2 | visibility | 0 | 0 | 0 | 360 | 10.373700 |
| factory-3 | other | 15187 | 881066 | 8231 | 0 | 0.000000 |
| factory-3 | poses | 0 | 0 | 0 | 720 | 31.822100 |
| factory-3 | bounds | 0 | 0 | 0 | 720 | 1.246800 |
| factory-3 | jobs | 1080 | 123840 | 200 | 2519 | 7.517200 |
| factory-3 | particles | 360 | 8269992 | 25095 | 720 | 3.137500 |
| factory-3 | entities | 0 | 0 | 0 | 10800 | 1.253100 |
| factory-3 | snapshots | 0 | 0 | 0 | 360 | 46.391100 |
| factory-3 | ui | 0 | 0 | 0 | 0 | 0.000000 |
| factory-3 | assets | 0 | 0 | 0 | 4680 | 0.785700 |
| factory-3 | renderer | 13680 | 1313280 | 336 | 1080 | 720.614700 |
| factory-3 | lighting | 0 | 0 | 0 | 360 | 7.047500 |
| factory-3 | visibility | 0 | 0 | 0 | 360 | 11.078300 |
| hound-null-1 | other | 15295 | 885338 | 8231 | 0 | 0.000000 |
| hound-null-1 | poses | 0 | 0 | 0 | 2880 | 191.395600 |
| hound-null-1 | bounds | 0 | 0 | 0 | 19080 | 15.934100 |
| hound-null-1 | jobs | 1440 | 149760 | 200 | 3233 | 7.516600 |
| hound-null-1 | particles | 360 | 6967080 | 21615 | 720 | 2.115600 |
| hound-null-1 | entities | 0 | 0 | 0 | 10872 | 1.107500 |
| hound-null-1 | snapshots | 0 | 0 | 0 | 360 | 223.700200 |
| hound-null-1 | ui | 1080 | 40320 | 48 | 360 | 12.655400 |
| hound-null-1 | assets | 0 | 0 | 0 | 4680 | 0.557600 |
| hound-null-1 | renderer | 15480 | 1391616 | 336 | 1080 | 848.699800 |
| hound-null-1 | lighting | 0 | 0 | 0 | 360 | 6.654600 |
| hound-null-1 | visibility | 0 | 0 | 0 | 360 | 12.380800 |
| hound-null-2 | other | 15295 | 885338 | 8231 | 0 | 0.000000 |
| hound-null-2 | poses | 0 | 0 | 0 | 2880 | 189.188800 |
| hound-null-2 | bounds | 0 | 0 | 0 | 19080 | 15.988400 |
| hound-null-2 | jobs | 1440 | 149760 | 200 | 3232 | 7.630900 |
| hound-null-2 | particles | 360 | 6967080 | 21615 | 720 | 2.137100 |
| hound-null-2 | entities | 0 | 0 | 0 | 10872 | 1.083000 |
| hound-null-2 | snapshots | 0 | 0 | 0 | 360 | 221.386900 |
| hound-null-2 | ui | 1080 | 40320 | 48 | 360 | 12.751600 |
| hound-null-2 | assets | 0 | 0 | 0 | 4680 | 0.538400 |
| hound-null-2 | renderer | 15480 | 1391616 | 336 | 1080 | 851.794300 |
| hound-null-2 | lighting | 0 | 0 | 0 | 360 | 6.497300 |
| hound-null-2 | visibility | 0 | 0 | 0 | 360 | 12.418500 |
| hound-audio-1 | other | 15295 | 885338 | 8231 | 0 | 0.000000 |
| hound-audio-1 | poses | 0 | 0 | 0 | 2880 | 181.141900 |
| hound-audio-1 | bounds | 0 | 0 | 0 | 19080 | 15.963500 |
| hound-audio-1 | jobs | 1440 | 149760 | 200 | 3232 | 6.385200 |
| hound-audio-1 | particles | 360 | 6967080 | 21615 | 720 | 1.919800 |
| hound-audio-1 | entities | 0 | 0 | 0 | 10872 | 1.034100 |
| hound-audio-1 | snapshots | 0 | 0 | 0 | 360 | 211.417000 |
| hound-audio-1 | ui | 1080 | 40320 | 48 | 360 | 10.687200 |
| hound-audio-1 | assets | 0 | 0 | 0 | 4680 | 0.459100 |
| hound-audio-1 | renderer | 15480 | 1391616 | 336 | 1080 | 866.796300 |
| hound-audio-1 | lighting | 0 | 0 | 0 | 360 | 6.369600 |
| hound-audio-1 | visibility | 0 | 0 | 0 | 360 | 10.937800 |
| hound-audio-2 | other | 15295 | 885338 | 8231 | 0 | 0.000000 |
| hound-audio-2 | poses | 0 | 0 | 0 | 2880 | 187.975400 |
| hound-audio-2 | bounds | 0 | 0 | 0 | 19080 | 16.046100 |
| hound-audio-2 | jobs | 1440 | 149760 | 200 | 3234 | 6.777300 |
| hound-audio-2 | particles | 360 | 6967080 | 21615 | 720 | 2.123000 |
| hound-audio-2 | entities | 0 | 0 | 0 | 10872 | 1.108800 |
| hound-audio-2 | snapshots | 0 | 0 | 0 | 360 | 219.164700 |
| hound-audio-2 | ui | 1080 | 40320 | 48 | 360 | 11.267100 |
| hound-audio-2 | assets | 0 | 0 | 0 | 4680 | 0.437600 |
| hound-audio-2 | renderer | 15480 | 1391616 | 336 | 1080 | 848.966900 |
| hound-audio-2 | lighting | 0 | 0 | 0 | 360 | 6.476200 |
| hound-audio-2 | visibility | 0 | 0 | 0 | 360 | 11.391000 |

Solo se intercepta new/new[] ordinario del ejecutable, incluidas bibliotecas estáticas; se excluyen aligned new, DLL, malloc/realloc directos y memoria GPU. Son solicitudes, no memoria viva. Los scopes son inclusivos y se solapan: no sumarlos para calcular CPU de frame. Reservar al enlazar es coste de carga, medido aparte por el control positivo.

La primera tentativa de serie de diagnóstico fue rechazada porque el ejecutable no contenía el marcador del profiler; se reconstruyó y se ejecutaron las siete pasadas válidas de profile-final. No se usa esa tentativa como diagnóstico ni se mezcla con tiempos normales. La medición instrumentada precede a vaciar paletas desactivadas en bind; esa operación es solo de carga. Las pruebas finales recompiladas cubren el cambio de enlace y mantienen cero en la ventana de evaluación.

GPU conservado: Factory maps=3/copied=16.512 bytes/frame; Hound maps=9/copied=81.408, 53 instancias skinned visibles, siete TPS, máscaras 127/31 y BC5/BC7. Residencia Hound 345,26 MiB; cero evictions y frames limitados en el intervalo medido. El contador acumulado de budget_limited_frames incluye carga, como en el 112. No se anuncia reducción de RAM de proceso; tablas anteriores muestran reservas y picos observados.

## Reproducir las medidas

```text
rtk proxy cmake --preset windows -DGLOOM_ALLOCATION_PROFILE=OFF
rtk proxy cmake --build --preset windows-release --parallel 4
rtk proxy python tools/perf/measure_cpp_baseline.py --output .cache/hito113/normal-final
rtk proxy cmake --preset windows -DGLOOM_ALLOCATION_PROFILE=ON
rtk proxy cmake --build --preset windows-release --target gloom --parallel 4
rtk proxy python tools/perf/measure_cpp_baseline.py --profile --output .cache/hito113/profile-final
rtk proxy cmake --preset windows -DGLOOM_ALLOCATION_PROFILE=OFF
rtk proxy cmake --build --preset windows-release --parallel 4
```

Esperar a que termine cada configuración/build antes de la operación dependiente. No ejecutar pruebas, capturas o builds durante la serie de timings. El juego final queda normal, sin marcador Allocation profile.
