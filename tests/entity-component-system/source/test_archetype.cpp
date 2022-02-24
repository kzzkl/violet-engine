#include "archetype.hpp"
#include "test_common.hpp"

using namespace ash::ecs;
using namespace test;

TEST_CASE("layout", "[archetype_layout]")
{
    archetype_layout layout(2048);
    layout.insert<uint8_t, uint32_t, uint16_t, uint64_t, std::string>();

    CHECK(layout.get_entity_per_chunk() == 37);

    CHECK(layout[component_trait<uint64_t>::index()].layout.size == sizeof(uint64_t));
    CHECK(layout[component_trait<uint32_t>::index()].layout.size == sizeof(uint32_t));
    CHECK(layout[component_trait<uint16_t>::index()].layout.size == sizeof(uint16_t));
    CHECK(layout[component_trait<uint8_t>::index()].layout.size == sizeof(uint8_t));
    CHECK(layout[component_trait<std::string>::index()].layout.size == sizeof(std::string));

    std::vector<std::pair<component_index, archetype_layout::component_info>> info;
    for (auto& [type, l] : layout)
        info.push_back({type, l});

    std::sort(info.begin(), info.end(), [](const auto& a, const auto& b) {
        return a.second.layout.offset < b.second.layout.offset;
    });

    std::size_t align = 0xFFFF;
    for (auto& [type, l] : info)
    {
        CHECK(align >= l.layout.align);
        align = l.layout.align;
    }

    std::size_t offset = 0;
    for (auto& [type, l] : info)
    {
        CHECK(layout[type].layout.offset == offset);
        offset += layout[type].layout.size * layout.get_entity_per_chunk();
    }
}

TEST_CASE("redirector", "[redirector]")
{
    redirector r;

    entity entity1 = 1;
    size_t entity1Index = 0;
    r.map(entity1, entity1Index);

    CHECK(r.get_index(entity1) == entity1Index);
    CHECK(r.get_enitiy(entity1Index) == entity1);

    entity entity2 = 2;
    size_t entity2Index = 1;
    r.map(entity2, entity2Index);

    CHECK(r.get_index(entity2) == entity2Index);
    CHECK(r.get_enitiy(entity2Index) == entity2);

    CHECK(r.has_entity(entity2) == true);
    CHECK(r.size() == 2);

    r.unmap(entity2);
    CHECK(r.has_entity(entity2) == false);
    CHECK(r.has_entity(entity1) == true);
    CHECK(r.size() == 1);

    r.unmap(entity1);
    CHECK(r.has_entity(entity1) == false);
    CHECK(r.size() == 0);
}

TEST_CASE("Archetype Add", "[archetype]")
{
    archetype_layout layout(storage::CHUNK_SIZE);
    layout.insert<rotation, position, test_class>();

    archetype a(layout);

    entity entity1 = 1;

    archetype::handle<rotation> iter = a.add(entity1);

    auto aiter = a.begin<rotation, position, test_class>();
    rotation& r = aiter.get_component<rotation>();

    r.angle = 99;

    CHECK(&iter.get_component<rotation>() == &aiter.get_component<rotation>());
}

TEST_CASE("Archetype Remove", "[archetype]")
{
    archetype_layout layout(storage::CHUNK_SIZE);
    layout.insert<rotation, position, test_class>();

    archetype a(layout);

    entity entity1 = 1;

    a.add(1);
    CHECK(a.size() == 1);
    CHECK(a.has_entity(entity1) == true);

    a.remove(entity1);
    CHECK(a.size() == 0);
    CHECK(a.has_entity(entity1) == false);
}

TEST_CASE("Archetype Iterator", "[archetype]")
{
    archetype_layout layout(storage::CHUNK_SIZE);
    layout.insert<rotation, position, test_class>();

    archetype a(layout);

    std::vector<entity> entities = {11, 2, 34, 59};
    for (entity e : entities)
    {
        a.add(e);
    }

    for (auto iter = a.begin<rotation>(); iter != a.end<rotation>(); ++iter)
    {
        iter.get_component<rotation>().angle = iter.get_entity();
    }
}