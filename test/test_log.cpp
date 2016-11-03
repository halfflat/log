#include "gtest.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <log/log.hpp>

TEST(log, source_location) {
    log::source_location here = LOG_LOC;
    ASSERT_NE(std::string::npos, std::string(here.file).find("test_log.cpp"));
    ASSERT_GT(here.line, 0);

    log::source_location later = LOG_LOC;
    ASSERT_GT(later.line, here.line);
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
    ASSERT_EQ(name, "test");
    ASSERT_EQ(level, 0);
    ASSERT_EQ(msg, "hello");
    ASSERT_EQ(which_sink, 1);

    test(1) << "there"; // level too high
    ASSERT_EQ(level, 0);
    ASSERT_EQ(msg, "hello");

    test.level(3);
    test(1) << "there"; // level ok now
    ASSERT_EQ(level, 1);
    ASSERT_EQ(msg, "there");

    test.sink(sink2);
    test(2) << "matey"; // level ok now
    ASSERT_EQ(level, 2);
    ASSERT_EQ(msg, "matey");
    ASSERT_EQ(which_sink, 2);
}

TEST(log, stream_sink) {
    using log::flag;
    std::stringstream ss;

    log::stream_sink sink1(ss, flag::noemitloc, flag::emitfac);

    log::facility_manager mgr(sink1);
    log::facility test("test", mgr);
    log::facility fooble("fooble", mgr);

    test << "quux" << 3;
    ASSERT_NE(std::string::npos, ss.str().find("test"));
    ASSERT_NE(std::string::npos, ss.str().find("quux3"));

    fooble << "xyzzy" << 8;
    ASSERT_NE(std::string::npos, ss.str().find("test"));
    ASSERT_NE(std::string::npos, ss.str().find("quux3"));
    ASSERT_NE(std::string::npos, ss.str().find("fooble"));
    ASSERT_NE(std::string::npos, ss.str().find("xyzzy8"));
}

TEST(log, macro) {
    int count = 0;
    std::string message;

    log::default_sink([&](const log::log_entry& e) { message = e.message; });
    log::level(1);

    LOG("foo", 0) << ++count;
    EXPECT_EQ(message, "1");
    EXPECT_EQ(count, 1);

    // no side-effects
    LOG("foo", 2) << ++count;
    EXPECT_EQ(message, "1");
    EXPECT_EQ(count, 1);

    LOG("foo", 1) << ++count;
    EXPECT_EQ(message, "2");
    EXPECT_EQ(count, 2);
}

TEST(log, global_log) {
    std::string message;
    auto saved_sink = log::sink(log::log);

    log::log.sink([&](const log::log_entry& e) { message = e.message; });
    log::log.level(2);

    LOG(2) << "hello";
    EXPECT_EQ(message, "hello");

    LOG(3) << "there";
    EXPECT_EQ(message, "hello");

    log::log.sink(saved_sink);
}

TEST(log, rename) {
    std::string fac_name;
    auto saved_sink = log::sink(log::log);

    log::facility_manager mgr;

    auto logger = log::facility("hoopy", mgr);
    logger.sink([&](const log::log_entry& e) { fac_name = e.name; });
    logger << "ding!";
    EXPECT_EQ(fac_name, "hoopy");
    EXPECT_EQ(std::string("hoopy"), logger.name());

    logger.name("frood");
    logger << "ptang!";
    EXPECT_EQ(fac_name, "frood");
    EXPECT_EQ(std::string("frood"), logger.name());

    fac_name = "";
    auto same = log::facility("frood", mgr);
    same << "freeow";
    EXPECT_EQ(fac_name, "frood");
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
