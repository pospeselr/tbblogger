#ifndef TBB_LOGGER_H
#define TBB_LOGGER_H

// C
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

// C++
#include <atomic>
#include <memory>
#include <vector>
#include <functional>
#include <type_traits>
#include <utility>
#include <string>

#ifdef _WIN32
#   include <windows.h>
#else
#   include <unistd.h>
#   include <sys/prctl.h>
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <sys/syscall.h>
#endif

#define TBB_LOG_IMPL(...) tbb::logger::log(__FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)

#if 0
#define TBB_LOG(...) (void)(0)
#else
#define TBB_LOG(...) TBB_LOG_IMPL(__VA_ARGS__)
#endif
#define TBB_TRACE(...) TBB_LOG("")

namespace tbb
{
    namespace serialization
    {
        enum class data_type : uint8_t
        {
            invalid = 0xFF,
            // strings
            utf8 = 0,
            utf16,
            utf32,
            // pointers
            p32,
            p64,
            // integers
            i8,
            u8,
            i16,
            u16,
            i32,
            u32,
            i64,
            u64,
            // floating points
            f32,
            f64,
        };

        // used to determine size of param in bytes
        constexpr size_t param_size()                        { return 0; }

        // string size of unknown size implementation
        size_t param_size_impl(const char* str);
        size_t param_size_impl(const char16_t* str);
        size_t param_size_impl(const char32_t* str);
        size_t param_size_impl(const wchar_t* str);
        // arbitatry pointer size impemetation
        constexpr size_t param_size_impl(const void*)        { return sizeof(void*)        + sizeof(data_type); }
        // fixed string literal size
        template<size_t N>
        constexpr size_t param_size(const char (&str)[N])    { return sizeof(char)     * N + sizeof(data_type); }
        template<size_t N>
        constexpr size_t param_size(const char16_t (&str)[N]){ return sizeof(char16_t) * N + sizeof(data_type); }
        template<size_t N>
        constexpr size_t param_size(const char32_t (&str)[N]){ return sizeof(char32_t) * N + sizeof(data_type); }
        template<size_t N>
        constexpr size_t param_size(const wchar_t (&str)[N]) { return sizeof(wchar_t)  * N + sizeof(data_type); }
        // read/write string buffer size
        size_t param_size(char str[]);
        size_t param_size(char16_t str[]);
        size_t param_size(char32_t str[]);
        size_t param_size(wchar_t str[]);
        // catch-all template functionf or pointers
        template<typename T>
        typename std::enable_if<
            std::is_pointer<T>::value, size_t>::type
        param_size(T ptr)                                    { return param_size_impl(ptr); }
        constexpr size_t param_size(std::nullptr_t)          { return sizeof(void*)       + sizeof(data_type); }
        constexpr size_t param_size(int8_t)                  { return sizeof(int8_t)      + sizeof(data_type); }
        constexpr size_t param_size(uint8_t)                 { return sizeof(uint8_t)     + sizeof(data_type); }
        constexpr size_t param_size(int16_t)                 { return sizeof(int16_t)     + sizeof(data_type); }
        constexpr size_t param_size(uint16_t)                { return sizeof(uint16_t)    + sizeof(data_type); }
        constexpr size_t param_size(int32_t)                 { return sizeof(int32_t)     + sizeof(data_type); }
        constexpr size_t param_size(uint32_t)                { return sizeof(uint32_t)    + sizeof(data_type); }
        constexpr size_t param_size(int64_t)                 { return sizeof(int64_t)     + sizeof(data_type); }
        constexpr size_t param_size(uint64_t)                { return sizeof(uint64_t)    + sizeof(data_type); }
        constexpr size_t param_size(float)                   { return sizeof(float)       + sizeof(data_type); }
        constexpr size_t param_size(double)                  { return sizeof(double)      + sizeof(data_type); }

        template<typename FIRST, typename ...ARGS>
        size_t param_size(FIRST&& first, ARGS&&... args)
        {
            return param_size(std::forward<FIRST>(first)) + param_size(std::forward<ARGS>(args)...);
        }

