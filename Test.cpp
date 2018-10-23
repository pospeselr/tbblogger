#include "TbbLogger.h"
// #include <omp.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// C++
#include <vector>
#include <sstream>

// fmt
#include <fmt/format.h>
#include <fmt/printf.h>

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

struct catchall_type
{
    void* data = nullptr;
};

namespace fmt {
    template<>
    struct formatter<catchall_type> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext &ctx)
        {
            auto it = ctx.begin();
            while (*it != '}') {
                ++it;
            }
            std::stringstream ss;
            ss << "{:" << std::string(ctx.begin(), it) << '}';
            format_string = ss.str();
            printf("format string: %s\n", format_string.data());
            return it;
        }

        template <typename FormatContext>
        auto format(const catchall_type &cat, FormatContext &ctx)
        {
            return format_to(ctx.begin(), format_string, 0x128);
        }

        std::string format_string;
    };
}

namespace fmt{
    namespace v5 {
        namespace internal {
            template<>
            fmt::basic_format_arg<fmt::format_context> make_arg<fmt::format_context, catchall_type>(const catchall_type& cat) {
                fmt::basic_format_arg<fmt::format_context> retval;
                printf("%s:%u\n", __FUNCTION__, __LINE__);
                retval.value_ = value<fmt::format_context>(cat);
                retval.type_ = custom_type;
                return retval;
            }
        }
    }
}

void fmt_test()
{
    using ctx = fmt::format_context;
    std::vector<fmt::basic_format_arg<ctx>> args;

    catchall_type cat;
    args.push_back(fmt::internal::make_arg<ctx, catchall_type>(cat));

    auto result = fmt::vformat("hex: 0x{0:x} dec: {0:}", fmt::basic_format_args<ctx>(args.data(), (uint32_t)args.size()));
    puts(result.data());
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

    fmt::printf("Time Logging: %f seconds\n", (double)(end - begin) / NANOSECONDS_PER_SECOND);

    fmt_test();
}