#pragma once

#include <memory>
#include <vector>

namespace Bamboo
{
    class Editor
    {
        public:
            void init();
            void destroy();
            void run();

        private:
            class Engine* m_engine;
            std::vector<std::shared_ptr<class EditorUI>> m_editor_uis;
    };
}