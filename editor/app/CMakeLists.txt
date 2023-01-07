project(violet-editor)

add_library(${PROJECT_NAME} STATIC
    ./source/editor.cpp)
add_library(violet::editor ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::editor::ui)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()