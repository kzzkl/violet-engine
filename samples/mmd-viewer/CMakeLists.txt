project(mmd-viewer)

add_executable(${PROJECT_NAME}
    ./source/encode.cpp
    ./source/main.cpp
    ./source/mmd_animation.cpp
    ./source/mmd_ik.cpp
    ./source/mmd_loader.cpp
    ./source/mmd_viewer.cpp
    ./source/pmx_loader.cpp
    ./source/vmd_loader.cpp)

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
        ash::scene
        ash::physics)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/config DESTINATION bin)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

if (MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()