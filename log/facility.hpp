#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_map>

namespace log {

// source file location information

struct source_location {
    const char* file;
    int line;
    const char* func;
};

constexpr source_location no_source_location{nullptr, 0, nullptr};

// log record handler type

struct log_entry {
    const char* name;         // facility name
    int level;                // log message level
    source_location location; // source info if provided
    const char* message;      // log message
};

using log_sink_t = std::function<void (const log_entry&)>;

// `facility_manager` maintains a collection of log facilities

struct facility_record;

class facility_manager {
private:
    mutable std::mutex mgr_mex_;

    std::unordered_multimap<std::string, std::unique_ptr<facility_record>> tbl_;
    std::atomic<int> default_level_;
    log_sink_t default_sink_;

public:
    facility_manager():
        default_level_(0), default_sink_([](const log_entry&) {}) {}

    explicit facility_manager(log_sink_t sink):
        default_level_(0), default_sink_(std::move(sink)) {}

    facility_manager(const facility_manager&) = delete;
    facility_manager &operator=(const facility_manager&&) = delete;
    facility_manager &operator=(facility_manager&&) = delete;

    facility_manager(facility_manager&&) = default;

    // default level for new facilities
    int level() const { return default_level_; }

    // set level (and default level) for all facilities
    void level(int);

    // default sink for new facilities
    log_sink_t default_sink() const;

    // set default sink for new facilities
    void default_sink(log_sink_t sink);

private:
    friend class facility;

    // rename entry (O(n) for n facilities)
    void rename(facility_record* ptr, const char* name);

    // retrieve or create facility data
    facility_record* get(const char* name);
};


// typically there will be one facility manager used:

extern facility_manager g_facility_manager;

// facility semantics are determined by their `facility_record` data;
// pointers to `facility_record` data provided by a `facility_manager` instance
// have the same lifetime as that instance.

struct facility_record {
    facility_manager* manager;
    std::atomic<const char*> name;
    std::atomic<int> level;

    mutable std::mutex sink_mex;
    log_sink_t sink;
};

// stream class for collecting log record information

class sink_stream: public std::ostream {
    const facility_record* data_;
    int level_;
    source_location loc_;

public:
    sink_stream(const facility_record* data, int level):
        std::ostream(new std::stringbuf),
        data_(data), level_(level), loc_(no_source_location)
    {}

    sink_stream():
        std::ostream(nullptr),
        data_(nullptr), level_(0), loc_(no_source_location)
    {}

    sink_stream(sink_stream&& them):
        std::ostream(std::move(them)),
        data_(them.data_), level_(them.level_), loc_(them.loc_)
    {
        rdbuf(them.rdbuf());
        them.rdbuf(nullptr);
    }

    sink_stream(const sink_stream&) = delete;
    sink_stream& operator=(const sink_stream&) = delete;
    sink_stream& operator=(sink_stream&&) = delete;

    void set_location(source_location loc) {
        loc_ = loc;
    }

    ~sink_stream() {
        std::stringbuf *buf = dynamic_cast<std::stringbuf*>(rdbuf());
        if (buf && data_) {
            log_sink_t sink;
            {
                std::lock_guard<std::mutex> guard(data_->sink_mex);
                sink = data_->sink;
            }
            if (sink) sink(log_entry{data_->name, level_, loc_, buf->str().c_str()});
        }
        delete buf;
    }
};

// `source_location` data is handled especially by `sink_stream`

inline std::ostream& operator<<(std::ostream& out, const source_location& loc) {
    if (auto s = dynamic_cast<sink_stream*>(&out)) {
        s->set_location(loc);
    }
    else {
        // default formatting
        out << loc.file << ':' << loc.line << ' ' << loc.func;
    }
    return out;
}

// `log_test_proxy` is used by the LOG macro to test if a `sink_stream` is
// *not* valid

struct log_test_proxy {
    explicit log_test_proxy(sink_stream&& s): stream(std::move(s)) {}
    operator bool() const { return !stream; }
    sink_stream stream;
};

// logging facility

class facility {
    facility_record* data_;

public:
    sink_stream operator()(int lev) {
        return lev<=data_->level? sink_stream(data_, lev): sink_stream();
    }

    template <typename T>
    sink_stream operator<<(T&& x) {
        sink_stream s(data_, 0);
        s << std::forward<T>(x);
        return std::move(s);
    }

    explicit facility(const char* name, facility_manager& mgr = g_facility_manager):
        data_(mgr.get(name)) {}

    facility(const facility&) = default;
    facility& operator=(const facility&) = default;

    const char* name() const { return data_->name; }
    void name(const char* name) { data_->manager->rename(data_, name); }

    int level() const { return data_->level; }
    void level(int lev) { data_->level = lev; }

    log_sink_t sink() const {
        std::lock_guard<std::mutex> guard(data_->sink_mex);
        return data_->sink;
    }
    void sink(log_sink_t sink) const {
        std::lock_guard<std::mutex> guard(data_->sink_mex);
        data_->sink = std::move(sink);
    }
};

} // namespace log
