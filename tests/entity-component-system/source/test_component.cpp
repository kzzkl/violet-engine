#include "component.hpp"
#include "test_common.hpp"

using namespace test;
using namespace ash::ecs;
/*
TEST_CASE("get type id", "[component_index]")
{
    component_index type0 = component_trait<char>::index();
    component_index type1 = component_trait<int>::index();
    component_index type2 = component_trait<position>::index();
    component_index type3 = component_trait<long>::index();
    component_index type4 = component_trait<int>::index();

    CHECK(type1 != type2);
    CHECK(type2 != type3);
    CHECK(type1 != type3);
    CHECK(type1 == type4);
}

class type_functor
{
public:
    struct TypeUnit
    {
        component_index type;
        size_t size;
        size_t align;
    };

public:
    template <typename T>
    void operator()()
    {
        info.emplace_back(
            TypeUnit{.type = component_trait<T>::index(), .size = sizeof(T), .align = alignof(T)});
    }

    std::vector<TypeUnit> info;
};

TEST_CASE("each", "[component_list]")
{
    type_functor f;
    component_list<int, char, int>::each(f);

    REQUIRE(f.info.size() == 3);

    CHECK(f.info[0].type == component_trait<int>::index());
    CHECK(f.info[0].size == sizeof(int));
    CHECK(f.info[0].align == alignof(int));

    CHECK(f.info[1].type == component_trait<char>::index());
    CHECK(f.info[1].size == sizeof(char));
    CHECK(f.info[1].align == alignof(char));

    CHECK(f.info[2].type == component_trait<int>::index());
    CHECK(f.info[2].size == sizeof(int));
    CHECK(f.info[2].align == alignof(int));
}

TEST_CASE("mask", "[component_list]")
{
    component_mask mask1 = component_list<int, char, int>::get_mask();
    component_mask mask2 = component_list<long, int, test_class>::get_mask();

    component_mask mask3 = mask1 & mask2;

    bool none = mask3.any();
}*/