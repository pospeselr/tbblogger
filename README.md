# TbbLogger

Minimal offline logger for Firefox using [fmtlib](http://fmtlib.net/).  Logging calls are serialized at runtime to per-process binary files, which can be aggregated together with the 'aggregate' tool.

## Usage

Apply TbbLogger.patch to your firefox source. This patch does a couple of things:

1. Adds TbbLogger.cpp and TbbLogger.h to the firefox build
2. Adds %TEMP% to the directory whitelist on Windows
3. Adds initial TBB_LOG messages toward the beginning of the parent and child process launch logic

You will be able to add fmtlib-style logging functionality by including the TbbLogger.h header (see http://fmtlib.net/latest/syntax.html for fmtlib syntax).

```cpp
...
#include "TbbLogger.h"
...
void logging()
{
    TBB_LOG("string test: '{}' '{}' '{}' '{}'", u8"utf8", u"utf16", U"utf32", L"wide");
    TBB_LOG("null pointer: {}", nullptr);
    int local;
    TBB_LOG("stack ptr: {}", &local);
    TBB_LOG("hex : {0:#x} dec : {0} bin : {0:#016b}", (uint16_t)128);
}
...
```

Logged messages are serialized to binary blobs living in `/tmp/firefox/firefoxN.bin` (on Linux) or `C:\Users\%USERNAME%\Temp\firefox\firefoxN.bin` (on Windows).  These blobs can be combined together and converted into human-readable text using the aggregate tool built via:

```bash
# build linux aggregate tool
$ make aggregate
# build windows aggregate tool (requires mingw)
$ make win_aggregate
```

Messages appear in order sorted by timestamp.  The default output format for each log entry is:

```bash
[$TIMESTAMP][$FIREFOX_PROCESS][$THREAD_ID] $FUNCTION_NAME in $FILENAME:$LINENUMBER $MESSAGE
```

Each of those pieces of information can optionally removed using various switches on aggregate tool.   See aggregate --help:

```
$ ./aggregate --help
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

# Example

```
$ ./aggregate /tmp/firefox/*.bin --hide-threadid
[0.000000][Parent] main in Test.cpp:32
[0.000134][Parent] logging in Test.cpp:22 string test: 'utf8' 'utf16' 'utf32' 'wide'
[0.000143][Parent] logging in Test.cpp:23 null pointer: 0x0000000000000000
[0.000147][Parent] logging in Test.cpp:25 stack ptr: 0x00007ffe180ef59c
[0.000149][Parent] logging in Test.cpp:26 hex : 0x80 dec : 128 bin : 0b00000010000000

```

## Caveats

- On Linux, firefox's `security.sandbox.content.level` pref must be reduced to 0
