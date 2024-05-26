#pragma once

#include "common/type_index.hpp"
#include <map>
#include <string>

namespace violet
{
class theme_manager
{
private:
    struct theme_index : public type_index<theme_index, std::size_t>
    {
    };

    class theme_map_base
    {
    public:
        virtual ~theme_map_base() = default;
    };

    template <typename T>
    class theme_map : public theme_map_base
    {
    public:
        using theme_type = T;
        std::map<std::string, theme_type> themes;
    };

public:
    template <typename T>
    void register_theme(std::string_view name, const T& theme)
    {
        std::size_t index = theme_index::value<T>();
        if (m_theme_maps.size() <= index)
            m_theme_maps.resize(index + 1);

        if (m_theme_maps[index] == nullptr)
            m_theme_maps[index] = std::make_unique<theme_map<T>>();

        auto map = static_cast<theme_map<T>*>(m_theme_maps[index].get());
        map->themes[name.data()] = theme;
    }

    template <typename T>
    const T& theme(std::string_view name)
    {
        std::size_t index = theme_index::value<T>();
        auto map = static_cast<theme_map<T>*>(m_theme_maps[index].get());

        auto iter = map->themes.find(name.data());
        return iter->second;
    }

private:
    std::vector<std::unique_ptr<theme_map_base>> m_theme_maps;
};
} // namespace violet