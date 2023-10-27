#include "editor/editor.h"

int main(int argc, char** argv)
{
    Bamboo::Editor* editor = new Bamboo::Editor;
    editor->init();
    editor->run();
    editor->destroy();
    delete editor;
    
    return 0;
}