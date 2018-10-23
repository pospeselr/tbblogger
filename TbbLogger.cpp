#include "TbbLogger.h"

// C
#include <assert.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>

// C++
#include <atomic>
#include <memory>
#include <vector>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#endif

namespace tbb
{
    namespace internal
    {
        /// Utilities

        // timestamp in nanoseconds
        uint64_t get_timestamp()
        {
            constexpr uint64_t NANOSECONDS_PER_SECOND = 1000000000;
#ifdef _WIN32
            static LARGE_INTEGER frequency = []()
            {
                LARGE_INTEGER frequency;
                QueryPerformanceFrequency(&frequency);
                return frequency;
            }();

            LARGE_INTEGER ticks;
            QueryPerformanceCounter(&ticks);
            ticks.QuadPart *= NANOSECONDS_PER_SECOND;
            ticks.QuadPart /= frequency.QuadPart;

            return ticks.QuadPart;
#else
            timespec counter;
            clock_gettime(CLOCK_MONOTONIC_RAW, &counter);
            uint64_t retval = uint64_t(counter.tv_sec) * NANOSECONDS_PER_SECOND + uint64_t(counter.tv_nsec);
            return retval;
#endif
        }

        uint32_t get_thread_id()
        {

#ifdef _WIN32
            static thread_local uint32_t thread_id = GetCurrentThreadId();
#else
            static thread_local uint32_t thread_id = syscall(SYS_gettid);
#endif
            return thread_id;
        }

        void thread_yield()
        {
#ifdef _WIN32
            SwitchToThread();
#else
            pthread_yield();
#endif
        }

        const char* get_command_line()
        {
#ifdef _WIN32
            return GetCommandLineA();
#else
            static char cmdline[2048];
            FILE* cmdline_file = fopen("/proc/self/cmdline", "rb");
            size_t len = fread(cmdline, 1, sizeof(cmdline) - 1, cmdline_file);
            fclose(cmdline_file);
            for(size_t i = 0; i < len; ++i)
            {
                cmdline[i] = (cmdline[i] == 0 ? ' ' : cmdline[i]);
            }
            cmdline[len] = 0;
            return cmdline;
#endif
        }

        size_t get_temp_path(char* buffer, size_t len)
        {
#ifdef _WIN32
            return GetTempPathA(len, buffer);
#else
            const char tmp[] = "/tmp";
            memcpy(buffer, tmp, sizeof(tmp));
            return sizeof(tmp) - 1;
#endif
        }


        int32_t get_child_id()
        {
            const char* command_line = get_command_line();
            command_line = strstr(command_line, "-childID");
            if (command_line == nullptr) {
                return 0;
            }

            int32_t retval = -1;
            sscanf(command_line, "-childID %d", &retval);
            return retval;
        }

        FILE* get_log_file(int32_t childID)
        {
            char filename[1024];
            char* head = filename;
            head += get_temp_path(head, sizeof(filename));
#if _WIN32
            head += sprintf(head, "firefox");
            CreateDirectoryA(filename, nullptr);
            sprintf(head, "\\firefox%i.bin", childID);
#else
            head += sprintf(head, "/firefox");
            mkdir(filename, 0777);
            sprintf(head, "/firefox%i.bin", childID);
#endif
            return fopen(filename, "wb");
        }
    }

    namespace serialization
    {
        #define NULL_STRING(PREFIX) PREFIX##"(nil)"
        #define UTF8_NULLSTRING   NULL_STRING(u8)
        #define UTF16_NULLSTRING  NULL_STRING(u)
        #define UTF32_NULLSTRING  NULL_STRING(U)
        #define WIDE_NULLSTRING  NULL_STRING(L)

        template<typename CharType>
        constexpr size_t param_size_string(const CharType* str)
        {
            const auto* head = str;
            while(*str++ != (CharType)0);
            return (str - head) * sizeof(CharType) + sizeof(data_type);
        }

