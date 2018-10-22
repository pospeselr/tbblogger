#ifndef TBB_LOGGER_H
#define TBB_LOGGER_H

// C
#include <stdint.h>
#include <stddef.h>

// C++
#include <type_traits>
#include <utility>

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
            // LEGACY
            ascii_string = 1,
            wide_string,
            pointer,            
            // strings
            str_utf8,
            str_utf16,
            str_utf32,
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
    }

    namespace internal
    {
        uint64_t get_timestamp();
    }

    namespace logger
    {
        void enqueue_msg(serialization::message* msg, uint64_t timestamp);

        template<size_t N, size_t M, size_t O, typename... ARGS>
        void __attribute__((noinline)) log(const char (&func)[N], const char (&file)[M], uint32_t line, const char (&fmt)[O], ARGS&&... args)
        {
            const uint64_t timestamp = internal::get_timestamp();

            size_t size = serialization::msg_size(func, file, line, fmt, std::forward<ARGS>(args)...);
            auto* msg = serialization::alloc_msg(size);
            serialization::write_msg(msg, func, file, line, fmt, std::forward<ARGS>(args)...);

            // write info about logger
            logger::enqueue_msg(msg, timestamp);
        }
    }
}

#endif // TBB_LOGGER_H
