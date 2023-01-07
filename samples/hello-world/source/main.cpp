#include "core/application.hpp"
#include "graphics/graphics.hpp"
#include "log.hpp"
#include "window/window.hpp"
#include <filesystem>
#include <fstream>

using namespace violet::core;

class test_module : public violet::core::system_base
{
public:
    test_module(int data) : system_base("test_module"), m_data(data) {}

    virtual bool initialize(const violet::dictionary& config) override
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
    violet::dictionary json1 =
        R"({"test": {"title":"test app","array":["1","2","3"],"array2":[{"name":"1"}]}})"_json;

    violet::dictionary json2 =
        R"({"test": {"title":"test app2","array":["1","2","3","4"],"array2":[{"name":"2"}]}})"_json;

    json1.update(json2, true);

    violet::log::info("{}", json1);
}

int main()
{
    // test_json();

    violet::log::info("hello world");

    application app;
    app.install<test_module>(99);
    app.install<violet::window::window>();
    app.install<violet::graphics::graphics>();

    app.run();

    return 0;
}