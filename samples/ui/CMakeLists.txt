project(ui)

add_executable(${PROJECT_NAME}
    ./source/button_page.cpp
    ./source/docking_page.cpp
    ./source/gallery.cpp
    ./source/image_page.cpp
    ./source/main.cpp
    ./source/page.cpp
    ./source/scroll_page.cpp
    ./source/tree_page.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ash::ui)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource DESTINATION bin)

if (MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()