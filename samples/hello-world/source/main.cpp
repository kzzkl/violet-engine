#include "application.hpp"
#include "graphics.hpp"
#include "log.hpp"
#include "window.hpp"
#include <filesystem>
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
        m_title = config["title"];
        return true;
    }

private:
    std::string m_title;
    int m_data;
};

void test_json()
{
    ash::dictionary json1 = R"({"test": {"title":"test app","array":["1","2","3"],"array2":[{"name":"1"}]}})"_json;

    ash::dictionary json2 = R"({"test": {"title":"test app2","array":["1","2","3","4"],"array2":[{"name":"2"}]}})"_json;

    json1.update(json2, true);

    ash::log::info("{}", json1);
}

int main()
{
    // test_json();

    ash::log::info("hello world");

    application app;
    app.install<test_module>(99);
    app.install<ash::window::window>();
    app.install<ash::graphics::graphics>();

    app.run();

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(600));

    return 0;
}