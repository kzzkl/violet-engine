#include "core/context/engine.hpp"
#include "scene/scene_module.hpp"
#include <catch2/catch_all.hpp>

int main(int argc, char* argv[])
{
    violet::engine::initialize("");
    violet::engine::install<violet::scene_module>();

    return Catch::Session().run(argc, argv);
}