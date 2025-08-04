add_library(violet-animation STATIC
    private/animation_system.cpp)
add_library(violet::animation ALIAS violet-animation)

target_include_directories(violet-animation
    PUBLIC
        public
    PRIVATE
        private)

target_link_libraries(violet-animation
    PUBLIC
        violet::graphics)

install(TARGETS violet-animation
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)