#pragma once

namespace Bamboo
{
    class Engine;

    class Editor
    {
        public:
            Editor();
            virtual ~Editor();

            void init(Engine* engine);
            void destroy();
            void run();

        private:
            Engine* m_engine;
    };
}