        /// Size Params
        size_t param_size_impl(const char* str)
        {
            if (!str) {
                str = UTF8_NULLSTRING;
            }
            return param_size_string(str);
        }
        size_t param_size_impl(const char16_t* str)
        {
            if (!str) {
                str = UTF16_NULLSTRING;
            }
            return param_size_string(str);
        }
        size_t param_size_impl(const char32_t* str)
        {
            if (!str) {
                str = UTF32_NULLSTRING;
            }
            return param_size_string(str);
        }
        size_t param_size_impl(const wchar_t* str)
        {
            if (!str) {
                str = WIDE_NULLSTRING;
            }
            return param_size_string(str);
        }

        size_t param_size(char str[])
        {
            return param_size_impl(str);
        }
        size_t param_size(char16_t str[])
        {
            return param_size_impl(str);
        }
        size_t param_size(char32_t str[])
        {
            return param_size_impl(str);
        }
        size_t param_size(wchar_t str[])
        {
            return param_size_impl(str);
        }

        /// Pack Params
        template<typename CharType>
        uint8_t* pack_param_string(uint8_t* dest, const CharType* str)
        {
            static_assert((sizeof(CharType) == sizeof(char) ||
                           sizeof(CharType) == sizeof(char16_t) ||
                           sizeof(CharType) == sizeof(char32_t)), "Invalid character size");
            switch(sizeof(CharType))
            {
                case sizeof(char):
                    *dest++ = (uint8_t)data_type::utf8; break;
                case sizeof(char16_t):
                    *dest++ = (uint8_t)data_type::utf16; break;
                case sizeof(char32_t):
                    *dest++ = (uint8_t)data_type::utf32; break;
            }
            auto* str_dest = reinterpret_cast<CharType*>(dest);
            while((*str_dest++ = *str++));
            return reinterpret_cast<uint8_t*>(str_dest);
        }

        // various string overloads
        uint8_t* pack_param_impl(uint8_t* dest, const char* str)
        {
            if (!str) {
                str = UTF8_NULLSTRING;
            }
            return pack_param_string(dest, str);
        }
        uint8_t* pack_param_impl(uint8_t* dest, const char16_t* str)
        {
            if (!str) {
                str = UTF16_NULLSTRING;
            }
            return pack_param_string(dest, str);
        }
        uint8_t* pack_param_impl(uint8_t* dest, const char32_t* str)
        {
            if (!str) {
                str = UTF32_NULLSTRING;
            }
            return pack_param_string(dest, str);
        }
        uint8_t* pack_param_impl(uint8_t* dest, const wchar_t* str)
        {
            if (!str) {
                str = WIDE_NULLSTRING;
            }
            return pack_param_string(dest, str);
        }
        uint8_t* pack_param_impl(uint8_t* dest, char str[])
        {
            return pack_param_impl(dest, (const char*)str);
        }
        uint8_t* pack_param_impl(uint8_t* dest, char16_t str[])
        {
            return pack_param_impl(dest, (const char16_t*)str);
        }
        uint8_t* pack_param_impl(uint8_t* dest, char32_t str[])
        {
            return pack_param_impl(dest, (const char32_t*)str);
        }
        uint8_t* pack_param_impl(uint8_t* dest, wchar_t str[])
        {
            return pack_param_impl(dest, (const wchar_t*)str);
        }

