#include "gtest.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include <log/log.hpp>

#define ASSERT_STRING_HAS(s, match)\
ASSERT_NE(std::string::npos, std::string(s).find(match))

#define EXPECT_STRING_HAS(s, match)\
EXPECT_NE(std::string::npos, std::string(s).find(match))

#define ASSERT_STRING_EQ(expect, value)\
ASSERT_EQ(std::string(expect), std::string(value))

#define EXPECT_STRING_EQ(expect, value)\
EXPECT_EQ(std::string(expect), std::string(value))

// tmpnam() would be fine, but cannot surpress linker warning.
// modify as required if no `mkstemp` in stdlib
struct temporary_file {
    char* path;
    int fd;

    temporary_file(): path(nullptr), fd(-1) {
        try {
            const char *tmpenv = nullptr;
            for (auto env: {"TMPDIR", "TMP", "TEMP", "TEMPDIR"}) {
                if ((tmpenv = std::getenv(env))) break;
            }

            if (!tmpenv) tmpenv = "/tmp";

            std::size_t pathlen = strlen(tmpenv)+8;
            path = new char[pathlen];
            snprintf(path, pathlen, "%s/XXXXXX", tmpenv);
            fd = mkstemp(path);
        }
        catch (...) {
            delete[] path;
            throw;
        }
    }

    operator bool() const {
        return path!=nullptr && fd>=0;
    }

    ~temporary_file() {
        delete[] path;
        if (fd>-1) close(fd);
    }
};

TEST(log, source_location) {
    log::source_location here = LOG_LOC;
    //ASSERT_NE(std::string::npos, std::string(here.file).find("test_log.cpp"));
    EXPECT_STRING_HAS(here.file, "test_log.cpp");
    EXPECT_GT(here.line, 0);

    log::source_location later = LOG_LOC;
    EXPECT_GT(later.line, here.line);
}

TEST(log, log_source_location) {
    log::source_location save;
    log::facility_manager mgr([&](const log::log_entry& e) { save = e.location; });

    auto test = log::facility("test", mgr);
    test << log::source_location{"fake.cpp", 37, "foo()"};
    EXPECT_STRING_EQ("fake.cpp", save.file);
    EXPECT_EQ(37, save.line);
    EXPECT_STRING_EQ("foo()", save.func);

    test(0) << log::source_location{"fake.cpp", 54, "foo()"};
    EXPECT_EQ(54, save.line);
}

TEST(log, log_one) {
    std::string name, msg;
    int level;
    int which_sink;

    log::log_sink_t sink1 = [&](const log::log_entry& entry) {
       name = entry.name;
       msg = entry.message;
       level = entry.level;
       which_sink = 1;
    };

    log::log_sink_t sink2 = [&](const log::log_entry& entry) {
       name = entry.name;
       msg = entry.message;
       level = entry.level;
       which_sink = 2;
    };

    log::facility_manager mgr(sink1);
    log::facility test("test", mgr);

    test << "hello";
    EXPECT_STRING_EQ(name, "test");
    EXPECT_EQ(level, 0);
    EXPECT_STRING_EQ(msg, "hello");
    EXPECT_EQ(which_sink, 1);

    test(1) << "there"; // level too high
    EXPECT_EQ(level, 0);
    EXPECT_STRING_EQ(msg, "hello");

    test.level(3);
    test(1) << "there"; // level ok now
    EXPECT_EQ(level, 1);
    EXPECT_STRING_EQ(msg, "there");

    test.sink(sink2);
    test(2) << "matey"; // level ok now
    EXPECT_EQ(level, 2);
    EXPECT_STRING_EQ(msg, "matey");
    EXPECT_EQ(which_sink, 2);
}

