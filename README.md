# TbbLogger

Minimal offline logger for Firefox.  Logging calls are serialized at runtime to per-process binary files, which can be aggregated together with the 'aggregate' tool.

## Usage

Apply the provided patch to your firefox source.  You should be able to add logging functionality anywhere via:

```
    ...
    #include <mozilla/TbbLogger.h
    ...
    void function_name()
    {
        ...
        // outputs : [0.000323][Parent][29958] function_name in Test.cpp:18 
        TBB_TRACE();
        int some_int = 100;
        double some_double = 34.0;
        // outputs : [0.000324][Parent][29958] function_name in Test.cpp:22 This is some logging 100 34.000000
        TBB_LOG("This is some logging %i %f", some_int, some_double);  
        ...
    }
```

Logged messages are serialized to binary blobs living in `/tmp/firefox/firefoxN.bin` (on Linux) or `C:\Users\%USERNAME%\Temp\firefox\firefoxN.bin` (on Windows).  These blobs can be squashed together into human-readable text using the aggregate tool built via:

```
    # build linux aggregate tool
    make aggregate
    # build windows aggregate tool (requires mingw)
    make win_aggregate
```

The default output format for each log entry is:

```
    [$TIME_SINCE_FIRST_LOG_IN_SECONDS][$FIREFOX_PROCESS][$THREAD_ID] $FUNCTION_NAME in $FILENAME:$LINENUMBER $MESSAGE
```

Each of those pieces of information can optionally removed using various switches on aggregate tool.   See aggregate --help:

```
    Usage: aggregate [OPTION]... [FILE]... -o output.log
    Options:
     --help                 Print this help message
     --filename-offset=N    Skips first N characters when logging filename
     --hide-timestamp       Do not print log entry's timestamp
     --hide-childid         Do not print log entry's child id
     --hide-threadid        Do not print log entry's thread id
     --hide-logsite         Do not print log entry's log site
```

aggregate will print stdout if an output file is not specified.

## Caveats

- Aggregator must be built for 32-bit as it makes use of x86 code-generation to invoke snprintf with the serialized args in the generated bin files.
- Wide strings (wchar_t) are different sized on different platforms.  Therefore, any binary logs containing wide strings must be converted using the aggregator binary for the same platform.
- Pointer arguments are also specific to the architecture, don't printf("%p") in 64-bit firefox and expect it to work in the 32-bit aggregator.