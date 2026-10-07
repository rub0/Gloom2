```json
{
  "commit": "934b73dd50f9df4cd807646235da5cf79b970ec5",
  "exe_sha256": "b729a905886b3ba246ac79cefbc980e9c3115d973ddf434504eac184c882ba1c",
  "profile": false,
  "finished_utc": "2026-10-07T14:50:57.136367+00:00",
  "source_diff_sha256": "120116ec489d215b4d6e0b79f19ed16923db8320aac9a134a037fbe57238956f",
  "runs": [
    {
      "case": "factory-1",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.045 queue_present=0.025 acquire=0.002 fence_wait=1.482 ms",
        "Factory benchmark: samples=360 mean=2.346 ms fps=426.23 p50=2.344 p95=2.369 p99=2.385 max=2.480 ms",
        "CPU stages ms: update=0.144 begin=0.020 presentation=0.106 visibility=0.025 lighting=0.018 draw=0.461 end_present=1.572",
        "CPU animation ms: poses=0.077 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.052 p50=2.052 p95=2.058 p99=2.062 max=2.064 ms",
        "Process memory: private=908.80 MiB working_set=391.02 MiB peak_working_set=496.59 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.051 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1624 completed, 1064 helped by the waiting thread, peak queue 107, 2459209 us executing and 634464 us waiting."
      ]
    },
    {
      "case": "factory-2",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.042 queue_present=0.025 acquire=0.002 fence_wait=1.492 ms",
        "Factory benchmark: samples=360 mean=2.345 ms fps=426.38 p50=2.344 p95=2.361 p99=2.453 max=2.514 ms",
        "CPU stages ms: update=0.139 begin=0.020 presentation=0.105 visibility=0.024 lighting=0.018 draw=0.458 end_present=1.580",
        "CPU animation ms: poses=0.076 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.051 p50=2.052 p95=2.057 p99=2.060 max=2.062 ms",
        "Process memory: private=895.58 MiB working_set=390.21 MiB peak_working_set=530.09 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.047 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1332 completed, 763 helped by the waiting thread, peak queue 107, 2496168 us executing and 845386 us waiting."
      ]
    },
    {
      "case": "factory-3",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.503 ms",
        "Factory benchmark: samples=360 mean=2.345 ms fps=426.46 p50=2.345 p95=2.364 p99=2.381 max=2.498 ms",
        "CPU stages ms: update=0.137 begin=0.019 presentation=0.105 visibility=0.025 lighting=0.018 draw=0.453 end_present=1.588",
        "CPU animation ms: poses=0.076 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.051 p50=2.052 p95=2.059 p99=2.061 max=2.064 ms",
        "Process memory: private=912.29 MiB working_set=391.02 MiB peak_working_set=496.27 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.046 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1618 completed, 1052 helped by the waiting thread, peak queue 107, 2443767 us executing and 623919 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.043 queue_present=0.024 acquire=0.002 fence_wait=1.572 ms",
        "Factory benchmark: samples=360 mean=3.267 ms fps=306.06 p50=3.272 p95=3.515 p99=3.533 max=3.660 ms",
        "CPU stages ms: update=0.233 begin=0.020 presentation=0.587 visibility=0.032 lighting=0.018 draw=0.710 end_present=1.668",
        "CPU animation ms: poses=0.506 skin_bounds=0.052",
        "GPU full render: samples=359 mean=2.976 p50=2.979 p95=3.216 p99=3.234 max=3.240 ms",
        "Process memory: private=1409.85 MiB working_set=631.64 MiB peak_working_set=963.38 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.446 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 562 helped by the waiting thread, peak queue 134, 3954115 us executing and 1374807 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.045 queue_present=0.025 acquire=0.002 fence_wait=1.569 ms",
        "Factory benchmark: samples=360 mean=3.281 ms fps=304.76 p50=3.276 p95=3.509 p99=3.531 max=3.663 ms",
        "CPU stages ms: update=0.235 begin=0.020 presentation=0.591 visibility=0.032 lighting=0.018 draw=0.718 end_present=1.668",
        "CPU animation ms: poses=0.510 skin_bounds=0.051",
        "GPU full render: samples=359 mean=2.987 p50=2.979 p95=3.211 p99=3.220 max=3.227 ms",
        "Process memory: private=1410.58 MiB working_set=630.29 MiB peak_working_set=963.28 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.445 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 564 helped by the waiting thread, peak queue 134, 3948655 us executing and 1349458 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.611 ms",
        "Factory benchmark: samples=360 mean=3.281 ms fps=304.82 p50=3.277 p95=3.508 p99=3.520 max=3.537 ms",
        "CPU stages ms: update=0.237 begin=0.019 presentation=0.569 visibility=0.029 lighting=0.017 draw=0.707 end_present=1.702",
        "CPU animation ms: poses=0.492 skin_bounds=0.051",
        "GPU full render: samples=359 mean=2.987 p50=2.979 p95=3.210 p99=3.217 max=3.227 ms",
        "Process memory: private=1413.57 MiB working_set=632.63 MiB peak_working_set=964.39 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.585 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 558 helped by the waiting thread, peak queue 134, 3916091 us executing and 1368799 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.040 queue_present=0.024 acquire=0.002 fence_wait=1.646 ms",
        "Factory benchmark: samples=360 mean=3.277 ms fps=305.17 p50=3.279 p95=3.512 p99=3.526 max=3.537 ms",
        "CPU stages ms: update=0.227 begin=0.019 presentation=0.560 visibility=0.029 lighting=0.017 draw=0.690 end_present=1.736",
        "CPU animation ms: poses=0.484 skin_bounds=0.051",
        "GPU full render: samples=359 mean=2.981 p50=2.979 p95=3.211 p99=3.220 max=3.234 ms",
        "Process memory: private=1413.48 MiB working_set=631.23 MiB peak_working_set=963.79 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.440 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 554 helped by the waiting thread, peak queue 134, 3780905 us executing and 1279886 us waiting."
      ]
    }
  ]
}
```
