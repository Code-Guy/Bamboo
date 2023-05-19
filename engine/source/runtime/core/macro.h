#pragma once

#include "runtime/core/log_system.h"
#include "runtime/function/global/runtime_context.h"

#define LOG_HELPER(LOG_LEVEL, ...) \
    g_runtime_context.m_log_system->log(LOG_LEVEL, "[" + std::string(__FUNCTION__) + "] " + __VA_ARGS__);

#define LOG_DEBUG(...) LOG_HELPER(LogSystem::LogLevel::DEBUG, __VA_ARGS__);

#define LOG_INFO(...) LOG_HELPER(LogSystem::LogLevel::INFO, __VA_ARGS__);

#define LOG_WARNING(...) LOG_HELPER(LogSystem::LogLevel::WARNING, __VA_ARGS__);

#define LOG_ERROR(...) LOG_HELPER(LogSystem::LogLevel::ERROR, __VA_ARGS__);

#define LOG_FATAL(...) LOG_HELPER(LogSystem::LogLevel::FATAL, __VA_ARGS__);