        uint8_t* pack_param_impl(uint8_t* dest, const char* str);
        uint8_t* pack_param_impl(uint8_t* dest, const char16_t* str);
        uint8_t* pack_param_impl(uint8_t* dest, const char32_t* str);
        uint8_t* pack_param_impl(uint8_t* dest, const wchar_t* str);
        uint8_t* pack_param_impl(uint8_t* dest, char str[]);
        uint8_t* pack_param_impl(uint8_t* dest, wchar_t str[]);
        uint8_t* pack_param_impl(uint8_t* dest, char16_t str[]);
        uint8_t* pack_param_impl(uint8_t* dest, char32_t str[]);
        uint8_t* pack_param_impl(uint8_t* dest, const void* ptr);
        uint8_t* pack_param_impl(uint8_t* dest, void* ptr);
        uint8_t* pack_param_impl(uint8_t* dest, std::nullptr_t);
        uint8_t* pack_param_impl(uint8_t* dest, int8_t val);
        uint8_t* pack_param_impl(uint8_t* dest, uint8_t val);
        uint8_t* pack_param_impl(uint8_t* dest, int16_t val);
        uint8_t* pack_param_impl(uint8_t* dest, uint16_t val);
        uint8_t* pack_param_impl(uint8_t* dest, int32_t val);
        uint8_t* pack_param_impl(uint8_t* dest, uint32_t val);
        uint8_t* pack_param_impl(uint8_t* dest, int64_t val);
        uint8_t* pack_param_impl(uint8_t* dest, uint64_t val);
        uint8_t* pack_param_impl(uint8_t* dest, float val);
        uint8_t* pack_param_impl(uint8_t* dest, double val);

        inline uint8_t* pack_param(uint8_t* dest) {return dest;}
        template<typename FIRST, typename ...ARGS>
        uint8_t* pack_param(uint8_t* dest, FIRST&& first, ARGS&&... args)
        {
            return pack_param(pack_param_impl(dest, std::forward<FIRST>(first)), std::forward<ARGS>(args)...);
        }

        #pragma pack(1)
        struct message
        {
            uint32_t length;
            uint32_t process_id;
            uint32_t thread_id;
            uint64_t timestamp;
        };
        #pragma pack()

        message* alloc_msg(size_t size);
        void free_msg(message* msg);

        template<typename ...ARGS>
        size_t msg_size(ARGS&&... args)
        {
            return sizeof(message) + serialization::param_size(std::forward<ARGS>(args)...);
        }

        template<typename ...ARGS>
        void write_msg(message* msg, ARGS&&... args)
        {
            uint8_t* head = reinterpret_cast<uint8_t*>(msg);
            uint8_t* tail = head + sizeof(message);
            tail = serialization::pack_param(tail, std::forward<ARGS>(args)...);
            size_t len = (size_t)(tail - head);
            *reinterpret_cast<uint32_t*>(head) = static_cast<uint32_t>(len);
        }

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
        inline size_t param_size_impl(const char* str)
        {
            if (!str) {
                str = UTF8_NULLSTRING;
            }
            return param_size_string(str);
        }
        inline size_t param_size_impl(const char16_t* str)
        {
            if (!str) {
                str = UTF16_NULLSTRING;
            }
            return param_size_string(str);
        }
        inline size_t param_size_impl(const char32_t* str)
        {
            if (!str) {
                str = UTF32_NULLSTRING;
            }
            return param_size_string(str);
        }
        inline size_t param_size_impl(const wchar_t* str)
        {
            if (!str) {
                str = WIDE_NULLSTRING;
            }
            return param_size_string(str);
        }

        inline size_t param_size(char str[])
        {
            return param_size_impl(str);
        }
        inline size_t param_size(char16_t str[])
        {
            return param_size_impl(str);
        }
        inline size_t param_size(char32_t str[])
        {
            return param_size_impl(str);
        }
        inline size_t param_size(wchar_t str[])
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
        inline uint8_t* pack_param_impl(uint8_t* dest, const char* str)
        {
            if (!str) {
                str = UTF8_NULLSTRING;
            }
            return pack_param_string(dest, str);
        }
        inline uint8_t* pack_param_impl(uint8_t* dest, const char16_t* str)
        {
            if (!str) {
                str = UTF16_NULLSTRING;
            }
            return pack_param_string(dest, str);
        }
        inline uint8_t* pack_param_impl(uint8_t* dest, const char32_t* str)
        {
            if (!str) {
                str = UTF32_NULLSTRING;
            }
            return pack_param_string(dest, str);
        }
        inline uint8_t* pack_param_impl(uint8_t* dest, const wchar_t* str)
        {
            if (!str) {
                str = WIDE_NULLSTRING;
            }
            return pack_param_string(dest, str);
        }
        inline uint8_t* pack_param_impl(uint8_t* dest, char str[])
        {
            return pack_param_impl(dest, (const char*)str);
        }
        inline uint8_t* pack_param_impl(uint8_t* dest, char16_t str[])
        {
            return pack_param_impl(dest, (const char16_t*)str);
        }
        inline uint8_t* pack_param_impl(uint8_t* dest, char32_t str[])
        {
            return pack_param_impl(dest, (const char32_t*)str);
        }
        inline uint8_t* pack_param_impl(uint8_t* dest, wchar_t str[])
        {
            return pack_param_impl(dest, (const wchar_t*)str);
        }

