#pragma once

#include <memory>
#include <vector>

namespace Bamboo
{
    class Editor
    {
        public:
            void init(class Engine* engine);
            void destroy();
            void run();

        private:
            void onDrop(int n, const char** filenames);

            class Engine* m_engine;
            std::vector<std::shared_ptr<class EditorUI>> m_editor_uis;
    };
}