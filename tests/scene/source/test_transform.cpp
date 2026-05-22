#include "test_scene_common.hpp"

namespace violet::test
{

// ============================================================================
// Part 1: Single entity transforms (update_transform with force=true)
// ============================================================================

TEST_CASE("transform: default state is identity", "[transform]")
{
    world& w = *g_world;

    entity e = w.create();
    w.add_component<transform_component>(e);

    update_transform();

    auto pos = get_world_pos(e);
    CHECK(approx_eq(pos.x, 0.0f));
    CHECK(approx_eq(pos.y, 0.0f));
    CHECK(approx_eq(pos.z, 0.0f));

    auto scale = get_world_scale(e);
    CHECK(approx_eq(scale.x, 1.0f));
    CHECK(approx_eq(scale.y, 1.0f));
    CHECK(approx_eq(scale.z, 1.0f));

    w.destroy(e);
}

TEST_CASE("transform: translation is applied to world matrix", "[transform]")
{
    world& w = *g_world;

    entity e = w.create();
    w.add_component<transform_component>(e);
    w.get_component<transform_component>(e).set_position({3.0f, -1.0f, 7.5f});

    update_transform();

    auto pos = get_world_pos(e);
    CHECK(approx_eq(pos.x, 3.0f));
    CHECK(approx_eq(pos.y, -1.0f));
    CHECK(approx_eq(pos.z, 7.5f));

    w.destroy(e);
}

TEST_CASE("transform: uniform scale is applied to world matrix", "[transform]")
{
    world& w = *g_world;

    entity e = w.create();
    w.add_component<transform_component>(e);
    w.get_component<transform_component>(e).set_scale(3.0f);

    update_transform();

    auto scale = get_world_scale(e);
    CHECK(approx_eq(scale.x, 3.0f));
    CHECK(approx_eq(scale.y, 3.0f));
    CHECK(approx_eq(scale.z, 3.0f));

    w.destroy(e);
}

TEST_CASE("transform: non-uniform scale is applied to world matrix", "[transform]")
{
    world& w = *g_world;

    entity e = w.create();
    w.add_component<transform_component>(e);
    w.get_component<transform_component>(e).set_scale({2.0f, 3.0f, 4.0f});

    update_transform();

    auto scale = get_world_scale(e);
    CHECK(approx_eq(scale.x, 2.0f));
    CHECK(approx_eq(scale.y, 3.0f));
    CHECK(approx_eq(scale.z, 4.0f));

    w.destroy(e);
}

TEST_CASE("transform: 90-degree Y-axis rotation", "[transform]")
{
    // A root entity's world position equals its local translation regardless of rotation.
    // Setting rotation should not corrupt the entity's own world position.
    world& w = *g_world;

    entity e = w.create();
    w.add_component<transform_component>(e);
    auto& t = w.get_component<transform_component>(e);

    vec4f q = quaternion::from_axis_angle(vec3f{0.0f, 1.0f, 0.0f}, math::to_radians(90.0f));
    t.set_rotation(q);
    t.set_position({1.0f, 0.0f, 0.0f});

    update_transform();

    // For a root entity, world position == local translation. Rotation does not move it.
    auto pos = get_world_pos(e);
    CHECK(approx_eq(pos.x, 1.0f, 1e-4f));
    CHECK(approx_eq(pos.y, 0.0f, 1e-4f));
    CHECK(approx_eq(pos.z, 0.0f, 1e-4f));

    w.destroy(e);
}

// ============================================================================
// Part 2: Parent-child world matrix propagation
//   Hierarchy is set up manually (parent_component + child_component) so that
//   update_transform() can be tested independently of update_hierarchy().
// ============================================================================

TEST_CASE("transform: child world position = parent position + child local position", "[transform]")
{
    world& w = *g_world;

    entity parent = w.create();
    entity child = w.create();

    w.add_component<transform_component>(parent);
    w.add_component<transform_component>(child);
    w.add_component<parent_component>(child);
    w.add_component<child_component>(parent);

    w.get_component<transform_component>(parent).set_position({1.0f, 0.0f, 0.0f});
    w.get_component<transform_component>(child).set_position({2.0f, 0.0f, 0.0f});
    w.get_component<parent_component>(child).parent = parent;
    w.get_component<child_component>(parent).children.push_back(child);

    update_transform();

    auto child_pos = get_world_pos(child);
    CHECK(approx_eq(child_pos.x, 3.0f));
    CHECK(approx_eq(child_pos.y, 0.0f));
    CHECK(approx_eq(child_pos.z, 0.0f));

    w.destroy(child);
    w.destroy(parent);
}

TEST_CASE("transform: child at origin inherits parent translation", "[transform]")
{
    world& w = *g_world;

    entity parent = w.create();
    entity child = w.create();

    w.add_component<transform_component>(parent);
    w.add_component<transform_component>(child);
    w.add_component<parent_component>(child);
    w.add_component<child_component>(parent);

    w.get_component<transform_component>(parent).set_position({5.0f, -3.0f, 2.0f});
    // child stays at origin in local space
    w.get_component<parent_component>(child).parent = parent;
    w.get_component<child_component>(parent).children.push_back(child);

    update_transform();

    auto child_pos = get_world_pos(child);
    CHECK(approx_eq(child_pos.x, 5.0f));
    CHECK(approx_eq(child_pos.y, -3.0f));
    CHECK(approx_eq(child_pos.z, 2.0f));

    w.destroy(child);
    w.destroy(parent);
}

TEST_CASE("transform: parent scale propagates to child world position", "[transform]")
{
    // Parent scaled by 2. Child at local (1, 0, 0).
    // Child world position should be (2, 0, 0).
    world& w = *g_world;

    entity parent = w.create();
    entity child = w.create();

    w.add_component<transform_component>(parent);
    w.add_component<transform_component>(child);
    w.add_component<parent_component>(child);
    w.add_component<child_component>(parent);

    w.get_component<transform_component>(parent).set_scale(2.0f);
    w.get_component<transform_component>(child).set_position({1.0f, 0.0f, 0.0f});
    w.get_component<parent_component>(child).parent = parent;
    w.get_component<child_component>(parent).children.push_back(child);

    update_transform();

    auto child_pos = get_world_pos(child);
    CHECK(approx_eq(child_pos.x, 2.0f));
    CHECK(approx_eq(child_pos.y, 0.0f));
    CHECK(approx_eq(child_pos.z, 0.0f));

    auto child_world_scale = get_world_scale(child);
    CHECK(approx_eq(child_world_scale.x, 2.0f));
    CHECK(approx_eq(child_world_scale.y, 2.0f));
    CHECK(approx_eq(child_world_scale.z, 2.0f));

    w.destroy(child);
    w.destroy(parent);
}

TEST_CASE("transform: parent rotation rotates child local position into world space", "[transform]")
{
    // Parent has 90° Y rotation. Child at local (1, 0, 0).
    // Child world position should be approximately (0, 0, -1).
    world& w = *g_world;

    entity parent = w.create();
    entity child = w.create();

    w.add_component<transform_component>(parent);
    w.add_component<transform_component>(child);
    w.add_component<parent_component>(child);
    w.add_component<child_component>(parent);

    vec4f q = quaternion::from_axis_angle(vec3f{0.0f, 1.0f, 0.0f}, math::to_radians(90.0f));
    w.get_component<transform_component>(parent).set_rotation(q);
    w.get_component<transform_component>(child).set_position({1.0f, 0.0f, 0.0f});
    w.get_component<parent_component>(child).parent = parent;
    w.get_component<child_component>(parent).children.push_back(child);

    update_transform();

    auto child_pos = get_world_pos(child);
    CHECK(approx_eq(child_pos.x, 0.0f, 1e-4f));
    CHECK(approx_eq(child_pos.y, 0.0f, 1e-4f));
    CHECK(approx_eq(child_pos.z, -1.0f, 1e-4f));

    w.destroy(child);
    w.destroy(parent);
}

TEST_CASE("transform: three-level hierarchy accumulates translations", "[transform]")
{
    // A at (1,0,0) -> B at local (2,0,0) -> C at local (3,0,0)
    // C world position should be (6, 0, 0).
    world& w = *g_world;

    entity a = w.create();
    entity b = w.create();
    entity c = w.create();

    w.add_component<transform_component>(a);
    w.add_component<transform_component>(b);
    w.add_component<transform_component>(c);
    w.add_component<parent_component>(b);
    w.add_component<parent_component>(c);
    w.add_component<child_component>(a);
    w.add_component<child_component>(b);

    w.get_component<transform_component>(a).set_position({1.0f, 0.0f, 0.0f});
    w.get_component<transform_component>(b).set_position({2.0f, 0.0f, 0.0f});
    w.get_component<transform_component>(c).set_position({3.0f, 0.0f, 0.0f});
    w.get_component<parent_component>(b).parent = a;
    w.get_component<parent_component>(c).parent = b;
    w.get_component<child_component>(a).children.push_back(b);
    w.get_component<child_component>(b).children.push_back(c);

    update_transform();

    CHECK(vec3_approx_eq(get_world_pos(a), {1.0f, 0.0f, 0.0f}));
    CHECK(vec3_approx_eq(get_world_pos(b), {3.0f, 0.0f, 0.0f}));
    CHECK(vec3_approx_eq(get_world_pos(c), {6.0f, 0.0f, 0.0f}));

    w.destroy(c);
    w.destroy(b);
    w.destroy(a);
}

TEST_CASE("transform: three-level hierarchy - middle node update propagates to leaf", "[transform]")
{
    // A at origin -> B at local (1,0,0) -> C at local (1,0,0).
    // After updating B's position to (2,0,0), C's world position should change from (2,0,0) to
    // (3,0,0).
    world& w = *g_world;

    entity a = w.create();
    entity b = w.create();
    entity c = w.create();

    w.add_component<transform_component>(a);
    w.add_component<transform_component>(b);
    w.add_component<transform_component>(c);
    w.add_component<parent_component>(b);
    w.add_component<parent_component>(c);
    w.add_component<child_component>(a);
    w.add_component<child_component>(b);

    w.get_component<transform_component>(b).set_position({1.0f, 0.0f, 0.0f});
    w.get_component<transform_component>(c).set_position({1.0f, 0.0f, 0.0f});
    w.get_component<parent_component>(b).parent = a;
    w.get_component<parent_component>(c).parent = b;
    w.get_component<child_component>(a).children.push_back(b);
    w.get_component<child_component>(b).children.push_back(c);

    update_transform();
    CHECK(vec3_approx_eq(get_world_pos(c), {2.0f, 0.0f, 0.0f}));

    // Move B - this should cascade to C.
    w.get_component<transform_component>(b).set_position({2.0f, 0.0f, 0.0f});
    update_transform();
    CHECK(vec3_approx_eq(get_world_pos(c), {3.0f, 0.0f, 0.0f}));

    w.destroy(c);
    w.destroy(b);
    w.destroy(a);
}

// ============================================================================
// Part 3: Hierarchy management via full tick (tests update_hierarchy)
// ============================================================================

TEST_CASE(
    "hierarchy: parent_component causes child_component to be created on parent",
    "[hierarchy]")
{
    world& w = *g_world;

    entity parent = w.create();
    entity child = w.create();

    w.add_component<transform_component>(parent);
    w.add_component<transform_component>(child);
    w.add_component<parent_component>(child);

    w.get_component<transform_component>(parent).set_position({1.0f, 0.0f, 0.0f});
    w.get_component<transform_component>(child).set_position({2.0f, 0.0f, 0.0f});
    w.get_component<parent_component>(child).parent = parent;

    // Hierarchy management runs inside tick().
    tick();

    REQUIRE(w.has_component<child_component>(parent));
    const auto& children = w.get_component<const child_component>(parent).children;
    CHECK(children.size() == 1);
    CHECK(children[0] == child);

    // World position should be accumulated correctly.
    CHECK(vec3_approx_eq(get_world_pos(child), {3.0f, 0.0f, 0.0f}));

    w.destroy(child);
    w.destroy(parent);
    tick(); // flush destruction side-effects
}

TEST_CASE("hierarchy: multiple children added to same parent in one tick", "[hierarchy]")
{
    world& w = *g_world;

    entity parent = w.create();
    entity child1 = w.create();
    entity child2 = w.create();

    w.add_component<transform_component>(parent);
    w.add_component<transform_component>(child1);
    w.add_component<transform_component>(child2);
    w.add_component<parent_component>(child1);
    w.add_component<parent_component>(child2);

    w.get_component<transform_component>(parent).set_position({0.0f, 0.0f, 0.0f});
    w.get_component<transform_component>(child1).set_position({1.0f, 0.0f, 0.0f});
    w.get_component<transform_component>(child2).set_position({2.0f, 0.0f, 0.0f});
    w.get_component<parent_component>(child1).parent = parent;
    w.get_component<parent_component>(child2).parent = parent;

    tick();

    REQUIRE(w.has_component<child_component>(parent));
    const auto& children = w.get_component<const child_component>(parent).children;
    CHECK(children.size() == 2);

    // Both children should exist in the list.
    bool found1 = false, found2 = false;
    for (entity c : children)
    {
        if (c == child1)
            found1 = true;
        if (c == child2)
            found2 = true;
    }
    CHECK(found1);
    CHECK(found2);

    CHECK(vec3_approx_eq(get_world_pos(child1), {1.0f, 0.0f, 0.0f}));
    CHECK(vec3_approx_eq(get_world_pos(child2), {2.0f, 0.0f, 0.0f}));

    w.destroy(child2);
    w.destroy(child1);
    w.destroy(parent);
    tick();
}

TEST_CASE("hierarchy: removing parent_component detaches child from parent", "[hierarchy]")
{
    world& w = *g_world;

    entity parent = w.create();
    entity child = w.create();

    w.add_component<transform_component>(parent);
    w.add_component<transform_component>(child);
    w.add_component<parent_component>(child);

    w.get_component<transform_component>(parent).set_position({5.0f, 0.0f, 0.0f});
    w.get_component<transform_component>(child).set_position({1.0f, 0.0f, 0.0f});
    w.get_component<parent_component>(child).parent = parent;

    tick();

    // Verify initial state.
    REQUIRE(w.has_component<child_component>(parent));
    CHECK(vec3_approx_eq(get_world_pos(child), {6.0f, 0.0f, 0.0f}));

    // Set parent to INVALID to detach.
    w.get_component<parent_component>(child).parent = INVALID_ENTITY;

    tick();

    // parent_component (and its companion meta) should be removed from child.
    CHECK_FALSE(w.has_component<parent_component>(child));
    // Parent should no longer have child_component once its children list is empty.
    CHECK_FALSE(w.has_component<child_component>(parent));

    // Child is now a root entity; its world position equals its local position.
    w.get_component<transform_component>(child).set_position({1.0f, 0.0f, 0.0f});
    update_transform();
    CHECK(vec3_approx_eq(get_world_pos(child), {1.0f, 0.0f, 0.0f}));

    w.destroy(child);
    w.destroy(parent);
    tick();
}

TEST_CASE("hierarchy: reparenting moves child between parents", "[hierarchy]")
{
    world& w = *g_world;

    entity p1 = w.create();
    entity p2 = w.create();
    entity child = w.create();

    w.add_component<transform_component>(p1);
    w.add_component<transform_component>(p2);
    w.add_component<transform_component>(child);
    w.add_component<parent_component>(child);

    w.get_component<transform_component>(p1).set_position({10.0f, 0.0f, 0.0f});
    w.get_component<transform_component>(p2).set_position({20.0f, 0.0f, 0.0f});
    w.get_component<transform_component>(child).set_position({1.0f, 0.0f, 0.0f});
    w.get_component<parent_component>(child).parent = p1;

    tick();

    REQUIRE(w.has_component<child_component>(p1));
    CHECK(vec3_approx_eq(get_world_pos(child), {11.0f, 0.0f, 0.0f}));

    // Reparent: directly assign new parent so that update_hierarchy can compare
    // against previous_parent (p1) and remove the child from p1's children list.
    w.get_component<parent_component>(child).parent = p2;

    tick();

    // p1 should no longer have child_component.
    CHECK_FALSE(w.has_component<child_component>(p1));
    // p2 should now own the child.
    REQUIRE(w.has_component<child_component>(p2));
    const auto& p2_children = w.get_component<const child_component>(p2).children;
    REQUIRE(p2_children.size() == 1);
    CHECK(p2_children[0] == child);

    CHECK(vec3_approx_eq(get_world_pos(child), {21.0f, 0.0f, 0.0f}));

    w.destroy(child);
    w.destroy(p2);
    w.destroy(p1);
    tick();
}

// ============================================================================
// Part 4: Edge cases
// ============================================================================

TEST_CASE("transform: multiple calls to update_transform are idempotent", "[transform]")
{
    world& w = *g_world;

    entity e = w.create();
    w.add_component<transform_component>(e);
    w.get_component<transform_component>(e).set_position({4.0f, 5.0f, 6.0f});

    update_transform();
    auto pos1 = get_world_pos(e);

    // Second call without any position change should yield the same result.
    update_transform();
    auto pos2 = get_world_pos(e);

    CHECK(approx_eq(pos1.x, pos2.x));
    CHECK(approx_eq(pos1.y, pos2.y));
    CHECK(approx_eq(pos1.z, pos2.z));

    w.destroy(e);
}

TEST_CASE("transform: multiple siblings share parent correctly", "[transform]")
{
    // Parent at (1,0,0). Three children at local (1,0,0), (0,1,0), (0,0,1).
    world& w = *g_world;

    entity parent = w.create();
    entity c1 = w.create();
    entity c2 = w.create();
    entity c3 = w.create();

    w.add_component<transform_component>(parent);
    w.add_component<transform_component>(c1);
    w.add_component<transform_component>(c2);
    w.add_component<transform_component>(c3);
    w.add_component<parent_component>(c1);
    w.add_component<parent_component>(c2);
    w.add_component<parent_component>(c3);
    w.add_component<child_component>(parent);

    w.get_component<transform_component>(parent).set_position({1.0f, 0.0f, 0.0f});
    w.get_component<transform_component>(c1).set_position({1.0f, 0.0f, 0.0f});
    w.get_component<transform_component>(c2).set_position({0.0f, 1.0f, 0.0f});
    w.get_component<transform_component>(c3).set_position({0.0f, 0.0f, 1.0f});
    w.get_component<parent_component>(c1).parent = parent;
    w.get_component<parent_component>(c2).parent = parent;
    w.get_component<parent_component>(c3).parent = parent;
    auto& children = w.get_component<child_component>(parent).children;
    children.push_back(c1);
    children.push_back(c2);
    children.push_back(c3);

    update_transform();

    CHECK(vec3_approx_eq(get_world_pos(c1), {2.0f, 0.0f, 0.0f}));
    CHECK(vec3_approx_eq(get_world_pos(c2), {1.0f, 1.0f, 0.0f}));
    CHECK(vec3_approx_eq(get_world_pos(c3), {1.0f, 0.0f, 1.0f}));

    w.destroy(c3);
    w.destroy(c2);
    w.destroy(c1);
    w.destroy(parent);
}

} // namespace violet::test