add_library(violet-tools STATIC
    private/cluster/cluster_builder.cpp
    private/cluster/graph_partitioner.cpp
    private/mesh_simplifier/mesh_simplifier.cpp
    private/mesh_simplifier/quadric.cpp
    private/dds.cpp
    private/geometry_tool.cpp
    private/texture_tool.cpp)
add_library(violet::tools ALIAS violet-tools)

target_include_directories(violet-tools
    PUBLIC
        public
    PRIVATE
        private)

target_link_libraries(violet-tools
    PUBLIC
        violet::graphics
    PRIVATE
        metis
        MikkTSpace
        stb
        CMP_Compressonator)

install(TARGETS violet-tools
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)