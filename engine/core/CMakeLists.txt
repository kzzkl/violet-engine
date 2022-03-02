project(ash-core)

add_library(${PROJECT_NAME} SHARED
    ./source/application.cpp
    ./source/context.cpp
    ./source/plugin.cpp
    ./source/submodule.cpp)
add_library(ash::core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include
        ./interface)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::common
        ash::task)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
    EXPORT_MACRO_NAME CORE_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/core_exports.hpp)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)