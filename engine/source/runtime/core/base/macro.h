#pragma once

#include "runtime/core/log/log_system.h"
#include "runtime/platform/file/file_system.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/function/render/vulkan/vulkan_util.h"

#define DEBUG (!NDEBUG)

#define APP_NAME "BambooEngine"
#define APP_MAJOR_VERSION 1
#define APP_MINOR_VERSION 0
#define APP_PATCH_VERSION 0

#define LOG_HELPER(LOG_LEVEL, ...) \
    g_runtime_context.logSystem()->log(LOG_LEVEL, "[" + std::string(__FUNCTION__) + "] " + __VA_ARGS__)

#define LOG_DEBUG(...) LOG_HELPER(LogSystem::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) LOG_HELPER(LogSystem::LogLevel::INFO, __VA_ARGS__)
#define LOG_WARNING(...) LOG_HELPER(LogSystem::LogLevel::WARNING, __VA_ARGS__)
#define LOG_ERROR(...) LOG_HELPER(LogSystem::LogLevel::ERROR, __VA_ARGS__)
#define LOG_FATAL(...) LOG_HELPER(LogSystem::LogLevel::FATAL, __VA_ARGS__)
#define ASSERT(val, ...) \
    if (!(val)) \
    { \
        LOG_FATAL(__VA_ARGS__); \
    }

#define CHECK_VULKAN_RESULT(result, msg) \
    if (result != 0) \
    { \
        LOG_FATAL("failed to {}, error: {}", msg, Bamboo::vkErrorString(result)); \
    }

#define REDIRECT(path) g_runtime_context.fileSystem()->redirect(path);