#include "bt3_shape.hpp"
#include "bt3_world.hpp"
#include "bt3_rigidbody.hpp"

namespace ash::physics::bullet3
{
class bt3_debug_draw : public btIDebugDraw
{
public:
    bt3_debug_draw(debug_draw_interface* debug = nullptr) : m_debug(debug) {}

    void debug(debug_draw_interface* debug) { m_debug = debug; }

    virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
        override
    {
        m_debug->draw_line(convert_vector(from), convert_vector(to), convert_vector(color));
    }

    virtual void drawContactPoint(
        const btVector3& PointOnB,
        const btVector3& normalOnB,
        btScalar distance,
        int lifeTime,
        const btVector3& color) override
    {
    }
    virtual void reportErrorWarning(const char* warningString) override {}
    virtual void draw3dText(const btVector3& location, const char* textString) override {}
    virtual void setDebugMode(int debugMode) override
    {
        m_mode = static_cast<DebugDrawModes>(debugMode);
    }
    virtual int getDebugMode() const override { return m_mode; }

private:
    DebugDrawModes m_mode;
    debug_draw_interface* m_debug;
};

class bt3_factory : public factory
{
public:
    virtual world_interface* make_world(const world_desc& desc) override
    {
        bt3_world* result = new bt3_world(desc);
        if (m_debug)
            result->world()->setDebugDrawer(m_debug.get());
        return result;
    }

    virtual collision_shape_interface* make_collision_shape(
        const collision_shape_desc& desc) override
    {
        return new bt3_shape(desc);
    }

    virtual rigidbody_interface* make_rigidbody(const rigidbody_desc& desc) override
    {
        return new bt3_rigidbody(desc);
    }

    void debug(debug_draw_interface* debug)
    {
        if (m_debug == nullptr)
            m_debug = std::make_unique<bt3_debug_draw>(debug);
        else
            m_debug->debug(debug);
    }

private:
    std::unique_ptr<bt3_debug_draw> m_debug;
};

class bt3_context : public context
{
public:
    bt3_context() {}

    virtual factory_interface* factory() override { return &m_factory; }

    virtual void debug(debug_draw_interface* debug) override { m_factory.debug(debug); }

private:
    bt3_factory m_factory;
};
} // namespace ash::physics::bullet3

extern "C"
{
    PLUGIN_API ash::core::plugin_info get_plugin_info()
    {
        ash::core::plugin_info info = {};

        char name[] = "physics-bullet3";
        memcpy(info.name, name, sizeof(name));

        info.version.major = 1;
        info.version.minor = 0;

        return info;
    }

    PLUGIN_API ash::physics::context* make_context()
    {
        return new ash::physics::bullet3::bt3_context();
    }
}