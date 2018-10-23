// C
#include <string.h>
#include <stdint.h>
#include <malloc.h>
#include <assert.h>
// C++
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

#include "TbbLogger.h"

static_assert(sizeof(void*) == sizeof(uint32_t), "pointer type must be 32 bit for code generation to work properly");

using tbb::serialization::data_type;
using tbb::serialization::message;

typedef enum
{
    OPTIONS_NONE = 0,
    OPTIONS_HIDE_TIMESTAMP = 1,
    OPTIONS_HIDE_CHILDID = 2,
    OPTIONS_HIDE_THREADID = 4,
    OPTIONS_HIDE_LOGSITE = 8,
} aggregate_options_t;

struct print_config
{
    aggregate_options_t options;
    uint32_t filename_offset;
    uint64_t begin_timestamp;
    FILE* out_file;
};

void print_msg(const print_config& config, const message* msg);

void print_help() {
    printf(
        "Usage: aggregate [OPTION]... [FILE]... -o output.log\n"
        "Options:\n"
        " --help                 Print this help message\n"
        " --filename-offset=N    Skips first N characters when logging filename\n"
        " --hide-timestamp       Do not print log entry's timestamp\n"
        " --hide-childid         Do not print log entry's child id\n"
        " --hide-threadid        Do not print log entry's thread id\n"
        " --hide-logsite         Do not print log entry's log site\n");
}

int main(int argc, char** argv)
{
    if (argc == 1) {
        print_help();
        return -1;
    }

    std::vector<std::string> args;
    // start at 1, skip over ./aggregate
    for(int k = 1; k < argc; ++k)
    {
        args.push_back(argv[k]);
    }

    print_config config =
    {
        OPTIONS_NONE,
        0u,
        0ull,
        nullptr,
    };

    // parse options, get filenames
    std::vector<std::string> log_bins;
    std::string output_filename = "";
    for(size_t k = 0; k < args.size(); ++k)
    {
        auto& current_arg = args[k];
        if (current_arg == "--help") {
            print_help();
            return -1;
        } else if (current_arg == "--hide-timestamp") {
            config.options = aggregate_options_t(config.options | OPTIONS_HIDE_TIMESTAMP);
        } else if (current_arg == "--hide-childid") {
            config.options = aggregate_options_t(config.options | OPTIONS_HIDE_CHILDID);
        } else if (current_arg == "--hide-threadid") {
            config.options = aggregate_options_t(config.options | OPTIONS_HIDE_THREADID);
        } else if (current_arg == "--hide-logsite") {
            config.options = aggregate_options_t(config.options | OPTIONS_HIDE_LOGSITE);
        } else if (current_arg.find("--filename-offset=", 0) == 0) {
            int32_t filename_offset = 0;
            if (sscanf(current_arg.c_str(), "--filename-offset=%i", &filename_offset) != 1 || filename_offset < 0) {
                printf("Error parsing %s\n", current_arg.c_str());
                return -1;
            }
            config.filename_offset = filename_offset;
        } else if (current_arg == "-o") {
            if (output_filename != "") {
                printf("Duplicate -o option\n");
                return -1;
            }
            if (++k == args.size()) {
                printf("Missing name for -o option\n");
                return -1;
            }
            auto& current_arg = args[k];
            output_filename = current_arg;
        } else if (current_arg.find("-", 0) == 0) {
            printf("Unknown option: '%s'\n", current_arg.c_str());
            return -1;
        } else {
            log_bins.push_back(current_arg);
        }
    }

    if(output_filename == "") {
        config.out_file = stdout;
    } else {
        config.out_file = fopen(output_filename.c_str(), "wb");
        if (config.out_file == nullptr) {
            printf("Error opening output file: '%s'\n", output_filename.c_str());
            return -1;
        }
    }

    // bring logs in from disk
    std::vector<message*> messages;
    for (auto& current_log : log_bins)
    {
        FILE* log_file = fopen(current_log.c_str(), "rb");
        if (log_file) {
            while(true)
            {
                fpos_t fpos;
                fgetpos(log_file, &fpos);

                uint32_t len;
                if(fread(&len, sizeof(len), 1, log_file) != 1)
                {
                    break;
                }

                fsetpos(log_file, &fpos);
                message* msg = (message*)malloc(len);
                if(fread(msg, 1, len, log_file) != len)
                {
                    free(msg);
                    break;
                }
                messages.push_back(msg);
            }
            fclose(log_file);
        } else {
            printf("Error opening log file: '%s'\n", current_log.c_str());
            return -1;
        }
    }

    // stable sort by timestamp
    // messages from the same thread with the same timestamp will appear in correct
    // order sine it's a stable sort
    std::stable_sort (messages.begin(), messages.end(), [](message* a, message* b)
    {
        return (a->timestamp) < (b->timestamp);
    });

    config.begin_timestamp = messages.front()->timestamp;

    for(auto msg : messages)
    {
        print_msg(config, msg);
    }

    fflush(config.out_file);

    return 0;
}

template<typename CharType>
constexpr size_t string_length(const CharType* str)
{
    const auto* head = str;
    while(*str++ != (CharType)0);
    return (str - head);
}

struct param
{
    data_type type;
    union {
        char*     utf8_;
        char16_t* utf16_;
        char32_t* utf32_;
        uint32_t  p32_;
        uint64_t  p64_;
        int8_t    i8_;
        uint8_t   u8_;
        int16_t   i16_;
        uint16_t  u16_;
        int32_t   i32_;
        uint32_t  u32_;
        int64_t   i64_;
        uint64_t  u64_;
        float     f32_;
        double    f64_;
    } value;

