add_library(violet-common STATIC
    private/allocator.cpp
    private/log.cpp
    private/utility.cpp)
add_library(violet::common ALIAS violet-common)

target_include_directories(violet-common
    PUBLIC
        public)

target_link_libraries(violet-common
    PUBLIC
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        OffsetAllocator)

install(TARGETS violet-common
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)