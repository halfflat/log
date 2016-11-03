add_library(gtest gtest-all.cpp)

include_directories(.)
add_executable(test_log test_main.cpp test_log.cpp)

target_link_libraries(test_log LINK_PUBLIC log gtest)
