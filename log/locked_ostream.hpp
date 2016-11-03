#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace log {

struct locked_ostream: std::ostream {
    locked_ostream(std::streambuf *b): std::ostream(b) {
        mex = register_sbuf(b);
    }

    ~locked_ostream() {
        mex.reset();
        deregister_sbuf(rdbuf());
    }

    std::unique_lock<std::mutex> guard() {
        return std::unique_lock<std::mutex>(*mex);
    }

    std::shared_ptr<std::mutex> mex;

private:
    using tbl_type = std::unordered_map<std::streambuf*, std::weak_ptr<std::mutex>>;
    static tbl_type& mex_tbl() {
        static tbl_type tbl;
        return tbl;
    }

    static std::mutex& mex_tbl_mex() {
        static std::mutex mex;
        return mex;
    }

    static std::shared_ptr<std::mutex> register_sbuf(std::streambuf* b) {
        std::lock_guard<std::mutex> g(mex_tbl_mex());
        auto& wptr = mex_tbl()[b];
        auto mex = wptr.lock();
        if (!mex) {
            mex = std::shared_ptr<std::mutex>(new std::mutex);
            wptr = mex;
        }
        return mex;
    }

    static void deregister_sbuf(std::streambuf* b) {
        std::lock_guard<std::mutex> g(mex_tbl_mex());
        auto i = mex_tbl().find(b);
        if (i!=mex_tbl().end() && !(i->second.use_count())) {
            mex_tbl().erase(i);
        }
    }
};

} // namespace log
