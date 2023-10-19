#pragma once

#include "core/engine_system.hpp"
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

    void simulation(physics_world* world);

    pei_plugin* get_pei() const noexcept;

private:
    void simulation();

    std::vector<physics_world*> m_worlds;
    std::unique_ptr<physics_plugin> m_plugin;
};
} // namespace violet