        inline uint8_t* pack_param_impl(uint8_t* dest, const void* ptr)
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

        inline uint8_t* pack_param_impl(uint8_t* dest, void* ptr)
        {
            return pack_param_impl(dest, (const void*)ptr);
        }

        inline uint8_t* pack_param_impl(uint8_t* dest, std::nullptr_t)
        {
            return pack_param_impl(dest, (void*)nullptr);
        }

        inline uint8_t* pack_param_impl(uint8_t* dest, int8_t val)
        {
            constexpr size_t N = sizeof(int8_t);
            *dest++ = (uint8_t)(data_type::i8);
            memcpy(dest, &val, N);
            return dest + N;
        }

        inline uint8_t* pack_param_impl(uint8_t* dest, uint8_t val)
        {
            constexpr size_t N = sizeof(uint8_t);
            *dest++ = (uint8_t)(data_type::u8);
            memcpy(dest, &val, N);
            return dest + N;
        }

        inline uint8_t* pack_param_impl(uint8_t* dest, int16_t val)
        {
           constexpr size_t N = sizeof(int16_t);
            *dest++ = (uint8_t)(data_type::i16);
            memcpy(dest, &val, N);
            return dest + N;
        }

        inline uint8_t* pack_param_impl(uint8_t* dest, uint16_t val)
        {
            constexpr size_t N = sizeof(uint16_t);
            *dest++ = (uint8_t)(data_type::u16);
            memcpy(dest, &val, N);
            return dest + N;
        }

        inline uint8_t* pack_param_impl(uint8_t* dest, int32_t val)
        {
            constexpr size_t N = sizeof(int32_t);
            *dest++ = (uint8_t)(data_type::i32);
            memcpy(dest, &val, N);
            return dest + N;
        }

        inline uint8_t* pack_param_impl(uint8_t* dest, uint32_t val)
        {
            constexpr size_t N = sizeof(uint32_t);
            *dest++ = (uint8_t)(data_type::u32);
            memcpy(dest, &val, N);
            return dest + N;
        }

        inline uint8_t* pack_param_impl(uint8_t* dest, int64_t val)
        {
            constexpr size_t N = sizeof(int64_t);
            *dest++ = (uint8_t)(data_type::i64);
            memcpy(dest, &val, N);
            return dest + N;
        }

        inline uint8_t* pack_param_impl(uint8_t* dest, uint64_t val)
        {
            constexpr size_t N = sizeof(uint64_t);
            *dest++ = (uint8_t)(data_type::u64);
            memcpy(dest, &val, N);
            return dest + N;
        }

        inline uint8_t* pack_param_impl(uint8_t* dest, float val)
        {
            constexpr size_t N = sizeof(float);
            *dest++ = (uint8_t)(data_type::f32);
            memcpy(dest, &val, N);
            return dest + N;
        }

        inline uint8_t* pack_param_impl(uint8_t* dest, double val)
        {
            constexpr size_t N = sizeof(double);
            *dest++ = (uint8_t)(data_type::f64);
            memcpy(dest, &val, N);
            return dest + N;
        }

        inline message* alloc_msg(size_t size)
        {
            return reinterpret_cast<message*>(new uint8_t[size]);
        }

        inline void free_msg(message* msg)
        {
            delete[] reinterpret_cast<uint8_t*>(msg);
        }
    }

    // Utilities
    namespace internal
    {
        // timestamp in nanoseconds
        inline uint64_t get_timestamp()
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

        inline uint32_t get_thread_id()
        {

#ifdef _WIN32
            static thread_local uint32_t thread_id = GetCurrentThreadId();
#else
            static thread_local uint32_t thread_id = syscall(SYS_gettid);
#endif
            return thread_id;
        }

