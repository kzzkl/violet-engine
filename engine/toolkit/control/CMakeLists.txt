add_library(violet-control STATIC
    private/control_system.cpp)
add_library(violet::control ALIAS violet-control)

target_include_directories(violet-control
    PUBLIC
        public
    PRIVATE
        private)

target_link_libraries(violet-control
    PUBLIC
        violet::core
        violet::window
        violet::math
        violet::scene)

install(TARGETS violet-control
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)