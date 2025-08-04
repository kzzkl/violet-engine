add_library(violet-math INTERFACE)
add_library(violet::math ALIAS violet-math)

target_include_directories(violet-math
    INTERFACE
        public)

install(TARGETS violet-math
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)