project(ash-editor)

add_executable(${PROJECT_NAME}
    ./source/editor.cpp
    ./source/main.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ./include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ash::editor::ui)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

if (MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()