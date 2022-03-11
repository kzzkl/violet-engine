#pragma once

#include "core_exports.hpp"
#include "dictionary.hpp"
#include "task_manager.hpp"
#include "uuid.hpp"
#include <string_view>
#include <type_traits>

namespace ash::core
{
class context;
class CORE_API submodule
{
public:
    submodule(std::string_view name) noexcept;
    virtual ~submodule() = default;

    virtual bool initialize(const dictionary& config) = 0;

    inline std::string_view get_name() const noexcept { return m_name; }

protected:
    context& get_context();

private:
    friend class context;

    std::string m_name;
    context* m_context;
};

template <typename T>
concept derived_from_submodule = std::is_base_of<submodule, T>::value;

template <derived_from_submodule T>
struct submodule_trait
{
    static constexpr uuid id = T::id;
};
} // namespace ash::core