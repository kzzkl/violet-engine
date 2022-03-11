#include "Storage.hpp"
#include "test_common.hpp"

using namespace ash::ecs;
using namespace test;

TEST_CASE("chunk align", "[Chunk]")
{
    auto c = std::make_unique<chunk>();

    uint64_t address = reinterpret_cast<uint64_t>(c.get());
    CHECK(address % 64 == 0);
}

TEST_CASE("storage", "[Storage]")
{
    storage storage(2);

    CHECK(storage.get_chunk_size() == 0);

    storage.push_back();
    CHECK(storage.get_entity_size() == 1);
    CHECK(storage.get_chunk_size() == 1);

    storage.push_back();
    CHECK(storage.get_entity_size() == 2);
    CHECK(storage.get_chunk_size() == 1);

    storage.push_back();
    CHECK(storage.get_entity_size() == 3);
    CHECK(storage.get_chunk_size() == 2);

    storage.pop_back();
    CHECK(storage.get_entity_size() == 2);
    CHECK(storage.get_chunk_size() == 1);
}

TEST_CASE("append", "[Storage]")
{
    /*Storage storage = Storage::Make<rotation, position, test_class>();

    auto iter = storage.Append();

    rotation& r = *reinterpret_cast<rotation*>(iter.GetComponent<rotation>());
    position& p = *reinterpret_cast<position*>(iter.GetComponent<position>());

    r.angle = 180;
    p.x = 1;
    p.y = 2;
    p.z = 3;*/

    // test_class& t = iter.GetComponent<test_class>();
    // CHECK(t.data == 100);
}