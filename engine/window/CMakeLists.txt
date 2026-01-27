add_library(violet-window STATIC
    private/input.cpp
    private/window_system.cpp
    private/window_win32.cpp)
add_library(violet::window ALIAS violet-window)

target_include_directories(violet-window
    PUBLIC
        public
    PRIVATE
        private)

target_link_libraries(violet-window
    PUBLIC
        violet::core
        violet::math)

install(TARGETS violet-window
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)