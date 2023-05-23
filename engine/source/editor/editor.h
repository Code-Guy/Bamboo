#pragma once

namespace Bamboo
{
    class Engine;
    class Editor
    {
        public:
            void init(Engine* engine);
            void destroy();
            void run();

        private:
            Engine* m_engine;
    };
}