        inline void thread_yield()
        {
#ifdef _WIN32
            SwitchToThread();
#else
            pthread_yield();
#endif
        }

        inline void thread_sleep(size_t milliseconds)
        {
#ifdef _WIN32
            Sleep(milliseconds);
#else
            const auto microseconds = milliseconds * 1000;
            usleep(microseconds);
#endif
        }

        inline void set_thread_name()
        {
#ifdef _WIN32
            // only works on Win10  and up :(
            typedef HRESULT (WINAPI *SetThreadDescription_t)(HANDLE,PCWSTR);
            SetThreadDescription_t SetThreadDescription =
                (SetThreadDescription_t)GetProcAddress(
                    GetModuleHandle(TEXT("kernel32.dll")),
                    "SetThreadDescription");

            if(SetThreadDescription != nullptr)
            {
                SetThreadDescription(GetCurrentThread(), L"tbblogger_thread");
            }
#else
            prctl(PR_SET_NAME,"tbblogger_thread",0,0,0);
#endif // _WIN32
        }

        inline const char* get_command_line()
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

        inline size_t get_temp_path(char* buffer, size_t len)
        {
#ifdef _WIN32
            return GetTempPathA(len, buffer);
#else
            const char tmp[] = "/tmp";
            memcpy(buffer, tmp, sizeof(tmp));
            return sizeof(tmp) - 1;
#endif
        }

        inline int32_t get_child_id()
        {
            const char* command_line = get_command_line();
            if (strstr(command_line, "firefox"))
            {
                if (auto childIDStart = strstr(command_line, "-childID"); childIDStart != nullptr)
                {
                    int32_t retval = -1;
                    sscanf(childIDStart, "-childID %d", &retval);
                    return retval;
                }
                return 0;
            }
            else
            {
                auto hash = std::hash<std::string>{}(command_line);
                int32_t retval = (int32_t)hash;
                if (retval > 0) retval *= -1;
                return retval;
            }
        }

#ifdef _WIN32
        typedef HANDLE file_t;
#else
        typedef FILE* file_t;
#endif

        inline file_t open_file(const char* filename)
        {
#ifdef _WIN32
            return CreateFileA(filename,
                               GENERIC_WRITE,
                               FILE_SHARE_READ,
                               nullptr,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               nullptr);
#else
            return fopen(filename, "wb");
#endif
        }

        inline file_t get_log_file(int32_t childID)
        {
            char filename[1024];
            char* head = filename;
            head += get_temp_path(head, sizeof(filename));
#if _WIN32
            head += sprintf(head, "firefox");
            CreateDirectoryA(filename, nullptr);
            if (childID >= 0)
            {
                sprintf(head, "\\firefox%i.bin", childID);
            }
            else
            {
                sprintf(head, "\\other%i.bin", -childID);
            }

#else
            head += sprintf(head, "/firefox");
            mkdir(filename, 0777);
            if (childID >=0)
            {
                sprintf(head, "/firefox%i.bin", childID);
            }
            else
            {
                sprintf(head, "/other%i.bin", -childID);
            }

#endif
            return open_file(filename);
        }

        inline void write_file(void* buf, size_t bytes, file_t file)
        {
#ifdef _WIN32
            WriteFile(file, buf, bytes, nullptr, nullptr);
#else
            fwrite(buf, 1, bytes, file);
#endif
        }

        inline void flush_file(file_t file)
        {
#ifdef _WIN32
            FlushFileBuffers(file);
#else
            fflush(file);
#endif
        }

        inline void close_file(file_t file)
        {
#ifdef _WIN32
            CloseHandle(file);
#else
            fclose(file);
#endif
        }
    }



    class mutex
    {
    public:
        mutex()
        {
#ifdef _WIN32
            InitializeCriticalSection(&cs);
#else
            pthread_mutex_init(&mut, nullptr);
#endif
        }

        ~mutex()
        {
#ifdef _WIN32
            DeleteCriticalSection(&cs);
#else
            pthread_mutex_destroy(&mut);
#endif
        }

        void lock()
        {
#ifdef _WIN32
            EnterCriticalSection(&cs);
#else
            pthread_mutex_lock(&mut);
#endif
        }

