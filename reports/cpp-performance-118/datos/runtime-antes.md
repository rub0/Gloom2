```json
{
  "commit": "934b73dd50f9df4cd807646235da5cf79b970ec5",
  "exe_sha256": "b5260790409ffc5a0c64cfdcb7c66abc5de0813b766cd8eaf69f7821f14d1eb4",
  "profile": false,
  "finished_utc": "2026-10-07T14:50:25.849225+00:00",
  "source_diff_sha256": "120116ec489d215b4d6e0b79f19ed16923db8320aac9a134a037fbe57238956f",
  "runs": [
    {
      "case": "factory-1",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.042 queue_present=0.025 acquire=0.002 fence_wait=1.485 ms",
        "Factory benchmark: samples=360 mean=2.347 ms fps=426.01 p50=2.346 p95=2.365 p99=2.480 max=3.296 ms",
        "CPU stages ms: update=0.141 begin=0.024 presentation=0.105 visibility=0.025 lighting=0.018 draw=0.462 end_present=1.572",
        "CPU animation ms: poses=0.076 skin_bounds=0.004",
        "GPU full render: samples=359 mean=2.053 p50=2.055 p95=2.061 p99=2.064 max=2.067 ms",
        "Process memory: private=1074.15 MiB working_set=572.49 MiB peak_working_set=705.53 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.056 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1200 completed, 633 helped by the waiting thread, peak queue 107, 2453578 us executing and 803928 us waiting."
      ]
    },
    {
      "case": "factory-2",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.042 queue_present=0.025 acquire=0.002 fence_wait=1.484 ms",
        "Factory benchmark: samples=360 mean=2.344 ms fps=426.65 p50=2.342 p95=2.362 p99=2.388 max=2.510 ms",
        "CPU stages ms: update=0.141 begin=0.019 presentation=0.108 visibility=0.025 lighting=0.019 draw=0.461 end_present=1.571",
        "CPU animation ms: poses=0.078 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.051 p50=2.052 p95=2.058 p99=2.061 max=2.172 ms",
        "Process memory: private=1078.06 MiB working_set=570.59 MiB peak_working_set=699.79 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.046 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1225 completed, 654 helped by the waiting thread, peak queue 107, 2407019 us executing and 775924 us waiting."
      ]
    },
    {
      "case": "factory-3",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.045 queue_present=0.026 acquire=0.002 fence_wait=1.460 ms",
        "Factory benchmark: samples=360 mean=2.350 ms fps=425.56 p50=2.346 p95=2.381 p99=2.486 max=2.585 ms",
        "CPU stages ms: update=0.147 begin=0.022 presentation=0.111 visibility=0.025 lighting=0.019 draw=0.474 end_present=1.552",
        "CPU animation ms: poses=0.080 skin_bounds=0.004",
        "GPU full render: samples=359 mean=2.053 p50=2.053 p95=2.060 p99=2.068 max=2.089 ms",
        "Process memory: private=1076.77 MiB working_set=570.27 MiB peak_working_set=708.21 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.048 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1154 completed, 578 helped by the waiting thread, peak queue 107, 2512234 us executing and 870433 us waiting."
      ]
    },
    {
      "case": "hound-null-1",
      "options": [
        "--vertical-slice-performance-hound-eight-1080p",
        "--hound-production-v17",
        "--hound-budget-lod=0"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.045 queue_present=0.025 acquire=0.002 fence_wait=1.575 ms",
        "Factory benchmark: samples=360 mean=3.283 ms fps=304.62 p50=3.278 p95=3.514 p99=3.538 max=3.558 ms",
        "CPU stages ms: update=0.238 begin=0.021 presentation=0.585 visibility=0.032 lighting=0.018 draw=0.715 end_present=1.674",
        "CPU animation ms: poses=0.505 skin_bounds=0.052",
        "GPU full render: samples=359 mean=2.989 p50=2.981 p95=3.213 p99=3.224 max=3.227 ms",
        "Process memory: private=1773.44 MiB working_set=994.60 MiB peak_working_set=1327.26 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.439 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 569 helped by the waiting thread, peak queue 134, 3964222 us executing and 1330985 us waiting."
      ]
    },
    {
      "case": "hound-null-2",
      "options": [
        "--vertical-slice-performance-hound-eight-1080p",
        "--hound-production-v17",
        "--hound-budget-lod=0"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.043 queue_present=0.024 acquire=0.002 fence_wait=1.587 ms",
        "Factory benchmark: samples=360 mean=3.278 ms fps=305.05 p50=3.275 p95=3.508 p99=3.526 max=3.578 ms",
        "CPU stages ms: update=0.239 begin=0.020 presentation=0.578 visibility=0.032 lighting=0.018 draw=0.709 end_present=1.683",
        "CPU animation ms: poses=0.496 skin_bounds=0.052",
        "GPU full render: samples=359 mean=2.985 p50=2.981 p95=3.212 p99=3.225 max=3.228 ms",
        "Process memory: private=1776.79 MiB working_set=994.01 MiB peak_working_set=1326.61 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.443 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 558 helped by the waiting thread, peak queue 134, 3890551 us executing and 1278515 us waiting."
      ]
    },
    {
      "case": "hound-audio-1",
      "options": [
        "--vertical-slice-performance-hound-eight-1080p",
        "--hound-production-v17",
        "--hound-budget-lod=0",
        "--audio-device"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.621 ms",
        "Factory benchmark: samples=360 mean=3.284 ms fps=304.51 p50=3.278 p95=3.513 p99=3.528 max=3.537 ms",
        "CPU stages ms: update=0.237 begin=0.018 presentation=0.570 visibility=0.029 lighting=0.017 draw=0.698 end_present=1.713",
        "CPU animation ms: poses=0.492 skin_bounds=0.052",
        "GPU full render: samples=359 mean=2.989 p50=2.981 p95=3.212 p99=3.223 max=3.228 ms",
        "Process memory: private=1770.26 MiB working_set=994.80 MiB peak_working_set=1328.46 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.439 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 560 helped by the waiting thread, peak queue 134, 3800220 us executing and 1245515 us waiting."
      ]
    },
    {
      "case": "hound-audio-2",
      "options": [
        "--vertical-slice-performance-hound-eight-1080p",
        "--hound-production-v17",
        "--hound-budget-lod=0",
        "--audio-device"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.628 ms",
        "Factory benchmark: samples=360 mean=3.281 ms fps=304.77 p50=3.279 p95=3.517 p99=3.543 max=3.609 ms",
        "CPU stages ms: update=0.232 begin=0.019 presentation=0.573 visibility=0.029 lighting=0.018 draw=0.691 end_present=1.720",
        "CPU animation ms: poses=0.495 skin_bounds=0.052",
        "GPU full render: samples=359 mean=2.984 p50=2.980 p95=3.211 p99=3.222 max=3.326 ms",
        "Process memory: private=1778.04 MiB working_set=997.48 MiB peak_working_set=1330.01 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.448 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 567 helped by the waiting thread, peak queue 134, 3790794 us executing and 1253599 us waiting."
      ]
    }
  ]
}
```
