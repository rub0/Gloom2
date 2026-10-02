# Inventario H07 / v17

Fuente: 106 componentes; 53 huesos, máximo dos influencias. Siete primitivas TPS y cuatro FPS.
Los LODs son los índices reales del cooker; comparten vértices de autoría. Cada LOD/FPS tiene una copia GPU del buffer de su material.
V cocinados por pieza incluye splits de material/normal/UV; bytes incluyen copias e índices, sin alineación del driver.

| Material | V cocinados | T LOD0 | T LOD1 | T LOD2 | T FPS | Bytes geométricos | D LOD1/2 (m) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Hound17_Undersuit.001 | 5429 | 7683 | 3840 | 1535 | 4352 | 2467384 | 25.138 / 67.033 |
| Hound17_BurgundyCloth.001 | 2498 | 4326 | 2162 | 865 | 0 | 867612 | 10.038 / 26.769 |
| Hound17_AshSkin.001 | 4099 | 7186 | 3592 | 1436 | 3132 | 1889336 | 18.691 / 49.842 |
| Hound17_Iron.001 | 11981 | 13873 | 6936 | 2774 | 5480 | 5332852 | 27.655 / 73.746 |
| Hound17_EdgePlanes.001 | 4354 | 4106 | 2053 | 1173 | 870 | 1909688 | 24.075 / 64.200 |
| Hound17_Hood.001 | 1205 | 2054 | 1026 | 410 | 0 | 417840 | 7.962 / 21.232 |
| Hound17_EyeAccent.001 | 179 | 276 | 138 | 54 | 0 | 61464 | 1.554 / 4.143 |

Distancias nominales distancia/radio 30/80 con los bounds de reposo. La selección real usa los bounds de la instancia.
Siete materiales OPAQUE; UV0 y factores de diagnóstico, sin imágenes finales. Geometría total: 12946176 bytes.

