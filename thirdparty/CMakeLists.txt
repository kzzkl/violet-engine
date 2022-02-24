# Catch2
make_directory(${CMAKE_CURRENT_SOURCE_DIR}/build/Catch2)
execute_process(
    COMMAND ${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/Catch2
    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/install/Catch2
    -DCATCH_BUILD_TESTING=OFF
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/Catch2)
execute_process(COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_SOURCE_DIR}/build/Catch2 --target install)

# spdlog
make_directory(${CMAKE_CURRENT_SOURCE_DIR}/build/spdlog)
execute_process(
    COMMAND ${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/spdlog
    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/install/spdlog
    #-DSPDLOG_BUILD_SHARED=ON
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/spdlog)
execute_process(COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_SOURCE_DIR}/build/spdlog --target install)

# nlohmann/json
make_directory(${CMAKE_CURRENT_SOURCE_DIR}/build/json)
execute_process(
    COMMAND ${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/json
    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/install/json
    -DJSON_BuildTests=OFF
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/json)
execute_process(COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_SOURCE_DIR}/build/json --config Release --target install)