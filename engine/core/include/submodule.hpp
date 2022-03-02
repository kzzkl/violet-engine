#pragma once

#include "core_exports.hpp"
#include "dictionary.hpp"
#include "task_manager.hpp"
#include <string_view>
#include <type_traits>

namespace ash::core
{
class context;
class CORE_API submodule
{
public:
    submodule(std::string_view name);
    virtual ~submodule() = default;

    virtual bool initialize(const ash::common::dictionary& config) = 0;

    std::string_view get_name() const { return m_name; }

protected:
    context& get_context();

private:
    friend class context;

    std::string m_name;
    context* m_context;
};

using submodule_index = uint32_t;

namespace detail
{
submodule_index CORE_API next_submodule_index();
}

template <typename T>
concept derived_from_submodule = std::is_base_of<submodule, T>::value;

template <derived_from_submodule T>
struct submodule_trait
{
    static submodule_index index()
    {
        static submodule_index index = detail::next_submodule_index();
        return index;
    }
};
} // namespace ash::core