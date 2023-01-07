#include "tagged_pointer.hpp"
#include "test_common.hpp"
#include <limits>

using namespace violet::task;

TEST_CASE("atomic operation on a tagged_pointer is lock-free", "[tagged_pointer]")
{
    std::atomic<tagged_pointer<int>> a;
    CHECK(a.is_lock_free());
}

TEST_CASE("construct pointer", "[tagged_pointer]")
{
    tagged_pointer<int> tp1;
    CHECK(tp1.get_tag() == 0);
    CHECK(tp1.get_pointer() == nullptr);

    int data = 99;
    tagged_pointer<int> tp2(&data);
    CHECK(tp2.get_tag() == 0);
    CHECK(tp2.get_pointer() == &data);

    tagged_pointer<int> tp3(&data, 1);
    CHECK(tp3.get_tag() == 1);
    CHECK(tp3.get_pointer() == &data);
}

TEST_CASE("geter and seter", "[tagged_pointer]")
{
    tagged_pointer<int> tp1;

    int data = 99;

    tp1.set_pointer(&data);
    CHECK(tp1.get_pointer() == &data);

    CHECK(tp1.get_tag() == 0);
    CHECK(tp1.get_next_tag() == 1);

    tagged_pointer<int> tp2(nullptr, std::numeric_limits<tagged_pointer<int>::tag_type>::max());
    CHECK(tp2.get_next_tag() == 0);
}

TEST_CASE("operator overloading", "[tagged_pointer]")
{
    struct test_struct
    {
        int data;
    };

    test_struct data;
    data.data = 99;

    tagged_pointer<test_struct> tp1(&data);
    CHECK(tp1->data == 99);

    tagged_pointer<test_struct> tp2(&data);
    CHECK(tp1 == tp2);

    tagged_pointer<test_struct> tp3(&data, tp2.get_next_tag());
    CHECK(tp1 != tp3);
}