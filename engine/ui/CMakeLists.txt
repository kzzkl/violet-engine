project(ash-ui)

add_library(${PROJECT_NAME} STATIC
    ./source/element_tree.cpp
    ./source/font.cpp
    ./source/ui_pipeline.cpp
    ./source/ui.cpp)
add_library(ash::ui ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::common
        ash::core
        ash::graphics
    PRIVATE
        freetype)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)