```json
{
  "commit": "934b73dd50f9df4cd807646235da5cf79b970ec5",
  "exe_sha256": "b5260790409ffc5a0c64cfdcb7c66abc5de0813b766cd8eaf69f7821f14d1eb4",
  "profile": false,
  "finished_utc": "2026-10-07T15:58:14.902314+00:00",
  "source_diff_sha256": "5d9aed38087b7c1d21ef6bd45be22225a9dc420240240a47bf161fa16b89b6e3",
  "runs": [
    {
      "case": "factory-1",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.462 ms",
        "Factory benchmark: samples=360 mean=2.291 ms fps=436.40 p50=2.291 p95=2.306 p99=2.312 max=2.398 ms",
        "CPU stages ms: update=0.135 begin=0.017 presentation=0.102 visibility=0.023 lighting=0.018 draw=0.451 end_present=1.546",
        "CPU animation ms: poses=0.074 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.029 p50=2.030 p95=2.036 p99=2.038 max=2.141 ms",
        "Process memory: private=1077.15 MiB working_set=570.27 MiB peak_working_set=702.11 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.031 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1232 completed, 653 helped by the waiting thread, peak queue 107, 2428107 us executing and 791075 us waiting."
      ]
    },
    {
      "case": "factory-2",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.463 ms",
        "Factory benchmark: samples=360 mean=2.295 ms fps=435.82 p50=2.293 p95=2.309 p99=2.425 max=2.440 ms",
        "CPU stages ms: update=0.132 begin=0.017 presentation=0.103 visibility=0.024 lighting=0.018 draw=0.451 end_present=1.548",
        "CPU animation ms: poses=0.075 skin_bounds=0.004",
        "GPU full render: samples=359 mean=2.030 p50=2.031 p95=2.036 p99=2.038 max=2.137 ms",
        "Process memory: private=1072.02 MiB working_set=570.20 MiB peak_working_set=701.45 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.026 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1286 completed, 712 helped by the waiting thread, peak queue 107, 2383689 us executing and 763374 us waiting."
      ]
    },
    {
      "case": "factory-3",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.042 queue_present=0.025 acquire=0.002 fence_wait=1.466 ms",
        "Factory benchmark: samples=360 mean=2.297 ms fps=435.36 p50=2.297 p95=2.311 p99=2.423 max=2.436 ms",
        "CPU stages ms: update=0.132 begin=0.017 presentation=0.102 visibility=0.023 lighting=0.018 draw=0.453 end_present=1.551",
        "CPU animation ms: poses=0.074 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.032 p50=2.034 p95=2.039 p99=2.041 max=2.043 ms",
        "Process memory: private=1072.61 MiB working_set=569.96 MiB peak_working_set=702.30 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.028 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1269 completed, 697 helped by the waiting thread, peak queue 107, 2361291 us executing and 761545 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.042 queue_present=0.024 acquire=0.002 fence_wait=1.546 ms",
        "Factory benchmark: samples=360 mean=3.219 ms fps=310.61 p50=3.216 p95=3.450 p99=3.486 max=3.498 ms",
        "CPU stages ms: update=0.226 begin=0.020 presentation=0.578 visibility=0.031 lighting=0.018 draw=0.704 end_present=1.642",
        "CPU animation ms: poses=0.497 skin_bounds=0.052",
        "GPU full render: samples=359 mean=2.961 p50=2.954 p95=3.184 p99=3.220 max=3.225 ms",
        "Process memory: private=1791.63 MiB working_set=993.80 MiB peak_working_set=1327.40 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.427 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 572 helped by the waiting thread, peak queue 134, 3831148 us executing and 1251856 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.042 queue_present=0.024 acquire=0.002 fence_wait=1.570 ms",
        "Factory benchmark: samples=360 mean=3.233 ms fps=309.34 p50=3.219 p95=3.450 p99=3.463 max=3.478 ms",
        "CPU stages ms: update=0.224 begin=0.019 presentation=0.573 visibility=0.031 lighting=0.018 draw=0.703 end_present=1.664",
        "CPU animation ms: poses=0.493 skin_bounds=0.052",
        "GPU full render: samples=359 mean=2.974 p50=2.956 p95=3.186 p99=3.193 max=3.202 ms",
        "Process memory: private=1769.60 MiB working_set=993.30 MiB peak_working_set=1327.78 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.430 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 560 helped by the waiting thread, peak queue 134, 3792631 us executing and 1264536 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.039 queue_present=0.023 acquire=0.002 fence_wait=1.648 ms",
        "Factory benchmark: samples=360 mean=3.241 ms fps=308.51 p50=3.225 p95=3.456 p99=3.464 max=3.472 ms",
        "CPU stages ms: update=0.217 begin=0.017 presentation=0.556 visibility=0.028 lighting=0.017 draw=0.669 end_present=1.736",
        "CPU animation ms: poses=0.480 skin_bounds=0.051",
        "GPU full render: samples=359 mean=2.979 p50=2.959 p95=3.191 p99=3.196 max=3.202 ms",
        "Process memory: private=1772.58 MiB working_set=996.77 MiB peak_working_set=1330.00 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.424 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 574 helped by the waiting thread, peak queue 134, 3742482 us executing and 1221449 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.039 queue_present=0.023 acquire=0.002 fence_wait=1.634 ms",
        "Factory benchmark: samples=360 mean=3.241 ms fps=308.51 p50=3.226 p95=3.457 p99=3.465 max=3.483 ms",
        "CPU stages ms: update=0.218 begin=0.017 presentation=0.561 visibility=0.029 lighting=0.017 draw=0.676 end_present=1.722",
        "CPU animation ms: poses=0.485 skin_bounds=0.051",
        "GPU full render: samples=359 mean=2.979 p50=2.959 p95=3.191 p99=3.197 max=3.199 ms",
        "Process memory: private=1771.25 MiB working_set=995.07 MiB peak_working_set=1328.23 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.868 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 574 helped by the waiting thread, peak queue 134, 3781074 us executing and 1235638 us waiting."
      ]
    }
  ]
}
```
