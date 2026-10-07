```json
{
  "commit": "934b73dd50f9df4cd807646235da5cf79b970ec5",
  "exe_sha256": "4a93f53c10df0e9b59b48b38c2ef2e1134e6d230de006e58149969f7de9fbfa0",
  "profile": false,
  "finished_utc": "2026-10-07T15:53:28.256057+00:00",
  "source_diff_sha256": "5d9aed38087b7c1d21ef6bd45be22225a9dc420240240a47bf161fa16b89b6e3",
  "runs": [
    {
      "case": "factory-1",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.042 queue_present=0.025 acquire=0.002 fence_wait=1.451 ms",
        "Factory benchmark: samples=360 mean=2.294 ms fps=436.01 p50=2.294 p95=2.308 p99=2.315 max=2.318 ms",
        "CPU stages ms: update=0.135 begin=0.018 presentation=0.105 visibility=0.024 lighting=0.018 draw=0.455 end_present=1.538",
        "CPU animation ms: poses=0.075 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.031 p50=2.032 p95=2.038 p99=2.041 max=2.044 ms",
        "Process memory: private=905.01 MiB working_set=390.85 MiB peak_working_set=501.95 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.022 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=12",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1627 completed, 1051 helped by the waiting thread, peak queue 107, 2457120 us executing and 617703 us waiting."
      ]
    },
    {
      "case": "factory-2",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.025 acquire=0.002 fence_wait=1.455 ms",
        "Factory benchmark: samples=360 mean=2.295 ms fps=435.77 p50=2.294 p95=2.311 p99=2.320 max=2.450 ms",
        "CPU stages ms: update=0.140 begin=0.018 presentation=0.105 visibility=0.024 lighting=0.018 draw=0.450 end_present=1.541",
        "CPU animation ms: poses=0.074 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.031 p50=2.032 p95=2.037 p99=2.039 max=2.171 ms",
        "Process memory: private=898.45 MiB working_set=391.85 MiB peak_working_set=525.52 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.031 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1353 completed, 795 helped by the waiting thread, peak queue 107, 2435627 us executing and 807240 us waiting."
      ]
    },
    {
      "case": "factory-3",
      "options": [
        "--vertical-slice-performance-1080p"
      ],
      "evidence": [
        "Frame budget: over_5ms=0/360",
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.043 queue_present=0.026 acquire=0.002 fence_wait=1.440 ms",
        "Factory benchmark: samples=360 mean=2.296 ms fps=435.45 p50=2.296 p95=2.314 p99=2.351 max=2.359 ms",
        "CPU stages ms: update=0.139 begin=0.018 presentation=0.106 visibility=0.026 lighting=0.018 draw=0.462 end_present=1.528",
        "CPU animation ms: poses=0.075 skin_bounds=0.003",
        "GPU full render: samples=359 mean=2.033 p50=2.034 p95=2.040 p99=2.043 max=2.051 ms",
        "Process memory: private=896.26 MiB working_set=391.63 MiB peak_working_set=531.53 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=3 peak_maps=3 reserved=147456 copied=16512 bytes/frame; shadow_draws=244",
        "Animated TPS mask: 0; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=249 visible=96 batches=36 draws=340 skinned=2 visible_skinned=0 GPU_last_passes=2.031 ms",
        "GPU asset residency: 130.24 MiB; evictions=0 budget_limited_frames=13",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "Jobs: 1312 completed, 739 helped by the waiting thread, peak queue 107, 2513161 us executing and 842881 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.042 queue_present=0.024 acquire=0.002 fence_wait=1.559 ms",
        "Factory benchmark: samples=360 mean=3.237 ms fps=308.97 p50=3.219 p95=3.449 p99=3.460 max=3.466 ms",
        "CPU stages ms: update=0.226 begin=0.019 presentation=0.582 visibility=0.032 lighting=0.018 draw=0.708 end_present=1.652",
        "CPU animation ms: poses=0.502 skin_bounds=0.052",
        "GPU full render: samples=359 mean=2.976 p50=2.954 p95=3.184 p99=3.189 max=3.199 ms",
        "Process memory: private=1411.20 MiB working_set=630.22 MiB peak_working_set=962.77 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.425 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 564 helped by the waiting thread, peak queue 134, 3905164 us executing and 1339763 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.042 queue_present=0.024 acquire=0.002 fence_wait=1.394 ms",
        "Factory benchmark: samples=360 mean=3.236 ms fps=309.07 p50=3.221 p95=3.452 p99=3.473 max=3.586 ms",
        "CPU stages ms: update=0.223 begin=0.019 presentation=0.761 visibility=0.030 lighting=0.017 draw=0.698 end_present=1.487",
        "CPU animation ms: poses=0.684 skin_bounds=0.051",
        "GPU full render: samples=359 mean=2.975 p50=2.956 p95=3.189 p99=3.195 max=3.210 ms",
        "Process memory: private=1409.07 MiB working_set=631.88 MiB peak_working_set=963.93 MiB",
        "Audio output: null; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.423 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 556 helped by the waiting thread, peak queue 134, 3859534 us executing and 1315105 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.040 queue_present=0.024 acquire=0.002 fence_wait=1.589 ms",
        "Factory benchmark: samples=360 mean=3.238 ms fps=308.80 p50=3.221 p95=3.455 p99=3.476 max=3.582 ms",
        "CPU stages ms: update=0.217 begin=0.018 presentation=0.597 visibility=0.028 lighting=0.017 draw=0.683 end_present=1.678",
        "CPU animation ms: poses=0.523 skin_bounds=0.050",
        "GPU full render: samples=359 mean=2.976 p50=2.955 p95=3.186 p99=3.194 max=3.315 ms",
        "Process memory: private=1411.41 MiB working_set=631.10 MiB peak_working_set=966.17 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.413 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 566 helped by the waiting thread, peak queue 134, 3852933 us executing and 1325647 us waiting."
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
        "Swapchain: effective_mode=0 images=2; CPU mean flush=0.041 queue_present=0.024 acquire=0.002 fence_wait=1.548 ms",
        "Factory benchmark: samples=360 mean=3.239 ms fps=308.72 p50=3.226 p95=3.455 p99=3.472 max=3.555 ms",
        "CPU stages ms: update=0.224 begin=0.019 presentation=0.614 visibility=0.029 lighting=0.017 draw=0.696 end_present=1.639",
        "CPU animation ms: poses=0.538 skin_bounds=0.050",
        "GPU full render: samples=359 mean=2.979 p50=2.959 p95=3.191 p99=3.198 max=3.315 ms",
        "Process memory: private=1405.62 MiB working_set=628.79 MiB peak_working_set=965.33 MiB",
        "Audio output: SDL device; drawable=1920x1080; windowed",
        "Skin uploads: maps=9 peak_maps=9 reserved=442368 copied=81408 bytes/frame; shadow_draws=464",
        "Animated TPS mask: 127; measured evictions=0 budget_limited_frames=0",
        "Factory render: 1920x1080 output=1920x1080 instances=288 visible=155 batches=51 draws=619 skinned=53 visible_skinned=53 GPU_last_passes=2.870 ms",
        "GPU asset residency: 345.26 MiB; evictions=0 budget_limited_frames=30",
        "Texture compression: BC5/BC7; measured missing meshes=0 textures=0",
        "H07 v17 probe: game:/h07-v17/hound.gltf; forced TPS LOD=0; weapon_mask=31",
        "Jobs: 1609 completed, 568 helped by the waiting thread, peak queue 134, 3859769 us executing and 1296175 us waiting."
      ]
    }
  ]
}
```
