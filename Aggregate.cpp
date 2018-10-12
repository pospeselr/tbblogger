// C
#include <string.h>
#include <stdint.h>
#include <malloc.h>
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
    union
    {
        void* executable_memory;
        void __attribute__((__cdecl__)) (*print)();
    };
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

    // allocate executable memory
#ifdef _WIN32
    config.executable_memory = ::VirtualAllocEx( ::GetCurrentProcess(), 0, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
#else
    config.executable_memory = memalign(sysconf(_SC_PAGESIZE), 4096);
    mprotect(config.executable_memory, 4096, (PROT_READ | PROT_WRITE | PROT_EXEC));
#endif

    for(auto msg : messages)
    {
        print_msg(config, msg);
    }

    fflush(config.out_file);

    return 0;
}

void print_msg(const print_config& config, const message* msg)
{
    struct param
    {
        data_type type;
        union {
            const char*     ascii_string_;
            const wchar_t*  wide_string_;
            const void*     pointer_;
            int32_t         i32_;
            uint32_t        u32_;
            int64_t         i64_;
            uint64_t        u64_;
            double          f64_;
            uint8_t         raw[8];
        };

        param()
        {
            ::memset(this, 0x00, sizeof(*this));
        }

        param(param&& that)
        {
            ::memcpy(this, &that, sizeof(that));
            ::memset(&that, 0x00, sizeof(that));
        }

        ~param()
        {
            // wide strings are copied into a properly alligned buffer
            // so we need to delete them when we're done
            if (type == data_type::wide_string)
            {
                delete[] const_cast<wchar_t*>(wide_string_);
            }
        }
    };

    uint32_t len = msg->length;
    const uint8_t* head = reinterpret_cast<const uint8_t*>(msg);
    const uint8_t* tail = head + len;
    head += sizeof(message);

    std::vector<param> params;
    for(int i = 0; head < tail; ++i)
    {
        param current_param;
        current_param.type = (data_type)*head;
        head += sizeof(current_param.type);
        switch(current_param.type)
        {
            case data_type::ascii_string:
                current_param.ascii_string_ = reinterpret_cast<const char*>(head);
                head += strlen(current_param.ascii_string_) + 1;
                break;
            case data_type::wide_string:
                {
                    size_t wlen = 0;
                    wchar_t wc = 0;
                    do
                    {
                        wc = reinterpret_cast<const wchar_t*>(head)[wlen++];
                    } while(wc != 0);
                    current_param.wide_string_ = new wchar_t[wlen];
                    ::memcpy(const_cast<wchar_t*>(current_param.wide_string_), reinterpret_cast<const wchar_t*>(head), wlen * sizeof(wchar_t));
                    head += wlen * sizeof(wchar_t);
                }
                break;
            case data_type::pointer:
                current_param.pointer_ = *reinterpret_cast<const void* const*>(head);
                head += sizeof(void*);
                break;
            case data_type::i8:
                current_param.i32_ = (int32_t)*reinterpret_cast<const int8_t*>(head);
                head += sizeof(int8_t);
                break;
            case data_type::u8:
                current_param.u32_ = (uint32_t)*reinterpret_cast<const uint8_t*>(head);
                head += sizeof(uint8_t);
                break;
            case data_type::i16:
                current_param.i32_ = (int32_t)*reinterpret_cast<const int16_t*>(head);
                head += sizeof(int16_t);
                break;
            case data_type::u16:
                current_param.u32_ = (uint32_t)*reinterpret_cast<const uint16_t*>(head);
                head += sizeof(uint16_t);
                break;
            case data_type::i32:
                current_param.i32_ = *reinterpret_cast<const int32_t*>(head);
                head += sizeof(int32_t);
                break;
            case data_type::u32:
                current_param.u32_ = *reinterpret_cast<const uint32_t*>(head);
                head += sizeof(uint32_t);
                break;
            case data_type::i64:
                current_param.i64_ = *reinterpret_cast<const int64_t*>(head);
                head += sizeof(int64_t);
                break;
            case data_type::u64:
                current_param.u64_ = *reinterpret_cast<const uint64_t*>(head);
                head += sizeof(uint64_t);
                break;
            case data_type::f32:
                current_param.f64_ = (double)*reinterpret_cast<const float*>(head);
                current_param.type = data_type::f64;
                head += sizeof(float);
                break;
            case data_type::f64:
                current_param.f64_ = *reinterpret_cast<const double*>(head);
                head += sizeof(double);
                break;
        }
        params.push_back(std::move(current_param));
    }

    // generate code
    std::vector<uint8_t> code;

    uint32_t esp_add = 0xc;

    // function header
    code.push_back(0x55);   // push ebp
    code.push_back(0x89);   // move ebp, esp
    code.push_back(0xe5);

    // allocate some space on stack for printf?
    code.push_back(0x83);   // sub esp, 0xc
    code.push_back(0xec);
    code.push_back(0x0c);

    for(size_t k = params.size() - 1; k > 2; --k)
    {
        auto& current_param = params[k];
        switch(current_param.type)
        {
            case data_type::i64:
            case data_type::u64:
            case data_type::f64:
                // 64 bit
                // push DWORD
                code.push_back(0x68);
                code.push_back(current_param.raw[4]);
                code.push_back(current_param.raw[5]);
                code.push_back(current_param.raw[6]);
                code.push_back(current_param.raw[7]);
                esp_add += 4;
                break;
            default:
                break;
        }
        // 32 bit
        // push DWORD
        code.push_back(0x68);
        code.push_back(current_param.raw[0]);
        code.push_back(current_param.raw[1]);
        code.push_back(current_param.raw[2]);
        code.push_back(current_param.raw[3]);
        esp_add += 4;
    }

    // destination write buffer
    uint32_t dest_size = 4096;
    char dest[dest_size];
    char* dest_addr = dest;

    // code generation

    // push buffer size
    code.push_back(0x68);
    code.push_back(((uint8_t*)&dest_size)[0]);
    code.push_back(((uint8_t*)&dest_size)[1]);
    code.push_back(((uint8_t*)&dest_size)[2]);
    code.push_back(((uint8_t*)&dest_size)[3]);

    // push buffer address
    code.push_back(0x68);
    code.push_back(((uint8_t*)&dest_addr)[0]);
    code.push_back(((uint8_t*)&dest_addr)[1]);
    code.push_back(((uint8_t*)&dest_addr)[2]);
    code.push_back(((uint8_t*)&dest_addr)[3]);

    // move snprintf to eax
    uint32_t snprintf_addr = (uint32_t)&snprintf;
    code.push_back(0xb8);
    code.push_back(((uint8_t*)(&snprintf_addr))[0]);
    code.push_back(((uint8_t*)(&snprintf_addr))[1]);
    code.push_back(((uint8_t*)(&snprintf_addr))[2]);
    code.push_back(((uint8_t*)(&snprintf_addr))[3]);

    // call snprintf (addr lives in eax)
    code.push_back(0xff);   // call eax
    code.push_back(0xd0);

    // move esp back
    code.push_back(0x81);   // add esp, $esp_add
    code.push_back(0xc4);
    code.push_back(((uint8_t*)(&esp_add))[0]);
    code.push_back(((uint8_t*)(&esp_add))[1]);
    code.push_back(((uint8_t*)(&esp_add))[2]);
    code.push_back(((uint8_t*)(&esp_add))[3]);

    // function footer
    code.push_back(0x89);   // mov exp, ebp
    code.push_back(0xec);
    code.push_back(0x5d);   // pop ebp
    code.push_back(0xc3);   // ret

    // for(auto byte : code)
    // {
    //     printf("%02x ", byte);
    // }
    // printf("\n");
    // fflush(stdout);

    memcpy(config.executable_memory, code.data(), code.size());

    // print the user's message
    (*config.print)();

    // now format the output
    uint64_t timestamp = msg->timestamp - config.begin_timestamp;
    double seconds = timestamp / 1000000000.0;
    auto childid = msg->process_id;
    auto threadid = msg->thread_id;
    auto function = params[0].ascii_string_;
    auto filename = [&]() {
        try {
            return std::string(params[1].ascii_string_).substr(config.filename_offset);
        } catch(...) {
            return std::string("(nil)");
        }
    }();
    auto line = params[2].u32_;

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

    fprintf(config.out_file, "%s\n", dest);
}
