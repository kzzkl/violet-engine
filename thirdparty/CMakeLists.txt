#assimp
set(BUILD_SHARED_LIBS OFF)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_INSTALL_PDB OFF)
add_subdirectory(assimp)

#dxc
add_library(dxcompiler STATIC IMPORTED GLOBAL)
target_include_directories(dxcompiler INTERFACE ${VIOLET_THIRDPARTY_DIR}/dxc/inc)
set_target_properties(dxcompiler PROPERTIES IMPORTED_LOCATION ${VIOLET_THIRDPARTY_DIR}/dxc/lib/x64/dxcompiler.lib)
set_target_properties(dxcompiler PROPERTIES IMPORTED_IMPLIB  ${VIOLET_THIRDPARTY_DIR}/dxc/lib/x64/dxcompiler.lib)

install(
    FILES
    ${VIOLET_THIRDPARTY_DIR}/dxc/bin/x64/dxcompiler.dll
    ${VIOLET_THIRDPARTY_DIR}/dxc/bin/x64/dxil.dll
    DESTINATION bin)

# Catch2
set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(Catch2)

# spdlog
add_subdirectory(spdlog)

# nlohmann/json
add_subdirectory(json)

# METIS
add_library(metis STATIC IMPORTED GLOBAL)
target_include_directories(metis INTERFACE ${VIOLET_THIRDPARTY_DIR}/metis/include)
set_target_properties(metis PROPERTIES IMPORTED_LOCATION ${VIOLET_THIRDPARTY_DIR}/metis/lib/metis.lib)
set_target_properties(metis PROPERTIES IMPORTED_IMPLIB  ${VIOLET_THIRDPARTY_DIR}/metis/lib/metis.lib)

# MikkTSpace
add_library(MikkTSpace STATIC
    ./MikkTSpace/mikktspace.c)
target_include_directories(MikkTSpace PUBLIC ./MikkTSpace)

# OffsetAllocator
add_library(OffsetAllocator STATIC
    ./OffsetAllocator/offsetAllocator.cpp)
target_include_directories(OffsetAllocator PUBLIC ./OffsetAllocator)

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
add_subdirectory(freetype)

# volk
if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()
add_subdirectory(volk)

# vma
set(VMA_STATIC_VULKAN_FUNCTIONS OFF)
set(VMA_DYNAMIC_VULKAN_FUNCTIONS OFF)
add_subdirectory(VulkanMemoryAllocator)

# xxHash
add_library(xxHash STATIC
   xxHash/xxhash.c)
target_include_directories(xxHash
    PUBLIC
        xxHash)

# yoga
if(MSVC)
  add_compile_options(/wd4819)
endif()
add_subdirectory(yoga/yoga)