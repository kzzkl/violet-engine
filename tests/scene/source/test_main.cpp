#include "core/context/engine.hpp"
#include "scene/scene.hpp"
#include <catch2/catch_all.hpp>

int main(int argc, char* argv[])
{
    violet::core::engine::initialize("");
    violet::core::engine::install<violet::scene::scene>();

    return Catch::Session().run(argc, argv);
}