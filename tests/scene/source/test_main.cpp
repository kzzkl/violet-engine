#include "core/context/engine.hpp"
#include "scene/scene.hpp"
#include <catch2/catch_all.hpp>

int main(int argc, char* argv[])
{
    violet::engine::initialize("");
    violet::engine::install<violet::scene>();

    return Catch::Session().run(argc, argv);
}