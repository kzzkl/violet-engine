#pragma once

#include "rhi/rhi.hpp"
#include "rhi/vulkan/vk_traits.hpp"
#include <algorithm>
#include <list>
#include <set>
#include <string>
#include <vector>

namespace violet::vk
{
class vk_structure_chain
{
public:
    template <typename T>
    void append(const T& structure)
    {
        if (has<T>())
        {
            return;
        }

        std::vector<std::uint8_t> data(sizeof(T));
        std::memcpy(data.data(), &structure, sizeof(T));
        m_chain.emplace_back(std::move(data));

        auto* next = reinterpret_cast<VkBaseOutStructure*>(m_chain.back().data());
        next->sType = vk_structure_traits<T>::type;

        if (m_chain.size() > 1)
        {
            auto iter = std::prev(m_chain.end(), 2);
            auto* prev = reinterpret_cast<VkBaseOutStructure*>(iter->data());
            prev->pNext = next;
        }
    }

    VkBaseOutStructure* get_head()
    {
        return m_chain.empty() ? nullptr :
                                 reinterpret_cast<VkBaseOutStructure*>(m_chain.front().data());
    }

    const VkBaseOutStructure* get_head() const
    {
        return m_chain.empty() ?
                   nullptr :
                   reinterpret_cast<const VkBaseOutStructure*>(m_chain.front().data());
    }

    template <typename T>
    T* get()
    {
        VkBaseOutStructure* head = get_head();
        while (head != nullptr)
        {
            if (head->sType == vk_structure_traits<T>::type)
            {
                return reinterpret_cast<T*>(head);
            }

            head = head->pNext;
        }

        return nullptr;
    }

    template <typename T>
    const T* get() const
    {
        const VkBaseOutStructure* head = get_head();
        while (head != nullptr)
        {
            if (head->sType == vk_structure_traits<T>::type)
            {
                return reinterpret_cast<const T*>(head);
            }

            head = head->pNext;
        }

        return nullptr;
    }

    template <typename T>
    bool has()
    {
        return get<T>() != nullptr;
    }

    void clear()
    {
        m_chain.clear();
    }

private:
    std::list<std::vector<std::uint8_t>> m_chain;
};

class vk_extension_registry
{
public:
    bool check_layer(const char* layer) const
    {
        return m_available_layers.contains(layer);
    }

    bool check_layers(const std::vector<const char*>& layers) const
    {
        return std::ranges::all_of(
            layers,
            [this](const char* layer)
            {
                return check_layer(layer);
            });
    }

    void request_layer(const char* layer)
    {
        m_requested_layers.insert(layer);
    }

    void request_layers(const std::vector<const char*>& layers)
    {
        for (const auto& layer : layers)
        {
            request_layer(layer);
        }
    }

    std::vector<const char*> get_equested_layers() const
    {
        std::vector<const char*> layers;
        layers.reserve(m_requested_layers.size());
        for (const auto& layer : m_requested_layers)
        {
            layers.push_back(layer.c_str());
        }
        return layers;
    }

    bool check_instance_extension(const char* extension) const
    {
        return m_available_instance_extensions.contains(extension);
    }

    bool check_instance_extensions(const std::vector<const char*>& extensions) const
    {
        return std::ranges::all_of(
            extensions,
            [this](const char* extension)
            {
                return check_instance_extension(extension);
            });
    }

    void request_instance_extension(const char* extension)
    {
        m_requested_instance_extensions.insert(extension);
    }

    void request_instance_extensions(const std::vector<const char*>& extensions)
    {
        for (const auto& extension : extensions)
        {
            request_instance_extension(extension);
        }
    }

    std::vector<const char*> get_requested_instance_extensions() const
    {
        std::vector<const char*> extensions;
        extensions.reserve(m_requested_instance_extensions.size());
        for (const auto& extension : m_requested_instance_extensions)
        {
            extensions.push_back(extension.c_str());
        }
        return extensions;
    }

    bool check_device_extension(const char* extension) const
    {
        return m_available_device_extensions.contains(extension);
    }

    bool check_device_extensions(const std::vector<const char*>& extensions) const
    {
        return std::ranges::all_of(
            extensions,
            [this](const char* extension)
            {
                return check_device_extension(extension);
            });
    }

    void request_device_extension(const char* extension)
    {
        m_requested_device_extensions.insert(extension);
    }

    std::vector<const char*> get_equested_device_extensions() const
    {
        std::vector<const char*> extensions;
        extensions.reserve(m_requested_device_extensions.size());
        for (const auto& extension : m_requested_device_extensions)
        {
            extensions.push_back(extension.c_str());
        }
        return extensions;
    }

    template <typename T>
    void add_feature()
    {
        m_available_features.append(T{});
        m_requested_features.append(T{});
    }

    template <typename T, typename Functor>
    void set_feature(Functor&& functor)
    {
        const T& available_feature = *m_available_features.get<T>();
        T& requested_feature = *m_requested_features.get<T>();

        functor(available_feature, requested_feature);
    }

    template <typename T>
    const T& get_available_feature() const
    {
        return *m_available_features.get<T>();
    }

    template <typename T>
    T& get_requested_feature()
    {
        return *m_requested_features.get<T>();
    }

private:
    std::set<std::string> m_available_layers;
    std::set<std::string> m_requested_layers;

    std::set<std::string> m_available_instance_extensions;
    std::set<std::string> m_requested_instance_extensions;

    std::set<std::string> m_available_device_extensions;
    std::set<std::string> m_requested_device_extensions;

    vk_structure_chain m_available_features;
    vk_structure_chain m_requested_features;

    friend class vk_context;
};

class vk_extension : public rhi_extension
{
public:
    bool is_enabled() const override
    {
        return m_enabled;
    }

    virtual bool evaluate_for_instance(const vk_extension_registry& registry) const
    {
        return true;
    }

    virtual void commit_for_instance(vk_extension_registry& registry) {}

    virtual bool evaluate_for_device(const vk_extension_registry& registry) const
    {
        return true;
    }

    virtual void commit_for_device(vk_extension_registry& registry) {}

    virtual void add_feature_query(vk_extension_registry& registry) const {}

private:
    bool m_enabled{true};

    friend class vk_context;
};
} // namespace violet::vk