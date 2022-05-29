include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Catch2
set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_Declare(Catch2
    GIT_REPOSITORY  git@github.com:catchorg/Catch2.git
    GIT_TAG         v2.13.8
    SOURCE_DIR      ${ASH_THIRD_PARTY_DIR}/Catch2)
FetchContent_MakeAvailable(Catch2)

# spdlog
FetchContent_Declare(spdlog
    GIT_REPOSITORY  git@github.com:gabime/spdlog.git
    GIT_TAG         v1.9.2
    SOURCE_DIR      ${ASH_THIRD_PARTY_DIR}/spdlog)
FetchContent_MakeAvailable(spdlog)

# nlohmann/json
FetchContent_Declare(json
    GIT_REPOSITORY  git@github.com:nlohmann/json.git
    GIT_TAG         v3.10.5
    SOURCE_DIR      ${ASH_THIRD_PARTY_DIR}/json)
FetchContent_MakeAvailable(json)

# bullet3
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_PYBULLET OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(bullet3
    GIT_REPOSITORY  git@github.com:bulletphysics/bullet3.git
    GIT_TAG         3.22
    SOURCE_DIR      ${ASH_THIRD_PARTY_DIR}/bullet3)
FetchContent_MakeAvailable(bullet3)

# stbimage
FetchContent_Declare(stb
    GIT_REPOSITORY  git@github.com:nothings/stb.git
    GIT_TAG         master
    SOURCE_DIR      ${ASH_THIRD_PARTY_DIR}/stb)
FetchContent_MakeAvailable(stb)

# freetype
FetchContent_Declare(freetype
    GIT_REPOSITORY  git@github.com:freetype/freetype.git
    GIT_TAG         VER-2-12-1
    SOURCE_DIR      ${ASH_THIRD_PARTY_DIR}/freetype)
FetchContent_MakeAvailable(freetype)

# yoga
FetchContent_Declare(yoga
    GIT_REPOSITORY  git@github.com:facebook/yoga.git
    GIT_TAG         v1.19.0
    SOURCE_DIR      ${ASH_THIRD_PARTY_DIR}/yoga)
FetchContent_MakeAvailable(yoga)