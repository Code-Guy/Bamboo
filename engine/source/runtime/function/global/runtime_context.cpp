#include "runtime_context.h"
#include "runtime/core/log/log_system.h"

namespace Bamboo
{
    RuntimeContext g_runtime_context;

    void RuntimeContext::init()
    {
        m_log_system = std::make_shared<LogSystem>();
        m_log_system->init();
    }

    void RuntimeContext::destroy()
    {
        m_log_system->destroy();
    }
}