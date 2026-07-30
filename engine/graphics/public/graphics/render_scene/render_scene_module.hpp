#pragma once

#include "graphics/shader.hpp"

namespace violet
{
class render_scene_camera;
class render_scene_mesh;
class render_scene_light;
class render_scene_shadow;
class render_scene_environment;

struct render_scene_context
{
    render_scene_camera* camera_module;
    render_scene_mesh* mesh_module;
    render_scene_light* light_module;
    render_scene_shadow* shadow_module;
    render_scene_environment* environment_module;

    shader::scene_data scene_data;
};

class gpu_buffer_uploader;

class render_scene_module
{
public:
    virtual ~render_scene_module() = default;

    virtual void update(render_scene_context& context, gpu_buffer_uploader& uploader) {}
    virtual void reset() {}
};
} // namespace violet