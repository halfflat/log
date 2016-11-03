#include <log/log.hpp>

// Standard logging facilities

namespace log {

facility log("log");
facility debug("debug");
facility assertion_failure("assertion_failure");

struct std_log_init {
    std_log_init() {
        log.sink(stream_sink(std::cerr, flag::noemitloc));
        debug.sink(stream_sink(std::cerr));
        assertion_failure.sink(stream_sink(std::cerr, flag::emitfac, flag::abort));
    }
} Init;


} // namespace log
