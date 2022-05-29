project(test-math)

add_executable(${PROJECT_NAME}
    ./source/test_common.cpp
    ./source/test_main.cpp
    ./source/test_matrix.cpp
    ./source/test_misc.cpp
    ./source/test_quaternion.cpp
    ./source/test_simd.cpp
    ./source/test_vector.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ./include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ash::math
        Catch2::Catch2)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

if (MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()