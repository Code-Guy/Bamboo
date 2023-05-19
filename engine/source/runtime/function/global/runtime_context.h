#pragma once

#include <memory>
#include <vector>

namespace Bamboo
{
    class LogSystem;

    class RuntimeContext
    {
        public:
            void init();
            void destroy();

            std::shared_ptr<LogSystem> m_log_system;

        private:

    };

    extern RuntimeContext g_runtime_context;
}
