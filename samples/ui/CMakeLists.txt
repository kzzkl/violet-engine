project(ui)

add_executable(${PROJECT_NAME}
    ./source/gallery.cpp
    ./source/main.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        violet::ui)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/config DESTINATION bin/ui)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/image DESTINATION bin/ui)