TEST(log, stream_sink) {
    using log::flag;
    std::stringstream ss;

    log::stream_sink sink1(ss, flag::noemitloc, flag::emitfac);

    log::facility_manager mgr(sink1);
    log::facility test("test", mgr);
    log::facility fooble("fooble", mgr);

    test << "quux" << 3;
    ASSERT_STRING_HAS(ss.str(), "test");
    ASSERT_STRING_HAS(ss.str(), "quux3");

    fooble << "xyzzy" << 8;
    ASSERT_STRING_HAS(ss.str(), "test");
    ASSERT_STRING_HAS(ss.str(), "quux3");
    ASSERT_STRING_HAS(ss.str(), "fooble");
    ASSERT_STRING_HAS(ss.str(), "xyzzy8");
}

TEST(log, macro) {
    int count = 0;
    std::string message;

    log::default_sink([&](const log::log_entry& e) { message = e.message; });
    log::level(1);

    LOG("foo", 0) << ++count;
    EXPECT_STRING_EQ(message, "1");
    EXPECT_EQ(count, 1);

    // no side-effects
    LOG("foo", 2) << ++count;
    EXPECT_STRING_EQ(message, "1");
    EXPECT_EQ(count, 1);

    LOG("foo", 1) << ++count;
    EXPECT_STRING_EQ(message, "2");
    EXPECT_EQ(count, 2);
}

TEST(log, global_log) {
    std::string message;
    auto saved_sink = log::sink(log::log);

    log::log.sink([&](const log::log_entry& e) { message = e.message; });
    log::log.level(2);

    LOG(2) << "hello";
    EXPECT_STRING_EQ(message, "hello");

    LOG(3) << "there";
    EXPECT_STRING_EQ(message, "hello");

    log::log.sink(saved_sink);
}

TEST(log, rename) {
    std::string fac_name;
    auto saved_sink = log::sink(log::log);

    log::facility_manager mgr;

    auto logger = log::facility("hoopy", mgr);
    logger.sink([&](const log::log_entry& e) { fac_name = e.name; });
    logger << "ding!";
    EXPECT_STRING_EQ(fac_name, "hoopy");
    EXPECT_STRING_EQ("hoopy", logger.name());

    logger.name("frood");
    logger << "ptang!";
    EXPECT_STRING_EQ(fac_name, "frood");
    EXPECT_STRING_EQ("frood", logger.name());

    fac_name = "";
    auto same = log::facility("frood", mgr);
    same << "freeow";
    EXPECT_STRING_EQ(fac_name, "frood");
}

struct slow_stream_sink: public log::stream_sink {
    slow_stream_sink(std::ostream &o):
        log::stream_sink(o, log::flag::noemitloc, log::flag::noemitfac) {}

    // override default message formatter
    void format_message(std::ostream& out, const char* msg) override {
        while (*msg) {
            out << *msg++;
            std::this_thread::yield();
        }
        out << '\n';
    }
};

TEST(log, parallel_sink) {
    using log::flag;
    std::stringstream ss;
    log::facility_manager mgr;

    int nlines = 100;
    int nthread = 4;

    auto action = [&]() {
        std::stringstream id;
        id << std::this_thread::get_id();

        log::facility logger(id.str().c_str(), mgr);
        logger.sink(slow_stream_sink(ss));

        for (int i=0; i<nlines; ++i) {
            logger << "one " << "two " << "three";
        }
    };

    std::vector<std::thread> threads;
    for (int i=0; i<nthread; ++i) {
        threads.push_back(std::thread(action));
    }

    for (auto& h: threads) h.join();

    std::string check;
    for (int i=0; i<nlines*nthread; ++i) {
        check += "one two three\n";
    }

    EXPECT_EQ(check, ss.str());
}

TEST(log, assert_death_test) {
    ASSERT(true) << "nothing to see here";
    auto o_no = []() {
        ASSERT(false) << "o no!";
    };
    EXPECT_DEATH(o_no(), "assertion_failure.*o no");
}

TEST(log, file_sink) {
    temporary_file tmp;
    ASSERT_TRUE(tmp);

    log::log_entry entry = {"log", 1, log::no_source_location, "fancy message"};
    {
        log::file_sink sink(tmp.path);
        sink(entry);
    }

    std::ifstream file(tmp.path);
    std::string line;
    getline(file, line);
    file.close();

    EXPECT_STRING_HAS(line, "fancy message");
}
