add_library(violet-algorithm STATIC
    private/city_hash.cpp
    private/hash.cpp)
add_library(violet::algorithm ALIAS violet-algorithm)

target_include_directories(violet-algorithm
    PUBLIC
        public)

install(TARGETS violet-algorithm
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)