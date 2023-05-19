#include "runtime_context.h"
#include "runtime/core/log_system.h"

namespace Bamboo
{
    RuntimeContext g_runtime_context;

    void RuntimeContext::init()
    {
        m_log_system = std::make_shared<LogSystem>();

        m_context_modules = {
            m_log_system
        };

        for (auto& context_module : m_context_modules)
        {
            context_module->init();
        }
    }

    void RuntimeContext::destroy()
    {
        for (auto& context_module : m_context_modules)
        {
            context_module->destroy();
            context_module.reset();
        }
    }
}