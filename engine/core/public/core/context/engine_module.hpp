#pragma once

#include "common/dictionary.hpp"
#include <string_view>

namespace violet
{
class engine_module
{
public:
    engine_module(std::string_view name) noexcept;
    virtual ~engine_module() = default;

    virtual bool initialize(const dictionary& config) { return true; }
    virtual void shutdown() {}

    inline std::string_view get_name() const noexcept { return m_name; }

private:
    std::string m_name;
};
} // namespace violet