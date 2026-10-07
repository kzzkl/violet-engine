#ifndef SURFACE_CACHE_COMMON_HLSLI
#define SURFACE_CACHE_COMMON_HLSLI

static const uint SURFACE_CACHE_FACE_COUNT = 6;

static const uint SURFACE_CACHE_PAGE_RESOLUTION = 128;
static const uint SURFACE_CACHE_FACE_MAX_RESOLUTION = 512;
static const uint SURFACE_CACHE_MAX_PAGES_PER_FACE =
    SURFACE_CACHE_FACE_MAX_RESOLUTION / SURFACE_CACHE_PAGE_RESOLUTION;

static const float SURFACE_CACHE_ATLAS_RESOLUTION = 4096.0;

// The range of the orthographic projection the surface cache is baked with, every depth of the
// surface cache is a distance divided by this range.
static const float SURFACE_CACHE_DEPTH_RANGE = 100.0;

// One face of a surface cache. Six consecutive records belong to a mesh, the first of them starts
// at mesh.surface_cache_id.
//
// extent and page_count pack two 16 bit values, use the accessors instead of the fields. pages
// holds the atlas position of the top left texel of every page of the face, a face is not
// contiguous in the atlas and when it shares a page with other faces there is a single page and
// pages[0] is already the position of this face inside that page.
struct surface_cache
{
    uint extent;
    uint page_count;
    uint pages[SURFACE_CACHE_MAX_PAGES_PER_FACE];

    // Texture size of the face.
    uint2 get_extent()
    {
        return uint2(extent & 0xFFFF, extent >> 16);
    }

    // Number of pages of the face along each axis.
    uint2 get_page_count()
    {
        return uint2(page_count & 0xFFFF, page_count >> 16);
    }

    // Atlas position of the top left texel of a page.
    uint2 get_page(uint index)
    {
        return uint2(pages[index] & 0xFFFF, pages[index] >> 16);
    }
};

#endif
