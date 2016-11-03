#include <log/facility.hpp>
#include <log/sinks.hpp>

using mex_guard = std::lock_guard<std::mutex>;

namespace log {

facility_manager g_facility_manager(stream_sink(std::cerr, flag::noemitloc));

facility_record* facility_manager::get(const char* name) {
    std::lock_guard<std::mutex> guard(mgr_mex_);

    auto i = tbl_.find(name);
    if (i!=tbl_.end()) {
        return (i->second).get();
    }
    else {
        std::unique_ptr<facility_record> rec(new facility_record);
        rec->manager = this;
        rec->name = name;
        rec->level.store(default_level_);
        rec->sink = default_sink_;

        auto ptr = rec.get();
        tbl_.insert(std::make_pair(std::string(name), std::move(rec)));
        return ptr;
    }
}

void facility_manager::level(int level) {
    mex_guard guard(mgr_mex_);

    default_level_ = level;
    for (auto& entry: tbl_) {
        entry.second->level = level;
    }
}

log_sink_t facility_manager::default_sink() const {
    mex_guard guard(mgr_mex_);

    return default_sink_;
}

void facility_manager::default_sink(log_sink_t sink) {
    mex_guard guard(mgr_mex_);
    default_sink_ = std::move(sink);
}

// note: O(n) for n facilities
void facility_manager::rename(facility_record* ptr, const char* name) {
    mex_guard guard(mgr_mex_);

    auto i = tbl_.begin();
    for (; i!=tbl_.end() && i->second.get()!=ptr; ++i) {}

    if (i!=tbl_.end()) {
        std::unique_ptr<facility_record> rec(std::move(i->second));
        tbl_.erase(i);
        rec->name = name;
        tbl_.insert(std::make_pair(std::string(name), std::move(rec)));
    }
}


} // namespace log
