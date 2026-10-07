```text
Test project D:/Projects/Gloom/build/windows-vs
      Start  1: gloom.allocation_profile
 1/55 Test  #1: gloom.allocation_profile .........   Passed    0.06 sec
      Start  2: gloom.pose_storage
 2/55 Test  #2: gloom.pose_storage ...............   Passed    0.03 sec
      Start  3: gloom.jobs
 3/55 Test  #3: gloom.jobs .......................   Passed    0.03 sec
      Start  4: gloom.particle_storage
 4/55 Test  #4: gloom.particle_storage ...........   Passed    0.15 sec
      Start  5: gloom.dependency_contract
 5/55 Test  #5: gloom.dependency_contract ........   Passed    0.03 sec
      Start  6: gloom.game_tickets
 6/55 Test  #6: gloom.game_tickets ...............   Passed    0.20 sec
      Start  7: gloom.keycloak_identity
 7/55 Test  #7: gloom.keycloak_identity ..........   Passed    0.25 sec
      Start  8: gloom.match_https
 8/55 Test  #8: gloom.match_https ................   Passed   20.37 sec
      Start  9: gloom.unit
 9/55 Test  #9: gloom.unit .......................   Passed    0.02 sec
      Start 10: gloom.entities
10/55 Test #10: gloom.entities ...................   Passed    1.33 sec
      Start 11: gloom.storage
11/55 Test #11: gloom.storage ....................   Passed    0.06 sec
      Start 12: gloom.physics
12/55 Test #12: gloom.physics ....................   Passed    0.04 sec
      Start 13: gloom.render_scene
13/55 Test #13: gloom.render_scene ...............   Passed    0.02 sec
      Start 14: gloom.gpu_assets
14/55 Test #14: gloom.gpu_assets .................   Passed    0.03 sec
      Start 15: gloom.visibility
15/55 Test #15: gloom.visibility .................   Passed    0.03 sec
      Start 16: gloom.lighting
16/55 Test #16: gloom.lighting ...................   Passed    0.02 sec
      Start 17: gloom.temporal
17/55 Test #17: gloom.temporal ...................   Passed    0.02 sec
      Start 18: gloom.vertical_slice
18/55 Test #18: gloom.vertical_slice .............   Passed    0.30 sec
      Start 19: gloom.vertical_slice_network
19/55 Test #19: gloom.vertical_slice_network .....   Passed    0.34 sec
      Start 20: gloom.match_lobby
20/55 Test #20: gloom.match_lobby ................   Passed    0.03 sec
      Start 21: gloom.match_discovery
21/55 Test #21: gloom.match_discovery ............   Passed    0.06 sec
      Start 22: gloom.vertical_slice_transport
22/55 Test #22: gloom.vertical_slice_transport ...   Passed    1.02 sec
      Start 23: gloom.network
23/55 Test #23: gloom.network ....................   Passed    0.17 sec
      Start 24: gloom.network_protocol
24/55 Test #24: gloom.network_protocol ...........   Passed    0.03 sec
      Start 25: gloom.network_replication
25/55 Test #25: gloom.network_replication ........   Passed    0.03 sec
      Start 26: gloom.combat
26/55 Test #26: gloom.combat .....................   Passed    0.02 sec
      Start 27: gloom.session
27/55 Test #27: gloom.session ....................   Passed    0.03 sec
      Start 28: gloom.legacy_movement
28/55 Test #28: gloom.legacy_movement ............   Passed    0.05 sec
      Start 29: gloom.audio_no_device
29/55 Test #29: gloom.audio_no_device ............   Passed    0.03 sec
      Start 30: gloom.slice_server_smoke
30/55 Test #30: gloom.slice_server_smoke .........   Passed    0.14 sec
      Start 31: gloom.audio
31/55 Test #31: gloom.audio ......................   Passed    0.30 sec
      Start 32: gloom.audio_network
32/55 Test #32: gloom.audio_network ..............   Passed    0.32 sec
      Start 33: gloom.legacy_pickups
33/55 Test #33: gloom.legacy_pickups .............   Passed    0.05 sec
      Start 34: gloom.pickup_presentation
34/55 Test #34: gloom.pickup_presentation ........   Passed    0.02 sec
      Start 35: gloom.legacy_arsenal
35/55 Test #35: gloom.legacy_arsenal .............   Passed    0.06 sec
      Start 36: gloom.ui
36/55 Test #36: gloom.ui .........................   Passed    0.06 sec
      Start 37: gloom.animation_vfx
37/55 Test #37: gloom.animation_vfx ..............   Passed    0.13 sec
      Start 38: gloom.skin_bounds
38/55 Test #38: gloom.skin_bounds ................   Passed    0.77 sec
      Start 39: gloom.animation_network
39/55 Test #39: gloom.animation_network ..........***Failed    6.03 sec
Local waypoint=0 position=-35.5055,0.0823356,8.99865
Local waypoint=10 position=15.8293,0.000398964,-20.4048
Combat: shots=2, kills=1, death tick=697, respawn tick=937
Animation network review: GameNetworkingSockets send failed with result 25

      Start 40: gloom.animation_dedicated
40/55 Test #40: gloom.animation_dedicated ........   Passed   40.79 sec
      Start 41: gloom.ui_visual_review
41/55 Test #41: gloom.ui_visual_review ...........***Failed    5.93 sec
CMake Error at D:/Projects/Gloom/tools/review/ui_capture.cmake:18 (message):
  UI image regression failed: Visual regression: inspect full-resolution
  captures and render.log; references were not changed




      Start 42: gloom.ui_flow
42/55 Test #42: gloom.ui_flow ....................   Passed   10.07 sec
      Start 43: gloom.character_restoration
43/55 Test #43: gloom.character_restoration ......   Passed    0.09 sec
      Start 44: gloom.material_render
44/55 Test #44: gloom.material_render ............   Passed    1.34 sec
      Start 45: gloom.factory_restoration
45/55 Test #45: gloom.factory_restoration ........   Passed    0.26 sec
      Start 46: gloom.assets
46/55 Test #46: gloom.assets .....................   Passed    0.14 sec
      Start 47: gloom.asset_residency
47/55 Test #47: gloom.asset_residency ............   Passed    0.05 sec
      Start 48: gloom.sdl_smoke
48/55 Test #48: gloom.sdl_smoke ..................   Passed    0.89 sec
      Start 49: gloom.visual_review
49/55 Test #49: gloom.visual_review ..............   Passed    1.27 sec
      Start 50: gloom.factory_visual_review
50/55 Test #50: gloom.factory_visual_review ......***Failed    2.40 sec
-- factory-overview.ppm: mean=1.85215/255 changed=0.0347222% worst_tile=6.15333/255
factory-spawn-1.ppm: mean=3.30998/255 changed=1.47917% worst_tile=31.2022/255
factory-spawn-8.ppm: mean=1.3665/255 changed=0.0763889% worst_tile=8.05111/255
factory-spawn-9.ppm: mean=3.73687/255 changed=0.826389% worst_tile=10.9822/255
factory-lava.ppm: mean=1.96637/255 changed=0.0625% worst_tile=6.32222/255
factory-central-walkway.ppm: mean=1.74442/255 changed=0.0694444% worst_tile=4.83778/255

CMake Error at D:/Projects/Gloom/cmake/RunVisualReview.cmake:23 (message):
  Visual regression: inspect full-resolution captures and render.log;
  references were not changed




      Start 51: gloom.character_visual_review
51/55 Test #51: gloom.character_visual_review ....***Failed    2.78 sec
-- archangel-front.ppm: mean=6.92431/255 changed=3.65278% worst_tile=19.9933/255
archangel-back.ppm: mean=7.04718/255 changed=3.83333% worst_tile=19.9933/255
shadow-front.ppm: mean=6.88477/255 changed=3.65278% worst_tile=19.9933/255
shadow-back.ppm: mean=6.90449/255 changed=3.65278% worst_tile=19.9933/255
soul-reaper-forward.ppm: mean=5.62019/255 changed=2.69444% worst_tile=28.4722/255
soul-reaper-up.ppm: mean=2.97271/255 changed=0.763889% worst_tile=21.2089/255
soul-reaper-down.ppm: mean=12.0203/255 changed=7.99306% worst_tile=27.4622/255

CMake Error at D:/Projects/Gloom/cmake/RunVisualReview.cmake:23 (message):
  Visual regression: inspect full-resolution captures and render.log;
  references were not changed




      Start 52: gloom.network_scene_smoke
52/55 Test #52: gloom.network_scene_smoke ........   Passed    0.98 sec
      Start 53: gloom.vertical_slice_smoke
53/55 Test #53: gloom.vertical_slice_smoke .......   Passed    1.20 sec
      Start 54: gloom.vulkan_sync
54/55 Test #54: gloom.vulkan_sync ................   Passed    1.33 sec
      Start 55: gloom.hound_runtime
55/55 Test #55: gloom.hound_runtime ..............   Passed    4.48 sec

93% tests passed, 4 tests failed out of 55

Label Time Summary:
gameplay    =   1.20 sec*proc (1 test)
graphics    =  32.65 sec*proc (11 tests)
network     =  79.86 sec*proc (9 tests)
visual      =  13.70 sec*proc (5 tests)

Total Test time (real) = 106.74 sec

The following tests FAILED:
	 39 - gloom.animation_network (Failed)                  network
	 41 - gloom.ui_visual_review (Failed)                   graphics visual
	 50 - gloom.factory_visual_review (Failed)              graphics visual
	 51 - gloom.character_visual_review (Failed)            graphics visual
Errors while running CTest

```
