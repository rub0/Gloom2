file(MAKE_DIRECTORY "${GLOOM_CAPTURE_DIR}")
execute_process(COMMAND "${GLOOM_EXECUTABLE}" "${GLOOM_CAPTURE_DIR}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 45)
file(WRITE "${GLOOM_CAPTURE_DIR}/render.log" "${stdout}\n${stderr}")
if(NOT result EQUAL 0 OR "${stdout}\n${stderr}" MATCHES "Diligent Engine: ERROR|VUID-|Diligent Engine: Error")
    message(FATAL_ERROR "Material acceptance failed: ${result}. See ${GLOOM_CAPTURE_DIR}/render.log")
endif()
