```json
{
  "commit": "934b73dd50f9df4cd807646235da5cf79b970ec5",
  "exe_sha256": "480d9986b8078d862ceb45f835a0a59b27ab45f9805d53e944fa42b8e6a20e27",
  "profile": false,
  "finished_utc": "2026-10-07T15:05:44.084798+00:00",
  "source_diff_sha256": "c86ed6434de7e2084dc90386291fe69d62defa02dcb3c66bf500b09c21c08e7d",
  "runs": [
    {
      "case": "factory-1",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.043 queue_present=0.026 acquire=0.003 fence_wait=1.466 ms",
        "Factory benchmark: samples=360 mean=2.338 ms fps=427.72 p50=2.336 p95=2.363 p99=2.408 max=2.471 ms",
        "CPU stages ms: update=0.149 begin=0.019 presentation=0.107 visibility=0.025 lighting=0.018 draw=0.463 end_present=1.556",
        "CPU animation ms: poses=0.077 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.044 p50=2.045 p95=2.051 p99=2.058 max=2.066 ms",
        "Process memory: private=913.21 MiB working_set=391.59 MiB peak_working_set=501.53 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.039 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1600 completed, 1031 helped by the waiting thread, peak queue 107, 2468082 us executing and 622476 us waiting."
      ]
    },
    {
      "case": "factory-2",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.493 ms",
        "Factory benchmark: samples=360 mean=2.336 ms fps=428.00 p50=2.336 p95=2.352 p99=2.374 max=2.453 ms",
        "CPU stages ms: update=0.138 begin=0.021 presentation=0.103 visibility=0.024 lighting=0.018 draw=0.453 end_present=1.580",
        "CPU animation ms: poses=0.074 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.044 p50=2.045 p95=2.051 p99=2.053 max=2.056 ms",
        "Process memory: private=907.96 MiB working_set=391.16 MiB peak_working_set=500.80 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.046 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1671 completed, 1106 helped by the waiting thread, peak queue 107, 2499165 us executing and 623545 us waiting."
      ]
    },
    {
      "case": "factory-3",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.042 queue_present=0.025 acquire=0.002 fence_wait=1.489 ms",
        "Factory benchmark: samples=360 mean=2.339 ms fps=427.62 p50=2.338 p95=2.354 p99=2.378 max=2.484 ms",
        "CPU stages ms: update=0.141 begin=0.019 presentation=0.104 visibility=0.024 lighting=0.018 draw=0.456 end_present=1.575",
        "CPU animation ms: poses=0.075 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.046 p50=2.047 p95=2.053 p99=2.055 max=2.060 ms",
        "Process memory: private=911.37 MiB working_set=391.21 MiB peak_working_set=498.60 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.042 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1675 completed, 1101 helped by the waiting thread, peak queue 107, 2505846 us executing and 622556 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.043 queue_present=0.024 acquire=0.003 fence_wait=1.568 ms",
        "Factory benchmark: samples=360 mean=3.276 ms fps=305.27 p50=3.267 p95=3.504 p99=3.518 max=3.531 ms",
        "CPU stages ms: update=0.234 begin=0.021 presentation=0.602 visibility=0.031 lighting=0.017 draw=0.706 end_present=1.664",
        "CPU animation ms: poses=0.523 skin_bounds=0.051",
        "GPU full render: samples=359 mean=2.985 p50=2.973 p95=3.205 p99=3.217 max=3.233 ms",
        "Process memory: private=1410.73 MiB working_set=630.34 MiB peak_working_set=963.61 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.878 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 570 helped by the waiting thread, peak queue 134, 4042846 us executing and 1390102 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.047 queue_present=0.026 acquire=0.003 fence_wait=1.519 ms",
        "Factory benchmark: samples=360 mean=3.271 ms fps=305.70 p50=3.267 p95=3.502 p99=3.515 max=3.528 ms",
        "CPU stages ms: update=0.239 begin=0.020 presentation=0.604 visibility=0.036 lighting=0.018 draw=0.730 end_present=1.623",
        "CPU animation ms: poses=0.519 skin_bounds=0.052",
        "GPU full render: samples=359 mean=2.981 p50=2.974 p95=3.204 p99=3.211 max=3.223 ms",
        "Process memory: private=1405.31 MiB working_set=630.19 MiB peak_working_set=965.24 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.878 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 555 helped by the waiting thread, peak queue 134, 3922037 us executing and 1340438 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.624 ms",
        "Factory benchmark: samples=360 mean=3.277 ms fps=305.14 p50=3.270 p95=3.504 p99=3.521 max=3.541 ms",
        "CPU stages ms: update=0.228 begin=0.019 presentation=0.574 visibility=0.029 lighting=0.017 draw=0.696 end_present=1.716",
        "CPU animation ms: poses=0.498 skin_bounds=0.051",
        "GPU full render: samples=359 mean=2.981 p50=2.972 p95=3.205 p99=3.212 max=3.218 ms",
        "Process memory: private=1419.22 MiB working_set=634.85 MiB peak_working_set=965.62 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.429 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 556 helped by the waiting thread, peak queue 134, 3897873 us executing and 1320530 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.042 queue_present=0.024 acquire=0.002 fence_wait=1.613 ms",
        "Factory benchmark: samples=360 mean=3.279 ms fps=305.00 p50=3.268 p95=3.504 p99=3.531 max=3.691 ms",
        "CPU stages ms: update=0.236 begin=0.019 presentation=0.575 visibility=0.029 lighting=0.017 draw=0.696 end_present=1.706",
        "CPU animation ms: poses=0.499 skin_bounds=0.050",
        "GPU full render: samples=359 mean=2.983 p50=2.972 p95=3.203 p99=3.213 max=3.220 ms",
        "Process memory: private=1406.91 MiB working_set=628.45 MiB peak_working_set=964.38 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.449 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 556 helped by the waiting thread, peak queue 134, 3971539 us executing and 1400900 us waiting."
      ]
    }
  ]
}
```
