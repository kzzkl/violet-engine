#include "context.hpp"
#include "editor_component.hpp"
#include "editor_data.hpp"
#include "link.hpp"
#include "relation.hpp"

namespace ash::editor
{
class editor_layout : public core::system_base
{
public:
    editor_layout();

    virtual bool initialize(const dictionary& config) override;
    void draw();

private:
    void initialize_layout();

    template <typename T, typename... Args>
    T* create_view(std::string_view name, ecs::entity parent, Args&&... args)
    {
        auto& world = system<ecs::world>();
        ecs::entity view = world.create(name);
        world.add<core::link, editor_ui>(view);

        auto& ui = world.component<editor_ui>(view);
        ui.show = true;

        auto interface = std::make_unique<T>(std::forward<Args>(args)...);
        auto result = interface.get();
        ui.interface = std::move(interface);

        auto& relation = system<core::relation>();
        relation.link(view, parent);

        return result;
    }

    ecs::entity m_ui_root;
    editor_data m_data;
};
} // namespace ash::editor