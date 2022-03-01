project(test-plugin)

find_package(Catch2 REQUIRED PATHS ${THIRD_PARTY_DIR}/Catch2)

add_executable(${PROJECT_NAME}
    ./source/test_common.cpp
    ./source/test_main.cpp
    ./source/test_plugin.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ./include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ash::core
        Catch2::Catch2)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)