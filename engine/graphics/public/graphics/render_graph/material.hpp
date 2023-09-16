#pragma once

#include "graphics/render_graph/render_pipeline.hpp"
#include <unordered_map>

namespace violet
{
struct material_field
{
    std::size_t pipeline_index = 0;
    std::size_t field_index = 0;
    std::size_t size = 1;
    std::size_t offset = 0;
};

class material;
class material_layout : public render_node
{
public:
    material_layout(std::string_view name, rhi_context* rhi);

    void add_pipeline(render_pipeline& pipeline);

    void add_field(std::string_view name, const material_field& field)
    {
        m_fields[name.data()] = field;
    }

    const material_field& get_field(std::string_view name) const
    {
        auto iter = m_fields.find(name.data());
        return iter->second;
    }

    material* add_material(std::string_view name);

    const std::vector<render_pipeline*>& get_pipelines() const noexcept { return m_pipelines; }

private:
    std::vector<render_pipeline*> m_pipelines;
    std::unordered_map<std::string, material_field> m_fields;

    std::unordered_map<std::string, std::unique_ptr<material>> m_materials;
};

class material
{
public:
    material(material_layout* layout);
    material(const material&) = delete;
    virtual ~material();

    template <typename T>
    void set(std::string_view name, const T& value)
    {
        auto& field = m_layout->get_field(name);
        m_parameters[field.pipeline_index]
            ->set(field.field_index, &value, field.size, field.offset);
    }

    void set(std::string_view name, rhi_resource* texture, rhi_sampler* sampler);

    template <typename Functor>
    void each_pipeline(Functor functor)
    {
        auto& pipelines = m_layout->get_pipelines();
        for (std::size_t i = 0; i < pipelines.size(); ++i)
            functor(pipelines[i], m_parameters[i]);
    }

    material& operator=(const material&) = delete;

private:
    material_layout* m_layout;

    std::vector<rhi_pipeline_parameter*> m_parameters;
};
} // namespace violet