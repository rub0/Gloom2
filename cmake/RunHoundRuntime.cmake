execute_process(COMMAND "${GLOOM_COOKER}" "${GLOOM_SOURCE}/assets" "${GLOOM_BINARY}/content"
    game:/characters/hound_rig/v16/hound-rig.gltf cache:/characters/hound_rig/v16/hound-rig.gasset
    RESULT_VARIABLE cooked OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 60)
if(NOT cooked EQUAL 0)
    message(FATAL_ERROR "Hound cooking failed: ${output}\n${error}")
endif()
execute_process(COMMAND "${GLOOM_EXECUTABLE}" --vertical-slice-performance-hound-eight-1080p --present=immediate
    WORKING_DIRECTORY "${GLOOM_SOURCE}" RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 120)
file(WRITE "${GLOOM_BINARY}/hound-runtime-validation.log" "${output}\n${error}")
if(NOT result EQUAL 0 OR "${output}\n${error}" MATCHES "Diligent Engine: ERROR|VUID-|mapping failed|heap is exhausted")
    message(FATAL_ERROR "Hound runtime failed (${result}): ${output}\n${error}")
endif()
foreach(required IN ITEMS "samples=360" "render: 1920x1080 output=1920x1080" "skinned=53 visible_skinned=53"
    "shadow_draws=[1-9][0-9]*" "Animated TPS mask: 127" "measured missing meshes=0 textures=0")
    if(NOT output MATCHES "${required}")
        message(FATAL_ERROR "Missing Hound runtime evidence: ${required}\n${output}")
    endif()
endforeach()
if(NOT output MATCHES "peak_maps=9 ")
    message(FATAL_ERROR "Skin palettes are no longer shared across primitives/passes: ${output}")
endif()
message(STATUS "Seven animated Hound plus FPS completed 120 warmup + 360 measured frames at 1080p")
