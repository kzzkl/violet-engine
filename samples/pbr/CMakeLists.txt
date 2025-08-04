add_executable(pbr
    source/main.cpp)

target_include_directories(pbr
    PRIVATE
        include)

target_link_libraries(pbr
    PRIVATE
        violet::graphics
        violet::control
        violet::sample-base)

install(TARGETS pbr
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/assets)

if(MSVC)
    set_property(TARGET pbr PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()