#include "graphics/material.hpp"
#include "graphics/resources/texture.hpp"

namespace violet
{
struct gf2_material_base_constant
{
    std::uint32_t diffuse_texture;
    std::uint32_t normal_texture;
    std::uint32_t rmo_texture;
    std::uint32_t ramp_texture;
    std::uint32_t brdf_lut;
};

class gf2_material_base : public mesh_material<gf2_material_base_constant>
{
public:
    gf2_material_base();

    void set_diffuse(const texture_2d* texture);
    void set_normal(const texture_2d* texture);
    void set_rmo(const texture_2d* texture);
    void set_ramp(const texture_2d* texture);
};

struct gf2_material_face_constant
{
    std::uint32_t diffuse_texture;
    std::uint32_t sdf_texture;
    std::uint32_t ramp_texture;
    std::uint32_t brdf_lut;

    vec3f face_front_dir;
    std::uint32_t padding0;
    vec3f face_left_dir;
    std::uint32_t padding1;
};

class gf2_material_face : public mesh_material<gf2_material_face_constant>
{
public:
    gf2_material_face();

    void set_diffuse(const texture_2d* texture);
    void set_sdf(const texture_2d* texture);
    void set_ramp(const texture_2d* texture);

    void set_face_dir(const vec3f& face_front_dir, const vec3f& face_left_dir);
};

struct gf2_material_eye_constant
{
    std::uint32_t diffuse_texture;
};

class gf2_material_eye : public mesh_material<gf2_material_eye_constant>
{
public:
    gf2_material_eye();

    void set_diffuse(const texture_2d* texture);
};

struct gf2_material_eye_blend_constant
{
    std::uint32_t blend_texture;
};

class gf2_material_eye_blend : public mesh_material<gf2_material_eye_blend_constant>
{
public:
    gf2_material_eye_blend(bool is_add);

    void set_blend(const texture_2d* texture);
};

struct gf2_material_hair_constant
{
    std::uint32_t diffuse_texture;
    std::uint32_t specular_texture;
    std::uint32_t ramp_texture;
    std::uint32_t brdf_lut;
};

class gf2_material_hair : public mesh_material<gf2_material_hair_constant>
{
public:
    gf2_material_hair();

    void set_diffuse(const texture_2d* texture);
    void set_specular(const texture_2d* texture);
    void set_ramp(const texture_2d* texture);
};

struct gf2_material_plush_constant
{
    std::uint32_t diffuse_texture;
    std::uint32_t normal_texture;
    std::uint32_t noise_texture;
    std::uint32_t ramp_texture;
    std::uint32_t brdf_lut;
};

class gf2_material_plush : public mesh_material<gf2_material_plush_constant>
{
public:
    gf2_material_plush();

    void set_diffuse(const texture_2d* texture);
    void set_normal(const texture_2d* texture);
    void set_noise(const texture_2d* texture);
    void set_ramp(const texture_2d* texture);
};
} // namespace violet