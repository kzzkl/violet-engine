project("vulkan-demo")

add_executable(${PROJECT_NAME}
    ./source/main.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    violet::graphics)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()