        uint8_t* pack_param_impl(uint8_t* dest, const void* ptr)
        {
            static_assert(sizeof(void*) == sizeof(uint32_t) ||
                          sizeof(void*) == sizeof(uint64_t),
                          "Invalid pointer size");

            constexpr size_t N = sizeof(void*);
            if(N == sizeof(uint32_t)) {
                *dest++ = (uint8_t)(data_type::p32);
            } else if(N == sizeof(uint64_t)) {
                *dest++ = (uint8_t)(data_type::p64);
            }

            memcpy(dest, &ptr, N);
            return dest + N;
        }
        uint8_t* pack_param_impl(uint8_t* dest, void* ptr)
        {
            return pack_param_impl(dest, (const void*)ptr);
        }
        uint8_t* pack_param_impl(uint8_t* dest, std::nullptr_t)
        {
            return pack_param_impl(dest, (void*)nullptr);
        }
        uint8_t* pack_param_impl(uint8_t* dest, int8_t val)
        {
            constexpr size_t N = sizeof(int8_t);
            *dest++ = (uint8_t)(data_type::i8);
            memcpy(dest, &val, N);
            return dest + N;
        }
        uint8_t* pack_param_impl(uint8_t* dest, uint8_t val)
        {
            constexpr size_t N = sizeof(uint8_t);
            *dest++ = (uint8_t)(data_type::u8);
            memcpy(dest, &val, N);
            return dest + N;
        }
        uint8_t* pack_param_impl(uint8_t* dest, int16_t val)
        {
           constexpr size_t N = sizeof(int16_t);
            *dest++ = (uint8_t)(data_type::i16);
            memcpy(dest, &val, N);
            return dest + N;
        }
        uint8_t* pack_param_impl(uint8_t* dest, uint16_t val)
        {
            constexpr size_t N = sizeof(uint16_t);
            *dest++ = (uint8_t)(data_type::u16);
            memcpy(dest, &val, N);
            return dest + N;
        }
        uint8_t* pack_param_impl(uint8_t* dest, int32_t val)
        {
            constexpr size_t N = sizeof(int32_t);
            *dest++ = (uint8_t)(data_type::i32);
            memcpy(dest, &val, N);
            return dest + N;
        }
        uint8_t* pack_param_impl(uint8_t* dest, uint32_t val)
        {
            constexpr size_t N = sizeof(uint32_t);
            *dest++ = (uint8_t)(data_type::u32);
            memcpy(dest, &val, N);
            return dest + N;
        }
        uint8_t* pack_param_impl(uint8_t* dest, int64_t val)
        {
            constexpr size_t N = sizeof(int64_t);
            *dest++ = (uint8_t)(data_type::i64);
            memcpy(dest, &val, N);
            return dest + N;
        }
        uint8_t* pack_param_impl(uint8_t* dest, uint64_t val)
        {
            constexpr size_t N = sizeof(uint64_t);
            *dest++ = (uint8_t)(data_type::u64);
            memcpy(dest, &val, N);
            return dest + N;
        }
        uint8_t* pack_param_impl(uint8_t* dest, float val)
        {
            constexpr size_t N = sizeof(float);
            *dest++ = (uint8_t)(data_type::f32);
            memcpy(dest, &val, N);
            return dest + N;
        }
        uint8_t* pack_param_impl(uint8_t* dest, double val)
        {
            constexpr size_t N = sizeof(double);
            *dest++ = (uint8_t)(data_type::f64);
            memcpy(dest, &val, N);
            return dest + N;
        }

        message* alloc_msg(size_t size)
        {
            return reinterpret_cast<message*>(new uint8_t[size]);
        }

        void free_msg(message* msg)
        {
            delete[] reinterpret_cast<uint8_t*>(msg);
        }
    }

    /// Logger Data
    namespace logger
    {
        typedef std::vector<serialization::message*>* message_queue;
#ifdef _WIN32
        static CRITICAL_SECTION queue_lock;
#else
        static pthread_mutex_t queue_lock;
#endif // _WIN32

        static volatile message_queue producer = nullptr;
        static volatile message_queue consumer = nullptr;

        void init_queue()
        {
#ifdef _WIN32
            InitializeCriticalSection(&queue_lock);
#else
            pthread_mutex_init(&queue_lock, nullptr);
#endif // _WIN32
            producer = new std::vector<serialization::message*>();
            consumer = new std::vector<serialization::message*>();
        }

        void free_queue()
        {
#ifdef _WIN32
            DeleteCriticalSection(&queue_lock);
#else
            pthread_mutex_destroy(&queue_lock);
#endif // _WIN2
            delete producer; producer = nullptr;
            delete consumer; consumer = nullptr;
        }

