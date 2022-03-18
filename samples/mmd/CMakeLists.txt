project(mmd)

add_executable(${PROJECT_NAME}
    ./source/main.cpp
    ./source/pmx_loader.cpp
    ./source/unicode.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ash::ecs
        ash::core
        ash::window
        ash::graphics
        ash::math
        ash::scene)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/config DESTINATION bin)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

if (MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()