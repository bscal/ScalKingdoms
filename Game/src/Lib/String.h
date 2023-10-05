#pragma once

#include "Core.h"
#include "Memory.h"

extern "C" {

    typedef char* String;

    struct string_header
    {
        u32 Length;
        u32 Capacity;
    };

    #define StringHeader(str) ((string_header*)(str) - 1)

    inline String StringMakeReserve(SAllocator a, u32 capacity);
    inline String StringMakeLength(SAllocator a, void const* str, u32 num_bytes);
    inline String StringSprintf(SAllocator a, char* buf, u32 num_bytes, const char* fmt, ...);
    inline String StringSprintfBuffer(SAllocator a, const char* fmt, ...); // NOTE: Uses locally persistent buffer
    inline String StringAppendLength(SAllocator a, String str, void const* other, u32 num_bytes);
    inline String StringAppend(SAllocator a, String str, const char* other);
    inline String StringJoin(SAllocator a, const char** parts, u32 count, const char* glue);
    inline String StringSet(SAllocator a, String str, const char* cstr);
    inline String StringMakeSpaceFor(SAllocator a, String str, u32 add_len);
    inline u32    StringMemorySize(String const str);
    inline bool32 StringAreEqual(String const lhs, String const rhs);
    inline String StringTrim(String str, const char* cut_set);
    inline String StringAppendRune(SAllocator a, String str, zpl_rune r);
    inline String StringAppendFmt(SAllocator a, String str, const char* fmt, ...);

    inline String StringMake(SAllocator a, const char* str);
    inline void   StringFree(SAllocator a, String str);
    inline void   StringClear(String str);
    inline String StringClone(SAllocator a, String const str);
    inline u32    StringLength(String const str);
    inline u32    StringCapacity(String const str);
    inline u32    StringAvailableSpace(String const str);
    inline String StringAppendString(SAllocator a, String str, String const other);
    inline String StringTrimSpace(String str); // Whitespace ` \t\r\n\v\f`

    inline void StringSetLength(String str, u32 len) { StringHeader(str)->Length = len; }
    inline void StringSetCapacity(String str, u32 cap) { StringHeader(str)->Capacity = cap; }

    inline String StringMake(SAllocator a, const char* str)
    {
        size_t len = str ? strlen(str) : 0;
        SAssert(len < UINT32_MAX);
        return StringMakeLength(a, str, (u32)len);
    }

    inline void StringFree(SAllocator a, String str)
    {
        if (str)
        {
            string_header* header = StringHeader(str);
            SFree(a, header);
        }
    }

    inline String StringClone(SAllocator a, String const str)
    {
        return StringMakeLength(a, str, StringLength(str));
    }

    inline u32 StringLength(String const str) { return StringHeader(str)->Length; }
    inline u32 StringCapacity(String const str) { return StringHeader(str)->Capacity; }

    inline u32 StringAvailableSpace(String const str)
    {
        string_header* h = StringHeader(str);
        if (h->Capacity > h->Length) return h->Capacity - h->Length;
        return 0;
    }

    inline void StringClear(String str)
    {
        StringSetLength(str, 0);
        str[0] = '\0';
    }

    inline String StringAppendString(SAllocator a, String str, String const other)
    {
        return StringAppendLength(a, str, other, StringLength(other));
    }

    inline String StringTrimSpace(String str) { return StringTrim(str, " \t\r\n\v\f"); }

    inline String StringMakeReserve(SAllocator a, u32 capacity) {
        u32 header_size = sizeof(string_header);
        PushMemoryIgnoreFree();
        void* ptr = SAlloc(a, header_size + capacity + 1);
        PopMemoryIgnoreFree();

        String str;
        string_header* header;

        if (ptr == NULL) return NULL;
        SZero(ptr, header_size + capacity + 1);

        str = (char*)(ptr) + header_size;
        header = StringHeader(str);
        header->Length = 0;
        header->Capacity = capacity;
        str[capacity] = '\0';

        return str;
    }


    inline String StringMakeLength(SAllocator a, void const* init_str, u32 num_bytes) {
        u32 header_size = sizeof(string_header);
        PushMemoryIgnoreFree();
        void* ptr = SAlloc(a, header_size + num_bytes + 1);
        PopMemoryIgnoreFree();

        String str;
        string_header* header;

        if (ptr == NULL) return NULL;
        if (!init_str) SZero(ptr, header_size + num_bytes + 1);

        str = (char*)(ptr) + header_size;
        header = StringHeader(str);
        header->Length = num_bytes;
        header->Capacity = num_bytes;
        if (num_bytes && init_str) SCopy(str, init_str, num_bytes);
        str[num_bytes] = '\0';

        return str;
    }

    inline String StringSprintfBuffer(SAllocator a, const char* fmt, ...) {
        local_persist thread_local char buf[ZPL_PRINTF_MAXLEN] = { 0 };
        va_list va;
        va_start(va, fmt);
        zpl_snprintf_va(buf, ZPL_PRINTF_MAXLEN, fmt, va);
        va_end(va);

        return StringMake(a, buf);
    }

    inline String StringSprintf(SAllocator a, char* buf, u32 num_bytes, const char* fmt, ...) {
        va_list va;
        va_start(va, fmt);
        zpl_snprintf_va(buf, num_bytes, fmt, va);
        va_end(va);

        return StringMake(a, buf);
    }

    inline String StringAppendLength(SAllocator a, String str, void const* other, u32 other_len) {
        if (other_len > 0) {
            u32 curr_len = StringLength(str);

            str = StringMakeSpaceFor(a, str, other_len);
            if (str == NULL) return NULL;

            SCopy(str + curr_len, other, other_len);
            str[curr_len + other_len] = '\0';
            StringSetLength(str, curr_len + other_len);
        }
        return str;
    }

    _FORCE_INLINE_ String StringAppend(SAllocator a, String str, const char* other) {
        return StringAppendLength(a, str, other, (u32)strlen(other));
    }

    _FORCE_INLINE_ String StringJoin(SAllocator a, const char** parts, u32 count, const char* glue) {
        String ret;
        u32 i;

        ret = StringMake(a, NULL);

        for (i = 0; i < count; ++i) {
            ret = StringAppend(a, ret, parts[i]);

            if ((i + 1) < count) {
                ret = StringAppend(a, ret, glue);
            }
        }

        return ret;
    }

    inline String StringSet(SAllocator a, String str, const char* cstr) {
        size_t len = strlen(cstr);
        SAssert(len < UINT32_MAX);
        if (StringCapacity(str) < (u32)len) {
            str = StringMakeSpaceFor(a, str, (u32)len - StringLength(str));
            if (str == NULL) return NULL;
        }

        SCopy(str, cstr, (u32)len);
        str[len] = '\0';
        StringSetLength(str, (u32)len);

        return str;
    }

    inline String StringMakeSpaceFor(SAllocator a, String str, u32 add_len) {
        u32 available = StringAvailableSpace(str);

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

            new_len = StringLength(str) + add_len;
            ptr = StringHeader(str);
            old_size = sizeof(string_header) + StringLength(str) + 1;
            new_size = sizeof(string_header) + new_len + 1;
            
            PushMemoryIgnoreFree();
            new_ptr = SRealloc(a, ptr, old_size, new_size);
            PopMemoryIgnoreFree();
            if (new_ptr == NULL) return NULL;

            header = (string_header*)new_ptr;

            str = (String)(header + 1);
            StringSetCapacity(str, new_len);

            return str;
        }
    }

    inline u32 StringMemorySize(String const str) {
        u32 cap = StringCapacity(str);
        return sizeof(string_header) + cap;
    }

    inline bool32 StringAreEqual(String const lhs, String const rhs) {
        u32 lhs_len, rhs_len, i;
        lhs_len = StringLength(lhs);
        rhs_len = StringLength(rhs);
        if (lhs_len != rhs_len) return false;

        for (i = 0; i < lhs_len; i++) {
            if (lhs[i] != rhs[i]) return false;
        }

        return true;
    }

    inline String StringTrim(String str, const char* cut_set) {
        char* start, * end, * start_pos, * end_pos;
        u32 len;

        start_pos = start = str;
        end_pos = end = str + StringLength(str) - 1;

        while (start_pos <= end && zpl_char_first_occurence(cut_set, *start_pos)) start_pos++;
        while (end_pos > start_pos && zpl_char_first_occurence(cut_set, *end_pos)) end_pos--;

        len = (u32)((start_pos > end_pos) ? 0 : ((end_pos - start_pos) + 1));

        if (str != start_pos) SMemMove(str, start_pos, len);
        str[len] = '\0';

        StringSetLength(str, len);

        return str;
    }

    inline String StringAppendRune(SAllocator a, String str, zpl_rune r) {
        if (r >= 0) {
            zpl_u8 buf[8] = { 0 };
            u32 len = (u32)zpl_utf8_encode_rune(buf, r);
            return StringAppendLength(a, str, buf, len);
        }

        return str;
    }

    inline String StringAppendFmt(SAllocator a, String str, const char* fmt, ...) {
        u32 res;
        char buf[ZPL_PRINTF_MAXLEN] = { 0 };
        va_list va;
        va_start(va, fmt);
        res = (u32)zpl_snprintf_va(buf, zpl_count_of(buf) - 1, fmt, va) - 1;
        va_end(va);
        return StringAppendLength(a, str, buf, res);
    }

}