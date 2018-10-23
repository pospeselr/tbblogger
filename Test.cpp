#include "TbbLogger.h"
// #include <omp.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// C++
#include <vector>
#include <sstream>

namespace tbb
{
    namespace internal
    {
        uint64_t get_timestamp();
        size_t get_temp_path(char* buffer, size_t len);
    }
}

void logging()
{
    TBB_LOG("string test: '{}' '{}' '{}' '{}'", u8"utf8", u"utf16", U"utf32", L"wide");
    TBB_LOG("null pointer: {}", nullptr);
    int local;
    TBB_LOG("stack ptr: {}", &local);
    TBB_LOG("hex : {0:#x} dec : {0} bin : {0:#016b}", (uint16_t)128);
}

int main(int argc, char** argv)
{
    // initial log so that logger thread spinning up isn't counted in timing
    TBB_TRACE();
    srand(0);

    char temp[1024];
    tbb::internal::get_temp_path(temp, sizeof(temp));
    printf("Temp Directory: %s\n", temp);

    // omp_set_dynamic(0);
    // omp_set_num_threads(4);

    constexpr double NANOSECONDS_PER_SECOND = 1000000000.0;
    size_t begin = tbb::internal::get_timestamp();
    // #pragma omp parallel for
    // for(size_t k = 0; k < 10000; ++k)
    {
        logging();
    }
    size_t end = tbb::internal::get_timestamp();

    printf("Time Logging: %f seconds\n", (double)(end - begin) / NANOSECONDS_PER_SECOND);
}