#pragma once

namespace Bamboo
{
    class ContextModule
    {
        public:
            virtual void init() = 0;
            virtual void destroy() = 0;
    };
}