        bool push_message(message_queue q, serialization::message* msg)
        {
            assert(q != nullptr);
            q->push_back(msg);
            return true;
        }

        bool try_push_message(serialization::message* msg)
        {
#ifdef _WIN32
            EnterCriticalSection(&queue_lock);
#else
            pthread_mutex_lock(&queue_lock);
#endif // _WIN32

            producer->push_back(msg);

#ifdef _WIN32
            LeaveCriticalSection(&queue_lock);
#else
            pthread_mutex_unlock(&queue_lock);
#endif // _WIN32

            return true;
        }

        // size_t visit_msgs(message_queue q, auto func)
        size_t visit_msgs(message_queue q, std::function<void(serialization::message* msg)> func)
        {
            for(auto* msg : *q) {
                func(msg);
            }
            size_t retval = q->size();
            q->clear();
            return retval;
        }

        message_queue swap_queues()
        {
#ifdef _WIN32
            EnterCriticalSection(&queue_lock);
            std::swap(producer, consumer);
            LeaveCriticalSection(&queue_lock);
#else
            pthread_mutex_lock(&queue_lock);
            std::swap(producer, consumer);
            pthread_mutex_unlock(&queue_lock);
#endif // _WIN32
            return consumer;
        }

        static std::atomic_size_t remaining_messages;
        static std::atomic_bool thread_started;
        static std::atomic_bool signal_exit;
#ifdef _WIN32
        static HANDLE logger_thread;
#else
        static pthread_t logger_thread;
#endif  // _WIN32

#ifdef _WIN32
        DWORD WINAPI logger_func(void*);
#else
        void* logger_func(void*);
#endif  // _WIN32

        bool logger_init()
        {
            // init queue
            remaining_messages.store(0);
            init_queue();

            // init logger thread
            thread_started.store(false);
            signal_exit.store(false);
#ifdef _WIN32
            logger_thread = CreateThread(nullptr, 0, &logger_func, nullptr, 0, nullptr);
#else
            pthread_create(&logger_thread, nullptr, &logger_func, nullptr);
#endif // _WIN32
            // cleanup method
            atexit([]()
            {
                while(!thread_started) {
                    internal::thread_yield();
                }
                signal_exit.store(true);

                // wait for logger to finish
#ifdef _WIN32
                WaitForSingleObject(logger_thread, INFINITE);
#else
                pthread_join(logger_thread, nullptr);
#endif // _WIN32
                free_queue();
            });

            return true;
        }

#ifdef _WIN32
        DWORD WINAPI logger_func(LPVOID)
#else
        void* logger_func(void*)
#endif // _WIN32
        {
            thread_started.store(true);

            // name thread for debugging
#ifdef _WIN32

#else
            prctl(PR_SET_NAME,"logger_func",0,0,0);
#endif // _WIN32

            // init logging file
            int32_t childID = internal::get_child_id();
            FILE* log_file = internal::get_log_file(childID);

            size_t total_messages_written = 0;
            // spin until exit is signalled (unless there are remaining messages to write)
            while(!signal_exit || remaining_messages != 0)
            {
                // repeatedly spin until message queues are empty
                while (remaining_messages != 0)
                {
                    auto queue = swap_queues();

                    size_t messages_written = visit_msgs(queue,
                        [&](serialization::message* msg) -> void
                        {
                            msg->process_id = childID;
                            fwrite(msg, sizeof(uint8_t), msg->length, log_file);
                            free_msg(msg);
                        });

                    remaining_messages -= messages_written;
                    total_messages_written += messages_written;
                }
                fflush(log_file);
            }

            // flush to disk
            fflush(log_file);
            fclose(log_file);

            return 0;
        }

        void enqueue_msg(serialization::message* msg, uint64_t timestamp)
        {
            static bool logger_inited = logger_init();
            (void)logger_inited;

            msg->timestamp = timestamp;
            msg->thread_id = internal::get_thread_id();

            ++remaining_messages;
            while(!try_push_message(msg)) {
                internal::thread_yield();
            }
        }
    }
}