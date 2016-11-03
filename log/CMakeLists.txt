set(sources "facility.cpp" "log_standard.cpp")
set(headers "facility.hpp" "locked_ostream.hpp" "log.hpp" "sinks.hpp")

add_library(log ${sources})

target_include_directories(log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

install(TARGETS log ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include FILES_MATCHING PATTERN "*.hpp")
