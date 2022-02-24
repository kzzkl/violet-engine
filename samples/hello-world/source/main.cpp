#include "application.hpp"
#include "log.hpp"
#include "window.hpp"
#include <fstream>

using namespace ash::core;
using namespace ash::common;

class test_module : public submodule
{
public:
    test_module(int data) : submodule("test_module"), m_data(data) {}

    virtual bool initialize(const ash::common::dictionary& config) override
    {
        m_title = config["test"]["title"];
        return true;
    }

    virtual void tick() override
    {
        // Log::Debug("test: title: {} data: {}", m_title, m_data);
    }

private:
    std::string m_title;
    int m_data;
};

int main()
{
    int a = 10;
    log::info("hello world");

    // Dictionary config = R"({"test": {"title":"test app"},"window":{"title":"你好", "width":400,
    // "height":200}})"_json;

    dictionary config;

    std::fstream fin("resource/hello-world.json");
    if (fin.is_open())
    {
        fin >> config;
    }
    else
    {
        log::error("can not open config file");
        return -1;
    }

    application app(config);
    app.install<test_module>(99);
    app.install<ash::window::window>();

    app.run();

    return 0;
}