| Componente | V v16→v17 | T v16→v17 | V cocinados | T LOD0/1/2/FPS | Bytes GPU | Frontera / destino del detalle |
| --- | ---: | ---: | ---: | --- | ---: | --- |
| Torso_underlayer | 624→624 | 1244→1244 | 669 | 1244/430/202/0 | 300816 | Deformable; loops conservados; geometría de silueta |
| Collar_blade_inner_L | 26→26 | 48→48 | 104 | 48/48/48/0 | 44992 | Rígido: Bip001 Spine2; geometría de silueta |
| Collar_blade_inner_R | 26→26 | 48→48 | 104 | 48/48/48/0 | 44992 | Rígido: Bip001 Spine2; geometría de silueta |
| Arm_surface_L | 1184→1184 | 2364→2364 | 1341 | 2364/1006/440/1566 | 622368 | Deformable; loops conservados; geometría de silueta |
| Arm_surface_R | 1184→1184 | 2364→2364 | 1338 | 2364/1022/440/1566 | 621312 | Deformable; loops conservados; geometría de silueta |
| Hand_glove_L | 1090→1090 | 2176→2176 | 1461 | 2176/818/242/2176 | 672720 | Deformable; loops conservados; geometría de silueta |
| Hand_glove_R | 1090→1090 | 2176→2176 | 1454 | 2176/792/230/2176 | 669352 | Deformable; loops conservados; geometría de silueta |
| Trousers_continuous | 1163→1163 | 2322→2322 | 1396 | 2322/1944/727/0 | 559948 | Deformable; loops conservados; geometría de silueta |
| Waist_wrap | 1248→624 | 2496→1248 | 726 | 1248/790/326/0 | 254880 | Deformable; 13 anillos ×24 columnas; pesos de vértices retenidos; geometría de silueta; relieve restante en v16 para H09 |
| Hood_continuous | 1029→1029 | 2054→2054 | 1205 | 2054/1026/410/0 | 417840 | Deformable; loops conservados; geometría de silueta |
| Neck_surface | 320→320 | 636→636 | 349 | 636/364/130/0 | 158744 | Deformable; loops conservados; geometría de silueta |
| Bracer_face_L | 217→47 | 430→90 | 101 | 90/68/45/90 | 45532 | Rígido: Bip001 L Forearm; geometría de silueta; relieve restante en v16 para H09 |
| Bracer_face_R | 217→50 | 430→96 | 103 | 96/67/42/96 | 46460 | Rígido: Bip001 R Forearm; geometría de silueta; relieve restante en v16 para H09 |
| Elbow_cuff_L | 181→37 | 358→70 | 78 | 70/44/34/70 | 35064 | Rígido: Bip001 L Forearm; geometría de silueta; relieve restante en v16 para H09 |
| Elbow_cuff_R | 181→37 | 358→70 | 78 | 70/44/34/70 | 35064 | Rígido: Bip001 R Forearm; geometría de silueta; relieve restante en v16 para H09 |
| Hand_back_full_point_L | 325→163 | 646→322 | 336 | 322/120/85/322 | 149964 | Rígido: Bip001 L Hand; geometría de silueta; relieve restante en v16 para H09 |
| Hand_back_full_point_R | 325→163 | 646→322 | 345 | 322/125/89/322 | 153816 | Rígido: Bip001 R Hand; geometría de silueta; relieve restante en v16 para H09 |
| Hip_guard_L | 181→37 | 358→70 | 77 | 70/52/34/0 | 33904 | Rígido: Hound L HipGuard; geometría de silueta; relieve restante en v16 para H09 |
| Hip_guard_R | 181→37 | 358→70 | 78 | 70/56/39/0 | 34428 | Rígido: Hound R HipGuard; geometría de silueta; relieve restante en v16 para H09 |
| Thigh_outer_plate_L | 181→37 | 358→70 | 68 | 70/54/42/0 | 30280 | Rígido: Bip001 L Thigh; geometría de silueta; relieve restante en v16 para H09 |
| Thigh_outer_plate_R | 181→37 | 358→70 | 70 | 70/54/42/0 | 31112 | Rígido: Bip001 R Thigh; geometría de silueta; relieve restante en v16 para H09 |
| Bracer_mass_L | 384→246 | 768→492 | 392 | 492/276/113/492 | 179548 | Rígido: Bip001 L Forearm; geometría de silueta; relieve restante en v16 para H09 |
| Wrist_transition_L | 192→188 | 384→376 | 296 | 376/95/38/376 | 133756 | Rígido: Bip001 L Hand; geometría de silueta; relieve restante en v16 para H09 |
| Finger_L_0A | 110→65 | 216→126 | 133 | 126/44/24/126 | 59168 | Rígido: Bip001 L Finger1; geometría de silueta; relieve restante en v16 para H09 |
| Finger_L_0B | 110→56 | 216→108 | 118 | 108/35/23/108 | 52376 | Rígido: Bip001 L Finger11; geometría de silueta; relieve restante en v16 para H09 |
| Finger_L_1A | 110→61 | 216→118 | 128 | 118/41/24/118 | 56860 | Rígido: Bip001 L Finger2; geometría de silueta; relieve restante en v16 para H09 |
| Finger_L_1B | 110→56 | 216→108 | 121 | 108/38/25/108 | 53684 | Rígido: Bip001 L Finger21; geometría de silueta; relieve restante en v16 para H09 |
| Finger_L_2A | 110→64 | 216→124 | 128 | 124/40/25/124 | 57004 | Rígido: Bip001 L Finger3; geometría de silueta; relieve restante en v16 para H09 |
| Finger_L_2B | 110→56 | 216→108 | 118 | 108/38/23/108 | 52412 | Rígido: Bip001 L Finger31; geometría de silueta; relieve restante en v16 para H09 |
| Finger_L_3A | 110→61 | 216→118 | 121 | 118/36/22/118 | 53864 | Rígido: Bip001 L Finger4; geometría de silueta; relieve restante en v16 para H09 |
| Finger_L_3B | 110→66 | 216→128 | 126 | 128/32/22/128 | 56136 | Rígido: Bip001 L Finger41; geometría de silueta; relieve restante en v16 para H09 |
| Thumb_LA | 110→56 | 216→108 | 127 | 108/44/27/108 | 56276 | Rígido: Bip001 L Finger0; geometría de silueta; relieve restante en v16 para H09 |
| Bracer_mass_R | 384→246 | 768→492 | 392 | 492/270/120/492 | 179560 | Rígido: Bip001 R Forearm; geometría de silueta; relieve restante en v16 para H09 |
| Wrist_transition_R | 192→188 | 384→376 | 296 | 376/92/37/376 | 133708 | Rígido: Bip001 R Hand; geometría de silueta; relieve restante en v16 para H09 |
| Finger_R_0A | 110→63 | 216→122 | 130 | 122/40/23/122 | 57764 | Rígido: Bip001 R Finger1; geometría de silueta; relieve restante en v16 para H09 |
| Finger_R_0B | 110→56 | 216→108 | 123 | 108/39/23/108 | 54504 | Rígido: Bip001 R Finger11; geometría de silueta; relieve restante en v16 para H09 |
| Finger_R_1A | 110→63 | 216→122 | 129 | 122/42/23/122 | 57372 | Rígido: Bip001 R Finger2; geometría de silueta; relieve restante en v16 para H09 |
| Finger_R_1B | 110→56 | 216→108 | 121 | 108/42/23/108 | 53708 | Rígido: Bip001 R Finger21; geometría de silueta; relieve restante en v16 para H09 |
| Finger_R_2A | 110→61 | 216→118 | 126 | 118/40/22/118 | 55992 | Rígido: Bip001 R Finger3; geometría de silueta; relieve restante en v16 para H09 |
| Finger_R_2B | 110→56 | 216→108 | 118 | 108/39/23/108 | 52424 | Rígido: Bip001 R Finger31; geometría de silueta; relieve restante en v16 para H09 |
| Finger_R_3A | 110→58 | 216→112 | 117 | 112/37/23/112 | 52080 | Rígido: Bip001 R Finger4; geometría de silueta; relieve restante en v16 para H09 |
| Finger_R_3B | 110→66 | 216→128 | 125 | 128/33/25/128 | 55768 | Rígido: Bip001 R Finger41; geometría de silueta; relieve restante en v16 para H09 |
| Thumb_RA | 110→56 | 216→108 | 127 | 108/38/27/108 | 56204 | Rígido: Bip001 R Finger0; geometría de silueta; relieve restante en v16 para H09 |
| Greave_mass_L | 576→304 | 1152→608 | 455 | 608/361/101/0 | 202120 | Rígido: Bip001 L Calf; geometría de silueta; relieve restante en v16 para H09 |
| Boot_L | 449→292 | 894→580 | 487 | 580/515/237/0 | 218576 | Rígido: Bip001 L Foot; geometría de silueta; relieve restante en v16 para H09 |
| Boot_toe_L | 402→174 | 800→344 | 322 | 344/231/95/0 | 141992 | Rígido: Bip001 L Foot; geometría de silueta; relieve restante en v16 para H09 |
| Greave_mass_R | 576→304 | 1152→608 | 455 | 608/364/99/0 | 202132 | Rígido: Bip001 R Calf; geometría de silueta; relieve restante en v16 para H09 |
| Boot_R | 449→292 | 894→580 | 485 | 580/500/248/0 | 217696 | Rígido: Bip001 R Foot; geometría de silueta; relieve restante en v16 para H09 |
| Boot_toe_R | 402→174 | 800→344 | 324 | 344/231/105/0 | 142944 | Rígido: Bip001 R Foot; geometría de silueta; relieve restante en v16 para H09 |
| Head_surface | 9120→913 | 18236→1822 | 1071 | 1822/1200/426/0 | 486912 | Rígido: Bip001 Head; geometría de silueta; relieve restante en v16 para H09 |
| Eye_L | 350→71 | 696→138 | 90 | 138/70/28/0 | 30912 | Rígido: Bip001 Head; geometría de silueta; relieve restante en v16 para H09 |
| Eye_R | 350→71 | 696→138 | 89 | 138/68/26/0 | 30552 | Rígido: Bip001 Head; geometría de silueta; relieve restante en v16 para H09 |
| Mouth_shadow | 196→40 | 388→76 | 70 | 76/50/14/0 | 30800 | Rígido: Bip001 Head; geometría de silueta; relieve restante en v16 para H09 |
| Sash_tail_L | 442→442 | 880→880 | 498 | 880/116/28/0 | 167664 | Deformable; loops conservados; geometría de silueta |
| Sash_tail_R | 442→442 | 880→880 | 498 | 880/114/26/0 | 167616 | Deformable; loops conservados; geometría de silueta |
| Yoke_L | 32→32 | 60→60 | 52 | 60/56/32/0 | 23408 | Rígido: Bip001 Spine2; geometría de silueta |
| Yoke_R | 32→32 | 60→60 | 52 | 60/56/32/0 | 23408 | Rígido: Bip001 Spine2; geometría de silueta |
| Collar_blade_middle_L | 72→71 | 140→138 | 228 | 138/93/79/0 | 98568 | Rígido: Bip001 Spine2; geometría de silueta; relieve restante en v16 para H09 |
| Collar_blade_outer_L | 83→83 | 162→162 | 314 | 162/125/115/0 | 135448 | Rígido: Bip001 Spine2; geometría de silueta |
| Collar_blade_middle_R | 59→58 | 114→112 | 204 | 112/86/80/0 | 88200 | Rígido: Bip001 Spine2; geometría de silueta; relieve restante en v16 para H09 |
| Collar_blade_outer_R | 80→80 | 156→156 | 282 | 156/111/102/0 | 121740 | Rígido: Bip001 Spine2; geometría de silueta |
| Breastplate_L | 434→120 | 864→236 | 224 | 236/143/61/0 | 98464 | Rígido: Bip001 Spine2; geometría de silueta; relieve restante en v16 para H09 |
| Collar_plate_L | 36→36 | 68→68 | 82 | 68/68/28/0 | 36080 | Rígido: Bip001 Spine2; geometría de silueta |
| Back_blade_L | 16→16 | 28→28 | 33 | 28/24/12/0 | 14496 | Rígido: Bip001 Spine2; geometría de silueta |
| Breastplate_R | 434→119 | 864→234 | 223 | 234/142/63/0 | 98036 | Rígido: Bip001 Spine2; geometría de silueta; relieve restante en v16 para H09 |
| Collar_plate_R | 36→36 | 68→68 | 82 | 68/68/28/0 | 36080 | Rígido: Bip001 Spine2; geometría de silueta |
| Back_blade_R | 16→16 | 28→28 | 33 | 28/24/12/0 | 14496 | Rígido: Bip001 Spine2; geometría de silueta |
| Sternum | 242→72 | 480→140 | 142 | 140/97/33/0 | 62312 | Rígido: Bip001 Spine2; geometría de silueta; relieve restante en v16 para H09 |
| Abdominal_plate_1 | 578→130 | 1152→256 | 242 | 256/212/83/0 | 107284 | Rígido: Bip001 Spine1; geometría de silueta; relieve restante en v16 para H09 |
| Abdominal_plate_2 | 578→133 | 1152→262 | 246 | 262/179/85/0 | 108648 | Rígido: Bip001 Spine; geometría de silueta; relieve restante en v16 para H09 |
| Abdominal_plate_3 | 578→145 | 1152→286 | 260 | 286/182/77/0 | 114700 | Rígido: Bip001 Spine; geometría de silueta; relieve restante en v16 para H09 |
| Bracer_blade_upper_L | 14→14 | 24→24 | 30 | 24/24/14/24 | 13512 | Rígido: Bip001 L Forearm; geometría de silueta |
| Bracer_blade_lower_L | 14→14 | 24→24 | 30 | 24/24/14/24 | 13512 | Rígido: Bip001 L Forearm; geometría de silueta |
| Bracer_blade_distal_L | 14→14 | 24→24 | 38 | 24/24/16/24 | 16864 | Rígido: Bip001 L Forearm; geometría de silueta |
| Bracer_blade_upper_R | 14→14 | 24→24 | 30 | 24/24/14/24 | 13512 | Rígido: Bip001 R Forearm; geometría de silueta |
| Bracer_blade_lower_R | 14→14 | 24→24 | 30 | 24/24/14/24 | 13512 | Rígido: Bip001 R Forearm; geometría de silueta |
| Bracer_blade_distal_R | 14→14 | 24→24 | 38 | 24/24/16/24 | 16864 | Rígido: Bip001 R Forearm; geometría de silueta |
| Finger_L_0C | 73→73 | 142→142 | 98 | 142/30/4/142 | 44584 | Rígido: Bip001 L Finger12; geometría de silueta |
| Finger_L_1C | 73→73 | 142→142 | 98 | 142/26/2/142 | 44512 | Rígido: Bip001 L Finger22; geometría de silueta |
| Finger_L_2C | 73→73 | 142→142 | 98 | 142/32/2/142 | 44584 | Rígido: Bip001 L Finger32; geometría de silueta |
| Finger_L_3C | 73→73 | 142→142 | 98 | 142/30/2/142 | 44560 | Rígido: Bip001 L Finger42; geometría de silueta |
| Thumb_LB | 73→73 | 142→142 | 98 | 142/42/6/142 | 44752 | Rígido: Bip001 L Finger01; geometría de silueta |
| Finger_R_0C | 73→73 | 142→142 | 98 | 142/30/4/142 | 44584 | Rígido: Bip001 R Finger12; geometría de silueta |
| Finger_R_1C | 73→73 | 142→142 | 98 | 142/30/2/142 | 44560 | Rígido: Bip001 R Finger22; geometría de silueta |
| Finger_R_2C | 73→73 | 142→142 | 98 | 142/28/4/142 | 44560 | Rígido: Bip001 R Finger32; geometría de silueta |
| Finger_R_3C | 73→73 | 142→142 | 98 | 142/32/4/142 | 44608 | Rígido: Bip001 R Finger42; geometría de silueta |
| Thumb_RB | 73→73 | 142→142 | 98 | 142/42/4/142 | 44728 | Rígido: Bip001 R Finger01; geometría de silueta |
| Greave_front_L | 578→382 | 1152→760 | 564 | 760/285/95/0 | 248304 | Rígido: Bip001 L Calf; geometría de silueta; relieve restante en v16 para H09 |
| Shin_lower_L | 290→100 | 576→196 | 180 | 196/134/43/0 | 79356 | Rígido: Bip001 L Calf; geometría de silueta; relieve restante en v16 para H09 |
| Knee_shield_L | 386→164 | 768→324 | 284 | 324/164/52/0 | 124624 | Rígido: Bip001 L Calf; geometría de silueta; relieve restante en v16 para H09 |
| Ankle_guard_L | 386→181 | 768→358 | 294 | 358/182/47/0 | 129348 | Rígido: Bip001 L Foot; geometría de silueta; relieve restante en v16 para H09 |
| Greave_front_R | 578→384 | 1152→764 | 566 | 764/284/89/0 | 249100 | Rígido: Bip001 R Calf; geometría de silueta; relieve restante en v16 para H09 |
| Shin_lower_R | 290→101 | 576→198 | 180 | 198/135/46/0 | 79428 | Rígido: Bip001 R Calf; geometría de silueta; relieve restante en v16 para H09 |
| Knee_shield_R | 386→164 | 768→324 | 284 | 324/163/54/0 | 124636 | Rígido: Bip001 R Calf; geometría de silueta; relieve restante en v16 para H09 |
| Ankle_guard_R | 386→181 | 768→358 | 294 | 358/177/51/0 | 129336 | Rígido: Bip001 R Foot; geometría de silueta; relieve restante en v16 para H09 |
| Scapula_L | 242→103 | 480→202 | 190 | 202/124/50/0 | 83552 | Rígido: Bip001 Spine2; geometría de silueta; relieve restante en v16 para H09 |
| Scapula_R | 242→103 | 480→202 | 190 | 202/123/50/0 | 83540 | Rígido: Bip001 Spine2; geometría de silueta; relieve restante en v16 para H09 |
| Back_spine_0 | 362→279 | 720→554 | 423 | 554/187/73/0 | 185736 | Rígido: Bip001 Spine1; geometría de silueta; relieve restante en v16 para H09 |
| Back_spine_1 | 362→287 | 720→570 | 431 | 570/185/69/0 | 189184 | Rígido: Bip001 Spine1; geometría de silueta; relieve restante en v16 para H09 |
| Back_spine_2 | 362→304 | 720→604 | 448 | 604/187/64/0 | 196628 | Rígido: Bip001 Spine; geometría de silueta; relieve restante en v16 para H09 |
| Rib_flank_L | 544→154 | 1084→304 | 322 | 304/226/102/0 | 141536 | Rígido: Bip001 Spine2; geometría de silueta; relieve restante en v16 para H09 |
| Rib_lamella_2_L | 544→116 | 1084→228 | 247 | 228/166/86/0 | 108512 | Rígido: Bip001 Spine1; geometría de silueta; relieve restante en v16 para H09 |
| Rib_lamella_3_L | 544→110 | 1084→216 | 234 | 216/145/72/0 | 102540 | Rígido: Bip001 Spine; geometría de silueta; relieve restante en v16 para H09 |
| Rib_flank_R | 544→156 | 1084→308 | 334 | 308/226/103/0 | 146588 | Rígido: Bip001 Spine2; geometría de silueta; relieve restante en v16 para H09 |
| Rib_lamella_2_R | 544→110 | 1084→216 | 229 | 216/152/81/0 | 100652 | Rígido: Bip001 Spine1; geometría de silueta; relieve restante en v16 para H09 |
| Rib_lamella_3_R | 544→110 | 1084→216 | 239 | 216/147/79/0 | 104728 | Rígido: Bip001 Spine; geometría de silueta; relieve restante en v16 para H09 |

