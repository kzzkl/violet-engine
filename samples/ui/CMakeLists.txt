add_executable(ui
    source/gallery.cpp
    source/main.cpp)

target_include_directories(ui
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(ui
    PRIVATE
        violet::ui)

install(TARGETS ui
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/config DESTINATION bin/ui)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/image DESTINATION bin/ui)