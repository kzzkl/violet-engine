#pragma once

#include "core/engine_system.hpp"
#include "physics/physics_context.hpp"
#include "physics/physics_world.hpp"

namespace violet
{
class physics_plugin;
class physics_system : public engine_system
{
public:
    physics_system();
    virtual ~physics_system();

    virtual bool initialize(const dictionary& config) override;
    virtual void shutdown() override;

    void simulation(physics_world* world = nullptr, bool immediately = false);

    physics_context* get_context() const noexcept { return m_context.get(); }

private:
    std::vector<physics_world*> m_worlds;
    std::unique_ptr<physics_plugin> m_plugin;
    std::unique_ptr<physics_context> m_context;
};
} // namespace violet