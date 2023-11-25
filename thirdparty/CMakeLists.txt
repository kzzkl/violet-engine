# Catch2
set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(Catch2)

# spdlog
add_subdirectory(spdlog)

# nlohmann/json
add_subdirectory(json)

# bullet3
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_PYBULLET OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
add_subdirectory(bullet3)

# stbimage
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ./stb)

# freetype
set(SKIP_INSTALL_ALL ON)
# add_subdirectory(freetype)

# yoga
# add_subdirectory(yoga)