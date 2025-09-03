add_executable(cluster
    source/cluster_material.cpp
    source/cluster_renderer.cpp
    source/main.cpp)

target_include_directories(cluster
    PRIVATE
        include)

target_link_libraries(cluster
    PRIVATE
        violet::graphics
        violet::control
        violet::sample-base)

install(TARGETS cluster
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/assets)

if(MSVC)
    set_property(TARGET cluster PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()