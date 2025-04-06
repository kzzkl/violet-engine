project(mesh-simplifier)

add_executable(${PROJECT_NAME}
    ./source/main.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ./include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    violet::graphics
    violet::control
    violet::sample-base)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/assets)

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()