add_executable(hello-world
    source/main.cpp)

target_include_directories(hello-world
    PRIVATE
        include)

target_link_libraries(hello-world
    PRIVATE
        violet::graphics
        violet::control)

install(TARGETS hello-world
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/assets)

if(MSVC)
    set_property(TARGET hello-world PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()