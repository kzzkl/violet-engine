project(test-ecs)

find_package(Catch2 REQUIRED PATHS ${THIRD_PARTY_DIR}/Catch2)

add_executable(${PROJECT_NAME}
    ./source/test_archetype.cpp
    ./source/test_component.cpp
    ./source/test_main.cpp
    ./source/test_storage.cpp
    ./source/test_world.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ./include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ash::ecs
        Catch2::Catch2)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)