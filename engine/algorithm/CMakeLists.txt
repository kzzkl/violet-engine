add_library(violet-algorithm STATIC
    private/hash.cpp)
add_library(violet::algorithm ALIAS violet-algorithm)

target_include_directories(violet-algorithm
    PUBLIC
        public)

target_link_libraries(violet-algorithm
    PRIVATE
        xxHash)

install(TARGETS violet-algorithm
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)