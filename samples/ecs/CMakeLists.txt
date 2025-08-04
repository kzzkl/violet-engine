add_executable(ecs
    source/main.cpp)

target_include_directories(ecs
    PRIVATE
        include)

target_link_libraries(ecs
    PRIVATE
        violet::scene
        violet::window)

install(TARGETS ecs
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

if(MSVC)
    set_property(TARGET ecs PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${VIOLET_BINARY_ROOT_DIR}/)
endif()