#pragma once

#include "Core.h"
#include "Memory.h"

extern "C" {

    typedef char* String;

    struct string_header
    {
        u32 length;
        u32 capacity;
    };

    #define StringHeader(str) (Cast(string_header *)(str) - 1)

    #define DEF extern "C"

    DEF String string_make_reserve(Allocator a, u32 capacity);
    DEF String string_make_length(Allocator a, void const* str, u32 num_bytes);
    DEF String string_sprintf(Allocator a, char* buf, u32 num_bytes, const char* fmt, ...);
    DEF String string_sprintf_buf(Allocator a, const char* fmt, ...); // NOTE: Uses locally persistent buffer
    DEF String string_append_length(Allocator a, String str, void const* other, u32 num_bytes);
    DEF String string_appendc(Allocator a, String str, const char* other);
    DEF String string_join(Allocator a, const char** parts, u32 count, const char* glue);
    DEF String string_set(Allocator a, String str, const char* cstr);
    DEF String string_make_space_for(Allocator a, String str, u32 add_len);
    DEF u32    string_allocation_size(String const str);
    DEF bool32 string_are_equal(String const lhs, String const rhs);
    DEF String string_trim(String str, const char* cut_set);
    DEF String string_append_rune(Allocator a, String str, zpl_rune r);
    DEF String string_append_fmt(Allocator a, String str, const char* fmt, ...);

    DEF String string_make(Allocator a, const char* str);
    DEF void   string_free(Allocator a, String str);
    DEF void   string_clear(String str);
    DEF String string_duplicate(Allocator a, String const str);
    DEF u32    string_length(String const str);
    DEF u32    string_capacity(String const str);
    DEF u32    string_available_space(String const str);
    DEF String string_append(Allocator a, String str, String const other);
    DEF String string_trim_space(String str); // Whitespace ` \t\r\n\v\f`

    inline void string_set_length(String str, u32 len) { StringHeader(str)->length = len; }
    inline void string_set_capacity(String str, u32 cap) { StringHeader(str)->capacity = cap; }

    inline String string_make(Allocator a, const char* str)
    {
        size_t len = str ? strlen(str) : 0;
        SAssert(len < UINT32_MAX);
        return string_make_length(a, str, (u32)len);
    }

    inline void string_free(Allocator a, String str)
    {
        if (str)
        {
            string_header* header = StringHeader(str);
            SFree(a, header);
        }
    }

    inline String string_duplicate(Allocator a, String const str)
    {
        return string_make_length(a, str, string_length(str));
    }

    inline u32 string_length(String const str) { return StringHeader(str)->length; }
    inline u32 string_capacity(String const str) { return StringHeader(str)->capacity; }

    inline u32 string_available_space(String const str)
    {
        string_header* h = StringHeader(str);
        if (h->capacity > h->length) return h->capacity - h->length;
        return 0;
    }

    inline void string_clear(String str)
    {
        string_set_length(str, 0);
        str[0] = '\0';
    }

    inline String string_append(Allocator a, String str, String const other)
    {
        return string_append_length(a, str, other, string_length(other));
    }

    inline String string_trim_space(String str) { return string_trim(str, " \t\r\n\v\f"); }

    inline String string_make_reserve(Allocator a, u32 capacity) {
        u32 header_size = sizeof(string_header);
        PushMemoryIgnoreFree();
        void* ptr = SMalloc(a, header_size + capacity + 1);
        PopMemoryIgnoreFree();

        String str;
        string_header* header;

        if (ptr == NULL) return NULL;
        SZero(ptr, header_size + capacity + 1);

        str = Cast(char*)(ptr) + header_size;
        header = StringHeader(str);
        header->length = 0;
        header->capacity = capacity;
        str[capacity] = '\0';

        return str;
    }


    inline String string_make_length(Allocator a, void const* init_str, u32 num_bytes) {
        u32 header_size = sizeof(string_header);
        PushMemoryIgnoreFree();
        void* ptr = SMalloc(a, header_size + num_bytes + 1);
        PopMemoryIgnoreFree();

        String str;
        string_header* header;

        if (ptr == NULL) return NULL;
        if (!init_str) SZero(ptr, header_size + num_bytes + 1);

        str = Cast(char*)(ptr) + header_size;
        header = StringHeader(str);
        header->length = num_bytes;
        header->capacity = num_bytes;
        if (num_bytes && init_str) SCopy(str, init_str, num_bytes);
        str[num_bytes] = '\0';

        return str;
    }

    inline String string_sprintf_buf(Allocator a, const char* fmt, ...) {
        local_static thread_local char buf[ZPL_PRINTF_MAXLEN] = { 0 };
        va_list va;
        va_start(va, fmt);
        zpl_snprintf_va(buf, ZPL_PRINTF_MAXLEN, fmt, va);
        va_end(va);

        return string_make(a, buf);
    }

    inline String string_sprintf(Allocator a, char* buf, u32 num_bytes, const char* fmt, ...) {
        va_list va;
        va_start(va, fmt);
        zpl_snprintf_va(buf, num_bytes, fmt, va);
        va_end(va);

        return string_make(a, buf);
    }

    inline String string_append_length(Allocator a, String str, void const* other, u32 other_len) {
        if (other_len > 0) {
            u32 curr_len = string_length(str);

            str = string_make_space_for(a, str, other_len);
            if (str == NULL) return NULL;

            SCopy(str + curr_len, other, other_len);
            str[curr_len + other_len] = '\0';
            string_set_length(str, curr_len + other_len);
        }
        return str;
    }

    _FORCE_INLINE_ String string_appendc(Allocator a, String str, const char* other) {
        return string_append_length(a, str, other, (u32)strlen(other));
    }

    _FORCE_INLINE_ String string_join(Allocator a, const char** parts, u32 count, const char* glue) {
        String ret;
        u32 i;

        ret = string_make(a, NULL);

        for (i = 0; i < count; ++i) {
            ret = string_appendc(a, ret, parts[i]);

            if ((i + 1) < count) {
                ret = string_appendc(a, ret, glue);
            }
        }

        return ret;
    }

    inline String string_set(Allocator a, String str, const char* cstr) {
        size_t len = strlen(cstr);
        SAssert(len < UINT32_MAX);
        if (string_capacity(str) < (u32)len) {
            str = string_make_space_for(a, str, (u32)len - string_length(str));
            if (str == NULL) return NULL;
        }

        SCopy(str, cstr, (u32)len);
        str[len] = '\0';
        string_set_length(str, (u32)len);

        return str;
    }

    inline String string_make_space_for(Allocator a, String str, u32 add_len) {
        u32 available = string_available_space(str);

        // NOTE: Return if there is enough space left
        if (available >= add_len)
        {
            return str;
        }
        else
        {
            u32 new_len, old_size, new_size;
            void* ptr, * new_ptr;
            string_header* header;

            new_len = string_length(str) + add_len;
            ptr = StringHeader(str);
            old_size = sizeof(string_header) + string_length(str) + 1;
            new_size = sizeof(string_header) + new_len + 1;
            
            PushMemoryIgnoreFree();
            new_ptr = SRealloc(a, ptr, old_size, new_size);
            PopMemoryIgnoreFree();
            if (new_ptr == NULL) return NULL;

            header = Cast(string_header*) new_ptr;

            str = Cast(String)(header + 1);
            string_set_capacity(str, new_len);

            return str;
        }
    }

    inline u32 string_allocation_size(String const str) {
        u32 cap = string_capacity(str);
        return sizeof(string_header) + cap;
    }

    inline bool32 string_are_equal(String const lhs, String const rhs) {
        u32 lhs_len, rhs_len, i;
        lhs_len = string_length(lhs);
        rhs_len = string_length(rhs);
        if (lhs_len != rhs_len) return false;

        for (i = 0; i < lhs_len; i++) {
            if (lhs[i] != rhs[i]) return false;
        }

        return true;
    }

    inline String string_trim(String str, const char* cut_set) {
        char* start, * end, * start_pos, * end_pos;
        u32 len;

        start_pos = start = str;
        end_pos = end = str + string_length(str) - 1;

        while (start_pos <= end && zpl_char_first_occurence(cut_set, *start_pos)) start_pos++;
        while (end_pos > start_pos && zpl_char_first_occurence(cut_set, *end_pos)) end_pos--;

        len = Cast(u32)((start_pos > end_pos) ? 0 : ((end_pos - start_pos) + 1));

        if (str != start_pos) SMemMove(str, start_pos, len);
        str[len] = '\0';

        string_set_length(str, len);

        return str;
    }

    inline String string_append_rune(Allocator a, String str, zpl_rune r) {
        if (r >= 0) {
            zpl_u8 buf[8] = { 0 };
            u32 len = (u32)zpl_utf8_encode_rune(buf, r);
            return string_append_length(a, str, buf, len);
        }

        return str;
    }

    inline String string_append_fmt(Allocator a, String str, const char* fmt, ...) {
        u32 res;
        char buf[ZPL_PRINTF_MAXLEN] = { 0 };
        va_list va;
        va_start(va, fmt);
        res = (u32)zpl_snprintf_va(buf, zpl_count_of(buf) - 1, fmt, va) - 1;
        va_end(va);
        return string_append_length(a, str, buf, res);
    }

}