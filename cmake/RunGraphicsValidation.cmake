if(NOT DEFINED GLOOM_EXECUTABLE)
    message(FATAL_ERROR "GLOOM_EXECUTABLE is required")
endif()

execute_process(
    COMMAND "${GLOOM_EXECUTABLE}" --vulkan-sync-stress
    RESULT_VARIABLE result
    OUTPUT_VARIABLE standard_output
    ERROR_VARIABLE standard_error
    TIMEOUT 30
)

set(combined_output "${standard_output}\n${standard_error}")

if(NOT result EQUAL 0)
    message(FATAL_ERROR "Gloom Vulkan stress test failed (${result}):\n${combined_output}")
endif()

if(combined_output MATCHES "Diligent Engine: ERROR|VUID-vkQueueSubmit-pSignalSemaphores-00067")
    message(FATAL_ERROR "Vulkan validation reported an error:\n${combined_output}")
endif()

message(STATUS "Gloom Vulkan synchronization validation passed")
