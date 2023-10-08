#include "engine/engine.h"
#include "editor.h"

int main(int argc, char** argv)
{
    Bamboo::Engine* engine = new Bamboo::Engine;
    engine->init();

    Bamboo::Editor* editor = new Bamboo::Editor;
    editor->init(engine);
    editor->run();
    editor->destroy();

    engine->destroy();

    return 0;
}