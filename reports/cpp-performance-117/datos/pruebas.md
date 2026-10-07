# 54 CTest por configuración

Datos de diagnóstico del hito 117. Capturas/builds/blobs permanecen en caché.

```json
{
  "Release": {
    "tests": [
      {
        "name": "gloom.allocation_profile",
        "passed": true,
        "seconds": 0.01
      },
      {
        "name": "gloom.pose_storage",
        "passed": true,
        "seconds": 0.02
      },
      {
        "name": "gloom.jobs",
        "passed": true,
        "seconds": 0.19
      },
      {
        "name": "gloom.particle_storage",
        "passed": true,
        "seconds": 0.13
      },
      {
        "name": "gloom.dependency_contract",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.game_tickets",
        "passed": true,
        "seconds": 0.18
      },
      {
        "name": "gloom.keycloak_identity",
        "passed": true,
        "seconds": 0.31
      },
      {
        "name": "gloom.match_https",
        "passed": true,
        "seconds": 20.42
      },
      {
        "name": "gloom.unit",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.entities",
        "passed": true,
        "seconds": 1.33
      },
      {
        "name": "gloom.storage",
        "passed": true,
        "seconds": 0.07
      },
      {
        "name": "gloom.physics",
        "passed": true,
        "seconds": 0.04
      },
      {
        "name": "gloom.render_scene",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.gpu_assets",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.visibility",
        "passed": true,
        "seconds": 0.04
      },
      {
        "name": "gloom.lighting",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.temporal",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.vertical_slice",
        "passed": true,
        "seconds": 0.33
      },
      {
        "name": "gloom.vertical_slice_network",
        "passed": true,
        "seconds": 0.4
      },
      {
        "name": "gloom.match_lobby",
        "passed": true,
        "seconds": 0.04
      },
      {
        "name": "gloom.match_discovery",
        "passed": true,
        "seconds": 0.06
      },
      {
        "name": "gloom.vertical_slice_transport",
        "passed": true,
        "seconds": 1.04
      },
      {
        "name": "gloom.network",
        "passed": true,
        "seconds": 0.18
      },
      {
        "name": "gloom.network_protocol",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.network_replication",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.combat",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.session",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.legacy_movement",
        "passed": true,
        "seconds": 0.04
      },
      {
        "name": "gloom.audio_no_device",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.slice_server_smoke",
        "passed": true,
        "seconds": 0.14
      },
      {
        "name": "gloom.audio",
        "passed": true,
        "seconds": 0.75
      },
      {
        "name": "gloom.audio_network",
        "passed": true,
        "seconds": 0.23
      },
      {
        "name": "gloom.legacy_pickups",
        "passed": true,
        "seconds": 0.06
      },
      {
        "name": "gloom.pickup_presentation",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.legacy_arsenal",
        "passed": true,
        "seconds": 0.06
      },
      {
        "name": "gloom.ui",
        "passed": true,
        "seconds": 0.06
      },
      {
        "name": "gloom.animation_vfx",
        "passed": true,
        "seconds": 0.12
      },
      {
        "name": "gloom.skin_bounds",
        "passed": true,
        "seconds": 0.76
      },
      {
        "name": "gloom.animation_network",
        "passed": false,
        "seconds": 4.53
      },
      {
        "name": "gloom.animation_dedicated",
        "passed": true,
        "seconds": 40.72
      },
      {
        "name": "gloom.ui_visual_review",
        "passed": false,
        "seconds": 6.27
      },
      {
        "name": "gloom.ui_flow",
        "passed": true,
        "seconds": 11.62
      },
      {
        "name": "gloom.character_restoration",
        "passed": true,
        "seconds": 0.1
      },
      {
        "name": "gloom.material_render",
        "passed": true,
        "seconds": 1.68
      },
      {
        "name": "gloom.factory_restoration",
        "passed": true,
        "seconds": 0.24
      },
      {
        "name": "gloom.assets",
        "passed": true,
        "seconds": 0.15
      },
      {
        "name": "gloom.sdl_smoke",
        "passed": true,
        "seconds": 0.95
      },
      {
        "name": "gloom.visual_review",
        "passed": true,
        "seconds": 1.36
      },
      {
        "name": "gloom.factory_visual_review",
        "passed": false,
        "seconds": 2.5
      },
      {
        "name": "gloom.character_visual_review",
        "passed": false,
        "seconds": 2.87
      },
      {
        "name": "gloom.network_scene_smoke",
        "passed": true,
        "seconds": 1.06
      },
      {
        "name": "gloom.vertical_slice_smoke",
        "passed": true,
        "seconds": 1.27
      },
      {
        "name": "gloom.vulkan_sync",
        "passed": true,
        "seconds": 1.41
      },
      {
        "name": "gloom.hound_runtime",
        "passed": true,
        "seconds": 4.83
      }
    ],
    "failed": [
      "gloom.animation_network",
      "gloom.ui_visual_review",
      "gloom.factory_visual_review",
      "gloom.character_visual_review"
    ],
    "focal_passed": 7,
    "seconds": 108.96,
    "failure_evidence": [
      "Animation network review: GameNetworkingSockets send failed with result 25",
      "UI image regression failed: Visual regression: inspect full-resolution",
      "captures and render.log; references were not changed",
      "-- factory-overview.ppm: mean=2.28549/255 changed=0.0555556% worst_tile=6.06444/255",
      "factory-spawn-1.ppm: mean=3.55565/255 changed=1.5% worst_tile=31.3844/255",
      "factory-spawn-8.ppm: mean=1.8641/255 changed=0.0763889% worst_tile=8.82333/255",
      "factory-spawn-9.ppm: mean=3.9437/255 changed=0.520833% worst_tile=11.1189/255",
      "factory-lava.ppm: mean=2.37868/255 changed=0.0625% worst_tile=6.25556/255",
      "factory-central-walkway.ppm: mean=2.1463/255 changed=0.0763889% worst_tile=5.51889/255",
      "references were not changed",
      "-- archangel-front.ppm: mean=6.85396/255 changed=3.55556% worst_tile=19.9933/255",
      "archangel-back.ppm: mean=6.97502/255 changed=3.72917% worst_tile=19.9933/255",
      "shadow-front.ppm: mean=6.8085/255 changed=3.55556% worst_tile=19.9933/255",
      "shadow-back.ppm: mean=6.82838/255 changed=3.55556% worst_tile=19.9933/255",
      "soul-reaper-forward.ppm: mean=5.55755/255 changed=2.52083% worst_tile=28.5722/255",
      "soul-reaper-up.ppm: mean=3.02542/255 changed=0.805556% worst_tile=21.5622/255",
      "soul-reaper-down.ppm: mean=11.8938/255 changed=7.80556% worst_tile=27.5056/255",
      "references were not changed"
    ]
  },
  "Debug": {
    "tests": [
      {
        "name": "gloom.allocation_profile",
        "passed": true,
        "seconds": 0.04
      },
      {
        "name": "gloom.pose_storage",
        "passed": true,
        "seconds": 0.08
      },
      {
        "name": "gloom.jobs",
        "passed": true,
        "seconds": 0.13
      },
      {
        "name": "gloom.particle_storage",
        "passed": true,
        "seconds": 1.06
      },
      {
        "name": "gloom.dependency_contract",
        "passed": true,
        "seconds": 0.21
      },
      {
        "name": "gloom.game_tickets",
        "passed": true,
        "seconds": 0.33
      },
      {
        "name": "gloom.keycloak_identity",
        "passed": true,
        "seconds": 0.39
      },
      {
        "name": "gloom.match_https",
        "passed": true,
        "seconds": 20.55
      },
      {
        "name": "gloom.unit",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.entities",
        "passed": true,
        "seconds": 2.88
      },
      {
        "name": "gloom.storage",
        "passed": true,
        "seconds": 0.21
      },
      {
        "name": "gloom.physics",
        "passed": true,
        "seconds": 0.11
      },
      {
        "name": "gloom.render_scene",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.gpu_assets",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.visibility",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.lighting",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.temporal",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.vertical_slice",
        "passed": true,
        "seconds": 1.72
      },
      {
        "name": "gloom.vertical_slice_network",
        "passed": true,
        "seconds": 3.72
      },
      {
        "name": "gloom.match_lobby",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.match_discovery",
        "passed": true,
        "seconds": 0.05
      },
      {
        "name": "gloom.vertical_slice_transport",
        "passed": true,
        "seconds": 1.57
      },
      {
        "name": "gloom.network",
        "passed": true,
        "seconds": 0.19
      },
      {
        "name": "gloom.network_protocol",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.network_replication",
        "passed": true,
        "seconds": 0.07
      },
      {
        "name": "gloom.combat",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.session",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.legacy_movement",
        "passed": true,
        "seconds": 0.49
      },
      {
        "name": "gloom.audio_no_device",
        "passed": true,
        "seconds": 0.05
      },
      {
        "name": "gloom.slice_server_smoke",
        "passed": true,
        "seconds": 0.34
      },
      {
        "name": "gloom.audio",
        "passed": true,
        "seconds": 1.25
      },
      {
        "name": "gloom.audio_network",
        "passed": true,
        "seconds": 1.25
      },
      {
        "name": "gloom.legacy_pickups",
        "passed": true,
        "seconds": 0.27
      },
      {
        "name": "gloom.pickup_presentation",
        "passed": true,
        "seconds": 0.03
      },
      {
        "name": "gloom.legacy_arsenal",
        "passed": true,
        "seconds": 0.43
      },
      {
        "name": "gloom.ui",
        "passed": true,
        "seconds": 0.08
      },
      {
        "name": "gloom.animation_vfx",
        "passed": true,
        "seconds": 0.99
      },
      {
        "name": "gloom.skin_bounds",
        "passed": true,
        "seconds": 5.43
      },
      {
        "name": "gloom.animation_network",
        "passed": true,
        "seconds": 7.93
      },
      {
        "name": "gloom.animation_dedicated",
        "passed": true,
        "seconds": 40.88
      },
      {
        "name": "gloom.ui_visual_review",
        "passed": false,
        "seconds": 19.45
      },
      {
        "name": "gloom.ui_flow",
        "passed": true,
        "seconds": 26.75
      },
      {
        "name": "gloom.character_restoration",
        "passed": true,
        "seconds": 0.76
      },
      {
        "name": "gloom.material_render",
        "passed": true,
        "seconds": 6.25
      },
      {
        "name": "gloom.factory_restoration",
        "passed": true,
        "seconds": 2.1
      },
      {
        "name": "gloom.assets",
        "passed": true,
        "seconds": 0.9
      },
      {
        "name": "gloom.sdl_smoke",
        "passed": true,
        "seconds": 4.77
      },
      {
        "name": "gloom.visual_review",
        "passed": true,
        "seconds": 7.57
      },
      {
        "name": "gloom.factory_visual_review",
        "passed": false,
        "seconds": 16.59
      },
      {
        "name": "gloom.character_visual_review",
        "passed": false,
        "seconds": 16.54
      },
      {
        "name": "gloom.network_scene_smoke",
        "passed": true,
        "seconds": 5.22
      },
      {
        "name": "gloom.vertical_slice_smoke",
        "passed": true,
        "seconds": 7.99
      },
      {
        "name": "gloom.vulkan_sync",
        "passed": true,
        "seconds": 6.49
      },
      {
        "name": "gloom.hound_runtime",
        "passed": true,
        "seconds": 41.78
      }
    ],
    "failed": [
      "gloom.ui_visual_review",
      "gloom.factory_visual_review",
      "gloom.character_visual_review"
    ],
    "focal_passed": 7,
    "seconds": 256.21,
    "failure_evidence": [
      "UI image regression failed: Visual regression: inspect full-resolution",
      "captures and render.log; references were not changed",
      "-- factory-overview.ppm: mean=2.28549/255 changed=0.0555556% worst_tile=6.06444/255",
      "factory-spawn-1.ppm: mean=3.55565/255 changed=1.5% worst_tile=31.3844/255",
      "factory-spawn-8.ppm: mean=1.8641/255 changed=0.0763889% worst_tile=8.82333/255",
      "factory-spawn-9.ppm: mean=3.9437/255 changed=0.520833% worst_tile=11.1189/255",
      "factory-lava.ppm: mean=2.37868/255 changed=0.0625% worst_tile=6.25556/255",
      "factory-central-walkway.ppm: mean=2.1463/255 changed=0.0763889% worst_tile=5.51889/255",
      "references were not changed",
      "-- archangel-front.ppm: mean=6.65729/255 changed=3.49306% worst_tile=19.86/255",
      "archangel-back.ppm: mean=6.7847/255 changed=3.64583% worst_tile=19.8611/255",
      "shadow-front.ppm: mean=6.62535/255 changed=3.49306% worst_tile=19.8611/255",
      "shadow-back.ppm: mean=6.6469/255 changed=3.49306% worst_tile=19.8611/255",
      "soul-reaper-forward.ppm: mean=5.36887/255 changed=2.5% worst_tile=28.6167/255",
      "soul-reaper-up.ppm: mean=2.95137/255 changed=0.8125% worst_tile=21.7567/255",
      "soul-reaper-down.ppm: mean=11.7389/255 changed=7.72917% worst_tile=27.48/255",
      "references were not changed"
    ]
  }
}
```
