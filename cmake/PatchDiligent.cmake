# Apply the small swapchain policy/profiling patch also to already-populated builds.
find_package(Git REQUIRED)
set(present_patch "${CMAKE_CURRENT_SOURCE_DIR}/cmake/diligent-present.patch")
execute_process(COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${present_patch}"
    WORKING_DIRECTORY "${diligentcore_SOURCE_DIR}" RESULT_VARIABLE already_applied OUTPUT_QUIET ERROR_QUIET)
if(NOT already_applied EQUAL 0)
    execute_process(COMMAND "${GIT_EXECUTABLE}" apply --check "${present_patch}"
        WORKING_DIRECTORY "${diligentcore_SOURCE_DIR}" COMMAND_ERROR_IS_FATAL ANY)
    execute_process(COMMAND "${GIT_EXECUTABLE}" apply "${present_patch}"
        WORKING_DIRECTORY "${diligentcore_SOURCE_DIR}" COMMAND_ERROR_IS_FATAL ANY)
endif()
target_include_directories(Diligent-GraphicsEngineVk-static PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
