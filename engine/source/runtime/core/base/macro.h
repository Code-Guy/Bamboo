#pragma once

#include "runtime/core/log/log_system.h"
#include "runtime/platform/file/file_system.h"
#include "runtime/function/global/runtime_context.h"

#define DEBUG (!NDEBUG)

#define APP_NAME "BambooEngine"
#define APP_MAJOR_VERSION 1
#define APP_MINOR_VERSION 0
#define APP_PATCH_VERSION 0

#define LOG_HELPER(LOG_LEVEL, ...) \
    g_runtime_context.logSystem()->log(LOG_LEVEL, "[" + std::string(__FUNCTION__) + "] " + __VA_ARGS__)

#define LOG_DEBUG(...) LOG_HELPER(LogSystem::ELogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...) LOG_HELPER(LogSystem::ELogLevel::Info, __VA_ARGS__)
#define LOG_WARNING(...) LOG_HELPER(LogSystem::ELogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...) LOG_HELPER(LogSystem::ELogLevel::Error, __VA_ARGS__)
#define LOG_FATAL(...) LOG_HELPER(LogSystem::ELogLevel::Fatal, __VA_ARGS__)
#define ASSERT(val, ...) \
    if (!(val)) \
    { \
        LOG_FATAL(__VA_ARGS__); \
    }

#define TO_ABSOLUTE(path) g_runtime_context.fileSystem()->absolute(path)