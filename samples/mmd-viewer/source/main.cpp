#include "mmd_viewer.hpp"

int main()
{
    violet::application app("assets/config/mmd-viewer.json");
    app.install<violet::mmd_viewer>();
    app.run();

    return 0;
}