        void unlock()
        {
#ifdef _WIN32
            LeaveCriticalSection(&cs);
#else
            pthread_mutex_unlock(&mut);
#endif
        }
    private:
#ifdef _WIN32
        CRITICAL_SECTION cs;
#else
        pthread_mutex_t mut;
#endif
    };

    class logger
    {
        typedef std::vector<serialization::message*> message_queue_t;
    public:

        template<size_t N, typename... ARGS>
        static void __attribute__((noinline)) debug_log(const char* func, const char* file, uint32_t line, const char(&fmt)[N], ARGS&&... args)
        {
            const uint64_t timestamp = internal::get_timestamp();

            auto& self = logger::get();

            size_t size = serialization::msg_size(func, file, line, fmt, std::forward<ARGS>(args)...);
            auto* msg = serialization::alloc_msg(size);
            serialization::write_msg(msg, func, file, line, fmt, std::forward<ARGS>(args)...);

            // write info about logger
            self.enqueue_msg(msg, timestamp);
        }

        template<size_t N, size_t M, size_t O, typename... ARGS>
        static void __attribute__((noinline)) log(const char (&func)[N], const char (&file)[M], uint32_t line, const char (&fmt)[O], ARGS&&... args)
        {
            const uint64_t timestamp = internal::get_timestamp();

            auto& self = logger::get();

            size_t size = serialization::msg_size(func, file, line, fmt, std::forward<ARGS>(args)...);
            auto* msg = serialization::alloc_msg(size);
            serialization::write_msg(msg, func, file, line, fmt, std::forward<ARGS>(args)...);

            // write info about logger
            self.enqueue_msg(msg, timestamp);
        }

    private:

        void enqueue_msg(serialization::message* msg, uint64_t timestamp)
        {
            msg->timestamp= timestamp;
            msg->thread_id = internal::get_thread_id();

            ++remaining_messages;
            queue_lock.lock();
            producer->push_back(msg);
            queue_lock.unlock();
        }

        message_queue_t* swap_queues()
        {
            queue_lock.lock();
            std::swap(producer, consumer);
            queue_lock.unlock();

            return consumer.get();
        }

        ~logger()
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
        }

        /// Constructor
        logger()
        {
            producer.reset(new message_queue_t());
            consumer.reset(new message_queue_t());

            remaining_messages.store(0);
            thread_started.store(false);
            signal_exit.store(false);

#ifdef _WIN32
            logger_thread = CreateThread(nullptr, 0, &logger::logger_func, nullptr, 0, nullptr);
#else
            pthread_create(&this->logger_thread, nullptr, &logger::logger_func, nullptr);
#endif
        }

        // Getter
        static logger& get()
        {
            static logger retval;
            return retval;
        }

        // Thread Func
#ifdef _WIN32
        static DWORD WINAPI logger_func(LPVOID)
#else
        static void* logger_func(void*)
#endif
        {
            auto& self = logger::get();
            self.thread_started.store(true);

            // name thread for debugging
            internal::set_thread_name();
            // init logging file
            const int32_t childID = internal::get_child_id();
            auto log_file = internal::get_log_file(childID);
            size_t total_messages_written = 0;
            // spin until exit is signalled (unless there are remaining messages to write)
            while(!self.signal_exit || self.remaining_messages != 0)
            {
                // repeatedly spin until message queues are empty
                while (self.remaining_messages != 0)
                {
                    auto queue = self.swap_queues();

                    // write out messages to disk and free their memory
                    for(auto* msg : *queue) {
                        msg->process_id = childID;
                        internal::write_file(msg, msg->length, log_file);
                        free_msg(msg);
                    }

                    const auto messages_written = queue->size();
                    queue->clear();

                    self.remaining_messages -= messages_written;
                    total_messages_written += messages_written;
                }
                internal::flush_file(log_file);

                // sleep for a bit rather than spinning
                while (!self.signal_exit && self.remaining_messages == 0) {
                    internal::thread_sleep(20);
                }
            }

            // flush to disk
            internal::flush_file(log_file);
            internal::close_file(log_file);

            return 0;
        }

        mutex queue_lock;
        std::unique_ptr<message_queue_t> producer;
        std::unique_ptr<message_queue_t> consumer;
        std::atomic_size_t remaining_messages;
        std::atomic_bool thread_started;
        std::atomic_bool signal_exit;
#ifdef _WIN32
        HANDLE logger_thread;
#else
        pthread_t logger_thread;
#endif
    };
}

#endif // TBB_LOGGER_H
