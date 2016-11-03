# Small logging library

The `log` library is a small (but not single-header) library
for helping to manage debug, informational and error output.

## Sample usage

Using default log facilities.
```
log::level("debug", 2);
DEBUG(1) << "Debug message, level 1.";
LOG(0) << "Log to default logger, level 0.";
```

Create facilities on the fly.
```
LOG("solver",1) << "This goes to the 'solver' facility."
```

Customize handling of log records.
```
log::default_sink(my_log_handler); // set default handler
log::sink("solver", my_other_log_handler); // set handler for 'solver'
```

Use macro-forms to avoid evaluation of message.
```
LOG("solver",3) << expensive_message();
```

Use function forms to ensure evaluation of message.
Note that source location info, if required, has to be supplied 'by hand'.
```
log::log("solver",3) << LOG_LOC << i_have_side_effects();
```

Facilities can be referenced by name or by object.
```
LOG("solver") << "A message.";

auto solver = log::facility("solver");
LOG(solver) << "A second message (with source location).";
solver << "A third message, omitting source location.";
```

Some simple log handlers are included
```
log::sink(log::file_sink("run.log", log::flag::flush, log::flasg::noemitloc));
```
and these can be extended by sub-classing.
```
struct my_stream_sink: public log::stream_sink {
    template <typename... Args>
    my_sink(Args&&... args): log::stream_sink(std::forward<Args>(args)...) {}

protected:
    void format_location(std::ostream& out, log::source_location loc) override {
        out << "FILE: " << loc.file << "; ";
    }
};

log::sink(my_stream_sink(std::cerr));
```

## Design

Goals:
* multiple logging facilities;
* each facility can have an independent handler or log level;
* facilities can be referenced by name or by object;
* one can log with forced or conditional evaluation of the
  message;
* facility creation and parameter setting is thread safe.

### Facilities

A facility is described by a tuple: _name_, _level_, _sink_.
Responsibility for facilities lies with a `facility_manager`;
there is a global `facility_manager` that is used by default.

When a facility is constructed from a `const char*` name,
an existing facility is retrieved from the manager, or
a new one constructed if none yet exist with that name.
Newly created facilities adopt the manager's current
default sink and log level.

Facilities are used for logging with `operator()`
(taking a message level) or directly as the left-hand
operand of `operator<<`. These both create a temporary
`sink_stream` object, derived from `std::ostream`, that
sends the composed message to the facility's sink on
destruction. The end of a log entry is implictly determined
by the end of the logging statement:
```
logger << "This all comprises exactly " << 1 << " record.";
```

Source location information is provided by writing a `source_location`
object to the `sink_stream`. A `source_location` corresponding to
the current source line is created by the macro `LOG_LOC`; this is
added automatically when one of the logging macros (see below) is
used to write an entry to the logging facility.
```
logger << source_location{"file.cc", 200, "foo()"} << "with explicit line info.";
logger << LOG_LOC << "with line info for this source line.";
LOG(logger) << "line info included automatically.";
```

### Sinks

A log entry produced by a facility is represented by a `log_entry` structure
with fields for the facility name, the message level, the source location,
and the message text. 

Sinks are represented by a `std::function<void (const log::log_entry&)>` object;
the objects refered to by fields in the `log_entry` are not guaranteed to have a
lifetime longer than that of the `log_entry` object itself.

### Macros

The `LOG` macro dispatches on the number of arguments (one or two).
`LOG(n)` is equivalent to `LOG(::log::log, n)` (where `log::log` is the default
logging facility).

`LOG(fac, n)` expands to
```
if (auto log_magic_reserved_temp_ = ::log::log_test_proxy(::log::facility(fac)(n))) ;
else log_magic_reserved_temp_.stream << LOG_LOC
```
Here `log::log_test_proxy` returns a wrapper which converts to false if and only if
the wrapped stream is in a good state.

Two other macros correspond to the predefined streams `debug` and `assertion_failure`.
`DEBUG(n)` is equivalent to `LOG(::log::debug, n)`, unless `LOG_NDEBUG` is defined,
in which case it expands to a no-op.

`ASSERT(test)` is equivalent to
```
if (!(test)) ; else ::log::assertion_failure << LOG_LOC
```
unless `LOG_NASSERT` is defined, in which case it expands to a no-op.


## Default sinks

A customizable sink object `log::stream_sink` will write log records to a supplied
stream, with behaviour governed by flags controlling whether to print source locations,
or flush the stream after each record.

`log::stream_sink` uses `log::locked_ostream` to coordinate access to streams shared
across multiple sinks and to maintain independent formatting state.

## API

WIP...

