add_library(violet-resource STATIC
    private/resource_manager.cpp)
add_library(violet::resource ALIAS violet-resource)

target_include_directories(violet-resource
    PUBLIC
        public)

target_link_libraries(violet-resource
    PUBLIC
        violet::core)

install(TARGETS violet-resource
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)