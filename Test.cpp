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
        void micro_sleep(size_t);
    }
}

void __attribute__((noinline)) do_work()
{
    // TBB_TRACE();
    // int some_int = 100;
    // double some_double = 34.0;
    // TBB_LOG("This is some logging %i %f", some_int, some_double);
    // TBB_LOG("whoa");
    // // tbb::internal::micro_sleep(rand() % 100);
    // TBB_LOG("%p", nullptr);
    // // tbb::internal::micro_sleep(rand() % 5000);
    // TBB_LOG("%f %s %S", 1.0f, "char", L"wchar");
    // // tbb::internal::micro_sleep(rand() % 1000);
    TBB_LOG("string test: '{}' '{}' '{}' '{}'", u8"utf8", u"utf16", U"utf32", L"wide");
    TBB_LOG("pointer: {}", nullptr);
    void* stack;
    TBB_LOG("stack ptr: {}", &stack);
}

int main(int argc, char** argv)
{
    srand(0);
    // omp_set_dynamic(0);
    // omp_set_num_threads(4);

    constexpr double NANOSECONDS_PER_SECOND = 1000000000.0;
    size_t begin = tbb::internal::get_timestamp();
    // #pragma omp parallel for
    for(size_t k = 0; k < 1; ++k)
    {
        do_work();
    }
    size_t end = tbb::internal::get_timestamp();

    printf("Time Logging: %f seconds\n", (double)(end - begin) / NANOSECONDS_PER_SECOND);
}