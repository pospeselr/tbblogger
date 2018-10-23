# TbbLogger

Minimal offline logger for Firefox using [fmtlib](http://fmtlib.net/).  Logging calls are serialized at runtime to per-process binary files, which can be aggregated together with the 'aggregate' tool.

## Usage

Apply TbbLogger.patch to your firefox source. You will be able to add [fmtlib](http://fmtlib.net/latest/syntax.html) logging functionality by including the TbbLogger.h header:

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
    TBB_LOG("hex : {0:#x} dec : {0} bin : {0:#032b}", 128);
}
...
```

Logged messages are serialized to binary blobs living in `/tmp/firefox/firefoxN.bin` (on Linux) or `C:\Users\%USERNAME%\Temp\firefox\firefoxN.bin` (on Windows).  These blobs can be squashed together into human-readable text using the aggregate tool built via:

```bash
# build linux aggregate tool
make aggregate
# build windows aggregate tool (requires mingw)
make win_aggregate
```

The default output format for each log entry is:

```bash
[$FIREFOX_PROCESS][$THREAD_ID] $FUNCTION_NAME in $FILENAME:$LINENUMBER $MESSAGE
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

- On Linux, firefox's `security.sadbox.content.level` pref must be reduced to 0
