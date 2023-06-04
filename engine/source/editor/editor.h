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
            void onDrop(int n, const char** filenames);

            Engine* m_engine;
    };
}