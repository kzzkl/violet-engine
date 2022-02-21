project(ash-common)

find_package(spdlog REQUIRED PATHS ${THIRD_PARTY_DIR}/spdlog)
find_package(nlohmann_json REQUIRED PATHS ${THIRD_PARTY_DIR}/json)

add_library(${PROJECT_NAME} SHARED
    ./source/common.cpp
    ./source/log.cpp)
add_library(ash::common ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        spdlog::spdlog
        nlohmann_json::nlohmann_json)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
    EXPORT_MACRO_NAME COMMON_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/common_exports.hpp)

install(DIRECTORY ./include/ DESTINATION include/ash)
# install(FILES ${THIRD_PARTY_DIR}/spdlog/bin/spdlog.dll DESTINATION bin)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)