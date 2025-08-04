add_executable(mesh-simplifier
    source/main.cpp)

target_include_directories(mesh-simplifier
    PRIVATE
        include)

target_link_libraries(mesh-simplifier
    PRIVATE
        violet::graphics
        violet::control
        violet::sample-base)

install(TARGETS mesh-simplifier
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/assets)

if(MSVC)
    set_property(TARGET mesh-simplifier PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()