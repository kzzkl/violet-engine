#include "editor/editor.hpp"

int main()
{
    ash::editor::editor_app editor;
    editor.initialize();
    editor.run();
    return 0;
}