#pragma once

#define LOG4CXX_FORMAT_NS std
#include <log4cxx/logger.h>

#define LOG_TRACE(...)    LOG4CXX_TRACE_FMT(  log4cxx::Logger::getLogger(LOGGER) __VA_OPT__(,) __VA_ARGS__ )
#define LOG_DEBUG(...)    LOG4CXX_DEBUG_FMT( log4cxx::Logger::getLogger(LOGGER) __VA_OPT__(,) __VA_ARGS__ )
#define LOG_INFO(...)     LOG4CXX_INFO_FMT( log4cxx::Logger::getLogger(LOGGER) __VA_OPT__(,) __VA_ARGS__ )
#define LOG_WARNING(...)  LOG4CXX_WARN_FMT( log4cxx::Logger::getLogger(LOGGER) __VA_OPT__(,) __VA_ARGS__ )
#define LOG_ERROR(...)    LOG4CXX_ERROR_FMT( log4cxx::Logger::getLogger(LOGGER) __VA_OPT__(,) __VA_ARGS__ )
