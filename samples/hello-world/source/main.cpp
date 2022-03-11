#include "application.hpp"
#include "graphics.hpp"
#include "log.hpp"
#include "window.hpp"
#include <fstream>

using namespace ash::core;

class test_module : public submodule
{
public:
    static constexpr ash::uuid id = "bd58a298-9ea4-4f8d-a79c-e57ae694915d";

public:
    test_module(int data) : submodule("test_module"), m_data(data) {}

    virtual bool initialize(const ash::dictionary& config) override
    {
        m_title = config["test"]["title"];
        return true;
    }

private:
    std::string m_title;
    int m_data;
};

int main()
{
    int a = 10;
    ash::log::info("hello world");

    // Dictionary config = R"({"test": {"title":"test app"},"window":{"title":"你好", "width":400,
    // "height":200}})"_json;

    ash::dictionary config;

    std::fstream fin("resource/hello-world.json");
    if (fin.is_open())
    {
        fin >> config;
    }
    else
    {
        ash::log::error("can not open config file");
        return -1;
    }

    application app(config);
    app.install<test_module>(99);
    app.install<ash::window::window>();
    app.install<ash::graphics::graphics>();

    app.run();

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(600));

    return 0;
}