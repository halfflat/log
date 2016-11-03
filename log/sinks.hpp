#pragma once

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ostream>

#include <log/facility.hpp>
#include <log/locked_ostream.hpp>

namespace log {

enum class flag {
    flush, noflush, emitloc, noemitloc, emitfac, noemitfac, abort, noabort
};

class stream_sink {
public:
    explicit stream_sink(std::ostream& o):
        out_(std::make_shared<locked_ostream>(o.rdbuf()))
    {
        out_->copyfmt(o);
    }

    template <typename... Flags>
    explicit stream_sink(std::ostream& o, Flags... flags): stream_sink(o) {
        flag fs[] = {flags...};
        for (auto f: fs) {
            set(f);
        }
    }

    void set(flag f) {
        switch (f) {
        case flag::flush:
            flush_ = true;
            break;
        case flag::noflush:
            flush_ = false;
            break;
        case flag::emitloc:
            emitloc_ = true;
            break;
        case flag::noemitloc:
            emitloc_ = false;
            break;
        case flag::emitfac:
            emitfac_ = true;
            break;
        case flag::noemitfac:
            emitfac_ = false;
            break;
        case flag::abort:
            abort_ = true;
            break;
        case flag::noabort:
            abort_ = false;
            break;
        }
    }

    void operator()(const log_entry& entry) {
        auto guard = out_->guard();

        format_entry(*out_, entry);
        if (flush_) out_->flush();
        if (abort_) std::abort();
    }

    static const char* basename(const char* path) {
        const char* slash = std::strrchr(path, '/');
        return slash? slash+1: path;
    }

private:
    std::shared_ptr<locked_ostream> out_;
    bool flush_ = true;
    bool abort_ = false;

protected:
    bool emitloc_ = true;
    bool emitfac_ = false;

    virtual void format_entry(std::ostream& o, const log_entry& entry) {
        // emit facility name and level, followed by source location,
        // followed by message.

        if (emitfac_) {
            format_facility(o, entry.name, entry.level);
        }
        if (emitloc_ && entry.location.file!=nullptr) {
            format_location(o, entry.location);
        }
        format_message(o, entry.message);
    }

    virtual void format_facility(std::ostream& o, const char* name, int level) {
        (void)level;
        o << name << ": ";
    }

    virtual void format_location(std::ostream& o, source_location loc) {
        o << basename(loc.file) << ':' << loc.line << " " << loc.func << ": ";
    }

    virtual void format_message(std::ostream& o, const char* msg) {
        o << msg << '\n';
    }
};

class file_sink: public stream_sink {
    std::shared_ptr<std::fstream> file;
public:
    template <typename... Flag>
    explicit file_sink(const std::string& filepath, Flag... flags):
        file(std::make_shared<std::fstream>(filepath)),
        stream_sink(file->rdbuf(), flags...)
    {}
};

} // namespace log
