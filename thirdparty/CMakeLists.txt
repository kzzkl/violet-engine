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

# volk
if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()
add_subdirectory(volk)

# vma
set(VMA_STATIC_VULKAN_FUNCTIONS OFF)
set(VMA_DYNAMIC_VULKAN_FUNCTIONS OFF)
add_subdirectory(VulkanMemoryAllocator)

# yoga
# add_subdirectory(yoga)

function(set_thirdparty_folder DIR)
    get_property(TARGETS DIRECTORY "${DIR}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(TARGET IN LISTS TARGETS)
        set_target_properties(${TARGET} PROPERTIES FOLDER "thirdparty")
    endforeach()

    get_property(SUBDIRS DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
    foreach(SUBDIR IN LISTS SUBDIRS)
        set_thirdparty_folder("${SUBDIR}")
    endforeach()
endfunction()

set_thirdparty_folder(.)