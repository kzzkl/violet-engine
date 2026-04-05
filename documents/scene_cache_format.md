# scene cache

```
struct block
{
    // BLOCK_NODE = 0,
    // BLOCK_GEOMETRY = 1,
    // BLOCK_MATERIAL = 2,
    // BLOCK_TEXTURE = 3,
    uint32 block_type;

    char data[block_size];
};

struct scene
{
    uint32 magic_number;

    uint32 block_count;
    block blocks[block_count];
};
```

## node block

```
struct node
{
    uint32 name_length;
    char name[name_length];

    vec3f position;
    vec4f rotation;
    vec3f scale;

    int32 parent;
    int32 mesh;
};

struct submesh
{
    uint submesh_index;
    uint material;
};

struct mesh
{
    uint geometry;
    uint submesh_count;
    submesh submeshes[submesh_count];
};

struct node_block
{
    uint32 node_count;
    node nodes[node_count];
    uint32 mesh_count;
    mesh meshes[mesh_count];
};
```

## geometry block

```
struct cluster
{
    uint32 index_offset;
    uint32 index_count;
    vec4f bounding_sphere;
    vec4f lod_bounds;
    float lod_error;
    vec4f parent_lod_bounds;
    float parent_lod_error;
    uint32 lod;
};

struct cluster_node
{
    vec4f bounding_sphere;
    vec4f lod_bounds;
    float min_lod_error;
    float max_parent_lod_error;
    uint32 is_leaf;
    uint32 depth;
    uint32 child_offset;
    uint32 child_count;
};

struct submesh
{
    uint32 type;

    if (type == 0)
    {
        uint32 vertex_offset;
        uint32 index_offset;
        uint32 index_count;
    }
    else
    {
        uint32 cluster_count;
        cluster clusters[cluster_count];

        uint32 cluster_node_count;
        cluster cluster_node[cluster_node_count];
    }
};

struct geometry
{
    uint32 vertex_count;
    uint32 index_count;

    // CLUSTER_HAS_NORMAL = 1 << 0
    // CLUSTER_HAS_TANGENT = 1 << 1
    // CLUSTER_HAS_TEXCOORD = 1 << 2
    uint32 flags;

    vec3f positions[vertex_count];
    uint32 indexes[index_count];

    if (flags & CLUSTER_HAS_NORMAL)
    {
        vec3f normals[vertex_count];
    }

    if (flags & CLUSTER_HAS_TANGENT)
    {
        vec3f tangents[vertex_count];
    }

    if (flags & CLUSTER_HAS_TEXCOORD)
    {
        vec2f texcoords[vertex_count];
    }

    uint32 submesh_count;
    submesh submeshes[submesh_count];
};

struct geometry_block
{
    uint geometry_count;
    geometry geometries[geometry_count];
};
```

## material block

```
struct material
{
    vec3f albedo;
    float roughness;
    float metallic;
    vec3f emissive;

    int32 albedo_texture;
    int32 roughness_metallic_texture;
    int32 emissive_texture;
    int32 normal_texture;

    uint32 cull_mode; // rhi_cull_mode
    float opacity_cutoff;
};

struct material_block
{
    uint material_count;
    material materials[material_count];
};
```

## texture block

```
struct texture
{
    uint32 format; // rhi_format
    uint32 width;
    uint32 height;

    uint32 layer_count;
    uint32 level_count;

    uint32 data_size;
    char data[data_size];
};

struct texture_block
{
    uint32 texture_count;
    texture textures[texture_count];
};
```