## Conservación y límites

44 archivos anteriores (.blend/glTF/BIN) comparados byte a byte con los blobs Git; manifiestos v13–v16 y v17 comprobados.
10 superficies deformables exactas, incluida capucha, cuello, brazos, guantes, pantalón, paños y soporte del torso.
La faja conserva todas sus filas de altura, doble pared y pesos en los vértices retenidos. No se elimina ninguna pieza o superficie oculta.
Cabeza, ojos, manos/garras, placas, carcasa, grebas y filos conservan geometría de silueta; biseles/relieve descartado siguen en v16 para H09.
Las 95 piezas rígidas siguen separadas y con un solo hueso; no hay soldaduras entre piezas ni con la piel.
Los 106 componentes TPS son cerrados/conexos/orientados y tienen volumen positivo. FPS corta por pesos >95 % según el contrato:
bordes abiertos en el recorte proximal del brazo son intencionales; no se tapan ni se exporta un segundo cuerpo superpuesto.
La limpieza retira degenerados heredados de polígonos de dedos/filos. LOD0/1/2/FPS cocinados: cero triángulos de área nula.
Máximo superficial bidireccional muestreado v16/v17: 2.917016 mm.
Se muestrean vértices y centros de triángulos; no es una cota matemática continua ni comparación solo de recuentos.
33 poses ×5.565 pares: mismos conjuntos de contactos y autointersecciones que v16; cambian los recuentos de caras tras la reducción.
Persisten codos a 85°, cabeza/torso al mirar abajo, flexión fuerte de capucha, inserciones de placas/guantes y apoyo Soul Reaper inválido.
H08 debe resolver pesos/holguras y agarres de las cinco armas; no se ha iniciado. H09 conserva la maestra para UV/bake.
