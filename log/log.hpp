#pragma once

#include <log/sinks.hpp>
#include <log/facility.hpp>

namespace log {

// top-level function interface; forward calls to `g_facility_manager` or supplied
// facility object.

inline int level() { return g_facility_manager.level(); }
inline void level(int level) { g_facility_manager.level(level); }
inline void default_sink(log_sink_t sink) { g_facility_manager.default_sink(std::move(sink)); }
inline log_sink_t default_sink() { return g_facility_manager.default_sink(); }

inline int level(const char* fac) { return facility(fac).level(); }
inline void level(const char* fac, int level) { facility(fac).level(level); }
inline log_sink_t sink(const char* fac) { return facility(fac).sink(); }
inline void sink(const char* fac, log_sink_t sink) { facility(fac).sink(std::move(sink)); }

inline int level(const facility &fac) { return fac.level(); }
inline void level(facility& fac, int level) { fac.level(level); }
inline log_sink_t sink(const facility& fac) { return fac.sink(); }
inline void sink(facility& fac, log_sink_t sink) { fac.sink(std::move(sink)); }

// 'standard' logging facilities

extern facility log;
extern facility assertion_failure;
extern facility debug;

// source location wrapper

#define LOG_LOC ::log::source_location{__FILE__, __LINE__, __PRETTY_FUNCTION__}

// macro wrappers for logging facilities

#define LOG2(fac, n) if (auto log_magic_reserved_temp_ = ::log::log_test_proxy(::log::facility(fac)(n))) ; else log_magic_reserved_temp_.stream << LOG_LOC
#define LOG1(n) LOG2(::log::log, n)

#define LOG_SELECT(_0, _1, _2, ...) _2
#define LOG(...) LOG_SELECT(__VA_ARGS__, LOG2, LOG1)(__VA_ARGS__)

#ifndef LOG_NDEBUG
#define DEBUG(n) LOG2(::log::debug, n)
#else
#define DEBUG(n) if (true); else
#endif

#ifndef LOG_NASSERT
#define ASSERT(test) if (test) ; else ::log::assertion_failure << LOG_LOC
#else
#define ASSERT(test) if (true); else
#endif

} // namespace log