    param()
    : type(data_type::invalid)
    , value({0})
    { }

    param(param&& that)
    {
        ::memcpy(this, &that, sizeof(that));
        ::memset(&that, 0x00, sizeof(that));
    }

    ~param()
    {
        // strings are copied to a new properly aligned buffer
        if (type == data_type::utf8)  delete[] value.utf8_;
        if (type == data_type::utf16) delete[] value.utf16_;
        if (type == data_type::utf32) delete[] value.utf32_;
        ::memset(this, 0x00, sizeof(*this));
    }
};

void print_msg(const print_config& config, const message* msg)
{

    uint32_t len = msg->length;
    const uint8_t* head = reinterpret_cast<const uint8_t*>(msg);
    const uint8_t* tail = head + len;
    head += sizeof(message);

    std::vector<param> params;
    for(int i = 0; head < tail; ++i)
    {
        param current_param;
        current_param.type = (data_type)*head;
        assert(current_param.type != data_type::invalid);
        head += sizeof(current_param.type);
        switch(current_param.type)
        {
            default:
                assert(!"Invalid data_type");
                break;
            case data_type::utf8:
            {
                const size_t len = string_length(reinterpret_cast<const char*>(head));
                current_param.value.utf8_ = new char[len];
                ::memcpy(current_param.value.utf8_, head, len * sizeof(char));
                head += len * sizeof(char);
                break;
            }
            case data_type::utf16:
            {
                const size_t len = string_length(reinterpret_cast<const char16_t*>(head));
                current_param.value.utf16_ = new char16_t[len];
                ::memcpy(current_param.value.utf16_, head, len * sizeof(char16_t));
                head += len * sizeof(char16_t);
                break;
            }
            case data_type::utf32:
            {
                const size_t len = string_length(reinterpret_cast<const char32_t*>(head));
                current_param.value.utf32_ = new char32_t[len];
                ::memcpy(current_param.value.utf32_, head, len * sizeof(char32_t));
                head += len * sizeof(char32_t);
                break;
            }
            case data_type::p32:
                current_param.value.p32_ = *reinterpret_cast<const uint32_t*>(head);
                head += sizeof(uint32_t);
                break;
            case data_type::p64:
                current_param.value.p64_ = *reinterpret_cast<const uint64_t*>(head);
                head += sizeof(uint64_t);
                break;
            case data_type::i8:
                current_param.value.i8_ = *reinterpret_cast<const int8_t*>(head);
                head += sizeof(int8_t);
                break;
            case data_type::u8:
                current_param.value.u8_ = *reinterpret_cast<const uint8_t*>(head);
                head += sizeof(uint8_t);
                break;
            case data_type::i16:
                current_param.value.i16_ = *reinterpret_cast<const int16_t*>(head);
                head += sizeof(int16_t);
                break;
            case data_type::u16:
                current_param.value.u16_ = *reinterpret_cast<const uint16_t*>(head);
                head += sizeof(uint16_t);
                break;
            case data_type::i32:
                current_param.value.i32_ = *reinterpret_cast<const int32_t*>(head);
                head += sizeof(int32_t);
                break;
            case data_type::u32:
                current_param.value.u32_ = *reinterpret_cast<const uint32_t*>(head);
                head += sizeof(uint32_t);
                break;
            case data_type::i64:
                current_param.value.i64_ = *reinterpret_cast<const int64_t*>(head);
                head += sizeof(int64_t);
                break;
            case data_type::u64:
                current_param.value.u64_ = *reinterpret_cast<const uint64_t*>(head);
                head += sizeof(uint64_t);
                break;
            case data_type::f32:
                current_param.value.f32_ = *reinterpret_cast<const float*>(head);
                head += sizeof(float);
                break;
            case data_type::f64:
                current_param.value.f64_ = *reinterpret_cast<const double*>(head);
                head += sizeof(double);
                break;
        }
        params.push_back(std::move(current_param));
    }

    // print the user's message
    // (*config.print)();

    // now format the output
    uint64_t timestamp = msg->timestamp - config.begin_timestamp;
    double seconds = timestamp / 1000000000.0;
    auto childid = msg->process_id;
    auto threadid = msg->thread_id;
    auto function = params[0].value.utf8_;
    auto filename = [&]() {
        try {
            return std::string(params[1].value.utf8_).substr(config.filename_offset);
        } catch(...) {
            return std::string("(nil)");
        }
    }();
    auto line = params[2].value.u32_;

    if (!(config.options & OPTIONS_HIDE_TIMESTAMP)) {
        fprintf(config.out_file, "[%f]", seconds);
    }
    if (!(config.options & OPTIONS_HIDE_CHILDID)) {
        if (childid == 0) {
            fprintf(config.out_file, "[Parent]");
        } else {
            fprintf(config.out_file, "[Child%u]", childid);
        }
    }
    if (!(config.options & OPTIONS_HIDE_THREADID)) {
        fprintf(config.out_file, "[%u]", threadid);
    }
    if (!(config.options & OPTIONS_HIDE_LOGSITE)) {
        fprintf(config.out_file, " %s in %s:%u ", function, filename.c_str(), line);
    }

    // fprintf(config.out_file, "%s\n", dest);
}
