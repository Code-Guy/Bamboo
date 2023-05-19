#pragma once

#include <memory>
#include <vector>

namespace Bamboo
{
    class ContextModule;
    class LogSystem;

    class RuntimeContext
    {
        public:
            void init();
            void destroy();

            std::shared_ptr<LogSystem> m_log_system;

        private:
            std::vector<std::shared_ptr<ContextModule>> m_context_modules;
    };

    extern RuntimeContext g_runtime_context;
}
