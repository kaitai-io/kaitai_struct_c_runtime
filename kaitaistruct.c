#define KS_DEPEND_ON_INTERNALS
#include "kaitaistruct.h"

#define VOID

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define REVERSE_FUNC(type) \
    static void reverse_##type(type* data, int len) { \
        int start = 0, end = len - 1; \
        while (start < end) { \
            type temp = data[start]; \
            data[start] = data[end]; \
            data[end] = temp; \
            start++; \
            end--; \
        } \
    }

REVERSE_FUNC(uint8_t);

static void** ks_alloc_internal(ks_config* config, uint64_t len)
{
    void** ret;
    ks_memory_info* meminfo = config->meminfo_current;
    if (meminfo->count >= KS_MAX_MEMINFO)
    {
        meminfo->next = calloc(1, sizeof(ks_memory_info));
        meminfo = meminfo->next;
        config->meminfo_current = meminfo;
    }
    ret = &meminfo->data[meminfo->count];
    meminfo->data[meminfo->count++] = calloc(1, len);
    return ret;
}

void* ks_alloc_data(ks_config* config, uint64_t len)
{
    return *ks_alloc_internal(config, len);
}

static ks_handle* ks_handle_create_internal(ks_stream* stream, void* data, ks_type type, int type_size, int internal_read_size, ks_usertype_generic* parent)
{
    ks_handle* ret = ks_alloc_data(stream->config, sizeof(ks_handle));

    ret->stream = stream;
    ret->pos = stream ? stream->pos : 0;
    ret->data = data;
    ret->type = type;
    ret->type_size = type_size;
    if (internal_read_size != 0)
    {
        ret->internal_read = ks_alloc_data(stream->config, internal_read_size);
    }
    ret->parent = parent;

    return ret;
}

void* ks_alloc_obj(ks_stream* stream, int size, ks_type type, int type_size, int internal_read_size, ks_usertype_generic* parent)
{
    ks_usertype_generic* ret = ks_alloc_data(stream->config, size);
    ret->handle = ks_handle_create_internal(stream, ret, type, type_size, internal_read_size, parent);
    return ret;
}

void* ks_realloc(ks_config* config, void* old, uint64_t len)
{
    ks_memory_info* meminfo = config->meminfo_start;
    if (!old)
    {
        config->meminfo_last_realloc = ks_alloc_internal(config, len);
        return *config->meminfo_last_realloc;
    }

    if (config->meminfo_last_realloc && *config->meminfo_last_realloc == old)
    {
        *config->meminfo_last_realloc = realloc(old, len);
        return *config->meminfo_last_realloc;
    }

    /* Fallback, search memory in whole list */
    while (meminfo)
    {
        int i;
        for (i = 0; i < meminfo->count; i++)
        {
           if (meminfo->data[i] == old)
           {
               void* ret = realloc(old, len);
               meminfo->data[i] = ret;
               config->meminfo_last_realloc = &meminfo->data[i];
               return ret;
           }
        }
        meminfo = meminfo->next;
    }

    /* Should never happen */

    KS_ERROR(config, "Can't realloc data that was not allocated using ks_alloc/ks_realloc!", KS_ERROR_REALLOC_FAILED);
    return 0;
}

ks_config* ks_config_create_internal(ks_log log, ks_ptr_inflate inflate, ks_ptr_str_decode str_decode)
{
    ks_config* config = calloc(1, sizeof(ks_config));
    config->inflate = inflate;
    config->str_decode = str_decode;
    config->log = log;
    config->fake_stream = calloc(1, sizeof(ks_stream));
    config->fake_stream->config = config;
    config->meminfo_start = calloc(1, sizeof(ks_memory_info));
    config->meminfo_current = config->meminfo_start;
    return config;
}

void ks_config_destroy(ks_config* config)
{
    ks_memory_info* meminfo = config->meminfo_start;
    while (meminfo)
    {
        int i;
        void* last = meminfo;
        for (i = 0; i < meminfo->count; i++)
        {
            free(meminfo->data[i]);
        }
        meminfo = meminfo->next;
        free(last);
    }
    free(config->fake_stream);
    free(config);
}

ks_stream* ks_stream_create_from_file(FILE* file, ks_config* config)
{
    ks_stream* ret;

    if (!file)
    {
        return 0;
    }

    ret = ks_alloc_data(config, sizeof(ks_stream));
    ret->config = config;
    ret->is_file = 1;
    ret->file = file;

    fseek(file, 0, SEEK_END);
    ret->length = ftell(file);
    return ret;
}

ks_stream* ks_stream_create_from_bytes(ks_bytes* bytes)
{
    ks_stream* ret = ks_alloc_data(HANDLE(bytes)->stream->config, sizeof(ks_stream));
    ks_stream* stream = HANDLE(bytes)->stream;

    ret->config = stream->config;
    ret->is_file = stream->is_file;
    if (bytes->data_direct)
    {
        ret->data  = bytes->data_direct;
        ret->is_file = 0;
        ret->start = 0;
    }
    else
    {
        ret->file = stream->file;
        ret->data = stream->data;
        ret->start = bytes->pos + stream->start;
    }
    ret->length = bytes->length;
    ret->parent = stream;

    return ret;
}

ks_stream* ks_stream_create_from_memory(uint8_t* data, int len, ks_config* config)
{
    ks_stream* ret = ks_alloc_data(config, sizeof(ks_stream));

    ret->config = config;
    ret->is_file = 0;
    ret->data = data;
    ret->length = len;

    return ret;
}

ks_stream* ks_stream_get_root(ks_stream* stream)
{
    while (stream->parent)
    {
        stream = stream->parent;
    }
    return stream;
}

ks_bool ks_stream_is_eof(ks_stream* stream)
{
    return stream->pos == stream->length && stream->bits_left == 0;
}

uint64_t ks_stream_get_pos(ks_stream* stream)
{
    return stream->pos;
}

uint64_t ks_stream_get_length(ks_stream* stream)
{
    return stream->length;
}

void ks_stream_seek(ks_stream* stream, uint64_t pos)
{
    KS_ASSERT(pos > stream->length, "End of stream", KS_ERROR_END_OF_STREAM, VOID);
    stream->pos = pos;
}

static void stream_read_bytes_nomove(const ks_stream* stream, uint64_t pos, uint64_t len, uint8_t* bytes)
{
    KS_ASSERT(pos + len > stream->length, "End of stream", KS_ERROR_END_OF_STREAM, VOID);
    if (stream->is_file)
    {
        int success = fseek(stream->file, stream->start + pos, SEEK_SET);
        size_t read = fread(bytes, 1, len, stream->file);
        KS_ASSERT(success != 0, "Failed to seek", KS_ERROR_SEEK_FAILED, VOID);
        KS_ASSERT(len != read, "Failed to read", KS_ERROR_READ_FAILED, VOID);
    }
    else
    {
        memcpy(bytes, stream->data + stream->start + pos, len);
    }
}

static void stream_read_bytes(ks_stream* stream, uint64_t len, void* bytes)
{
    KS_CHECK(stream_read_bytes_nomove(stream, stream->pos, len, bytes), VOID);
    stream->pos += len;
}

static int64_t stream_read_int(ks_stream* stream, int len, ks_bool big_endian)
{
    uint8_t bytes[8];
    int64_t ret = 0;
    int i;

    KS_CHECK(stream_read_bytes(stream, len, bytes), 0);

    if (big_endian)
    {
        reverse_uint8_t(bytes, len);
    }

    for (i = 0; i < len; i++)
    {
        ret += (int64_t)bytes[i] << (i*8);
    }
    return ret;
}

static ks_bool is_big_endian(void)
{
    int n = 1;
    return *(char *)&n == 0;
}

static float stream_read_float(ks_stream* stream, ks_bool big_endian)
{
    float value;
    uint8_t bytes[sizeof(value)];

    KS_CHECK(stream_read_bytes(stream, sizeof(bytes), bytes), 0);

    if (big_endian != is_big_endian())
    {
        reverse_uint8_t(bytes, sizeof(bytes));
    }

    memcpy(&value, bytes, sizeof(bytes));
    return value;
}

static double stream_read_double(ks_stream* stream, ks_bool big_endian)
{
    double value;
    uint8_t bytes[sizeof(value)];

    KS_CHECK(stream_read_bytes(stream, sizeof(bytes), bytes), 0);

    if (big_endian != is_big_endian())
    {
        reverse_uint8_t(bytes, sizeof(bytes));
    }

    memcpy(&value, bytes, sizeof(bytes));
    return value;
}


uint8_t ks_stream_read_u1(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 1, 0), 0);
    return temp;
}

uint16_t ks_stream_read_u2le(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 2, 0), 0);
    return temp;
}
uint32_t ks_stream_read_u4le(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 4, 0), 0);
    return temp;
}

uint64_t ks_stream_read_u8le(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 8, 0), 0);
    return temp;
}

uint16_t ks_stream_read_u2be(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 2, 1), 0);
    return temp;
}

uint32_t ks_stream_read_u4be(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 4, 1), 0);
    return temp;
}

uint64_t ks_stream_read_u8be(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 8, 1), 0);
    return temp;
}

int8_t ks_stream_read_s1(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 1, 0), 0);
    return temp;
}

int16_t ks_stream_read_s2le(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 2, 0), 0);
    return temp;
}

int32_t ks_stream_read_s4le(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 4, 0), 0);
    return temp;
}

int64_t ks_stream_read_s8le(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 8, 0), 0);
    return temp;
}

int16_t ks_stream_read_s2be(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 2, 1), 0);
    return temp;
}

int32_t ks_stream_read_s4be(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 4, 1), 0);
    return temp;
}

int64_t ks_stream_read_s8be(ks_stream* stream)
{
    int64_t temp;
    KS_CHECK(temp = stream_read_int(stream, 8, 1), 0);
    return temp;
}


void ks_stream_align_to_byte(ks_stream* stream)
{
    stream->bits = 0;
    stream->bits_left = 0;
}

float ks_stream_read_f4le(ks_stream* stream)
{
    float temp;
    KS_CHECK(temp = stream_read_float(stream, 0), 0);
    return temp;
}

float ks_stream_read_f4be(ks_stream* stream)
{
    float temp;
    KS_CHECK(temp = stream_read_float(stream, 1), 0);
    return temp;
}

double ks_stream_read_f8le(ks_stream* stream)
{
    double temp;
    KS_CHECK(temp = stream_read_double(stream, 0), 0);
    return temp;
}

double ks_stream_read_f8be(ks_stream* stream)
{
    double temp;
    KS_CHECK(temp = stream_read_double(stream, 1), 0);
    return temp;
}

uint64_t ks_stream_read_bits_be(ks_stream* stream, int width)
{
    int i;
    uint64_t res = 0;
    uint64_t mask;

    int bits_needed = width - stream->bits_left;
    stream->bits_left = -bits_needed & 7; /* -bits_needed mod 8 */

    if (bits_needed > 0)
    {
        uint64_t new_bits;
        uint8_t buf[8];
        int bytes_needed = ((bits_needed - 1) / 8) + 1;
        KS_ASSERT(bytes_needed > 8, "More than 8 bytes requested", KS_ERROR_BIT_VAR_TOO_BIG, 0);
        KS_CHECK(stream_read_bytes(stream, bytes_needed, buf), 0);
        for (i = 0; i < bytes_needed; i++)
        {
            res = res << 8 | buf[i];
        }

        new_bits = res;
        res = res >> stream->bits_left | (bits_needed < 64 ? stream->bits << bits_needed : 0); /* avoid undefined behavior of x << 64 */
        stream->bits = new_bits; /* will be masked at the end of the function */
    }
    else
    {
        res = stream->bits >> -bits_needed; /* shift unneeded bits out */
    }

    mask = (((uint64_t)1) << stream->bits_left) - 1;
    stream->bits &= mask;

    return res;
}

uint64_t ks_stream_read_bits_le(ks_stream* stream, int width)
{
    int i;
    uint64_t res = 0;
    int bits_needed = width - stream->bits_left;

    if (bits_needed > 0)
    {
        uint64_t new_bits;
        uint8_t buf[8];
        int bytes_needed = ((bits_needed - 1) / 8) + 1;
        KS_ASSERT(bytes_needed > 8, "More than 8 bytes requested", KS_ERROR_BIT_VAR_TOO_BIG, 0);
        KS_CHECK(stream_read_bytes(stream, bytes_needed, buf), 0);
        for (i = 0; i < bytes_needed; i++) {
            res |= (uint64_t)(buf[i]) << (i * 8);
        }

        new_bits = bits_needed < 64 ? res >> bits_needed : 0; /* avoid undefined behavior of x >> 64 */
        res = res << stream->bits_left | stream->bits;
        stream->bits = new_bits;
    }
    else
    {
        res = stream->bits;
        stream->bits >>= width;
    }

    stream->bits_left = -bits_needed & 7; /* `-bits_needed mod 8 */

    if (width < 64)
    {
        uint64_t mask = (((uint64_t)1) << width) - 1;
        res &= mask;
    }
    return res;
}

ks_bytes* ks_stream_read_bytes(ks_stream* stream, int len)
{
    ks_bytes* ret = ks_alloc_obj(stream, sizeof(ks_bytes), KS_TYPE_BYTES, sizeof(uint8_t), 0, 0);

    KS_ASSERT(stream->pos + len > stream->length, "End of stream", KS_ERROR_END_OF_STREAM, ret);

    ret->length = len;
    ret->pos = stream->pos;

    stream->pos += len;

    return ret;
}

ks_bytes* ks_stream_read_bytes_term(ks_stream* stream, uint8_t terminator, ks_bool include, ks_bool consume, ks_bool eos_error)
{
    ks_bytes* ret = ks_alloc_obj(stream, sizeof(ks_bytes), KS_TYPE_BYTES, sizeof(uint8_t), 0, 0);
    uint8_t byte;
    uint64_t start = stream->pos;
    uint64_t len;
    do
    {
        KS_CHECK(stream_read_bytes(stream, 1, &byte), ret);
    } while(byte != terminator);

    len = stream->pos - start;
    if (!consume)
    {
        stream->pos--;
    }
    if (!include)
    {
        len--;
    }

    ret->length = len;
    ret->pos = start;

    return ret;
}

ks_bytes* ks_stream_read_bytes_full(ks_stream* stream)
{
    ks_bytes* ret = ks_alloc_obj(stream, sizeof(ks_bytes), KS_TYPE_BYTES, sizeof(uint8_t), 0, 0);

    ret->length = stream->length - stream->pos;
    ret->pos = stream->pos;

    stream->pos = stream->length;

    return ret;
}

ks_bytes* ks_bytes_from_data(ks_config* config, uint64_t count, ...)
{
    ks_bytes* ret = ks_alloc_obj(config->fake_stream, sizeof(ks_bytes), KS_TYPE_BYTES, sizeof(uint8_t), 0, 0);
    va_list list;
    int i;

    ret->length = count;
    ret->data_direct = ks_alloc_data(config, count);

    va_start(list, count);
    for (i = 0; i < count; i++)
    {
        ret->data_direct[i] = va_arg(list, int);
    }
    va_end(list);

    return ret;
}

ks_bytes* ks_bytes_from_data_terminated(ks_config* config, ...)
{
    ks_bytes* ret = ks_alloc_obj(config->fake_stream, sizeof(ks_bytes), KS_TYPE_BYTES, sizeof(uint8_t), 0, 0);
    va_list list;
    int i;
    int count = 0;

    va_start(list, config);
    while (va_arg(list, int) != 0xffff)
    {
        count++;
    }
    va_end(list);

    ret->length = count;
    ret->data_direct = ks_alloc_data(config, count);

    va_start(list, config);
    for (i = 0; i < count; i++)
    {
        ret->data_direct[i] =  va_arg(list, int);
    }
    va_end(list);

    return ret;
}

ks_bytes* ks_bytes_recreate(ks_bytes* original, void* data, uint64_t length)
{
    ks_bytes* ret = ks_alloc_obj(HANDLE(original)->stream, sizeof(ks_bytes), KS_TYPE_BYTES, sizeof(uint8_t), 0, 0);
    ret->length = length;
    ret->data_direct = ks_alloc_data(HANDLE(original)->stream->config, length);
    memcpy(ret->data_direct, data, length);
    return ret;
}

ks_bytes* ks_bytes_create(ks_config* config, void* data, uint64_t length)
{
    ks_bytes* ret = ks_alloc_obj(config->fake_stream, sizeof(ks_bytes), KS_TYPE_BYTES, sizeof(uint8_t), 0, 0);
    ret->length = length;
    ret->data_direct = ks_alloc_data(config, length);
    memcpy(ret->data_direct, data, length);
    return ret;
}

uint64_t ks_bytes_get_length(const ks_bytes* bytes)
{
    return bytes->length;
}

ks_error ks_bytes_get_data(const ks_bytes* bytes, void* data)
{
    const ks_stream *stream = HANDLE(bytes)->stream;
    if (bytes->data_direct)
    {
        memcpy(data, bytes->data_direct, bytes->length);
    }
    else
    {
        stream_read_bytes_nomove(stream, bytes->pos, bytes->length, data);
    }
    return stream->config->error;
}

int64_t ks_bytes_get_at(const ks_bytes* bytes, uint64_t index)
{
    const ks_stream *stream = HANDLE(bytes)->stream;
    if (index >= bytes->length)
    {
        return 0;
    }

    if (bytes->data_direct)
    {
        return ((uint8_t*)bytes->data_direct)[index];
    }
    else
    {
        uint8_t data;
        KS_CHECK(stream_read_bytes_nomove(stream, bytes->pos + index, 1, &data), 0);
        return data;
    }
}

ks_bytes* ks_bytes_strip_right(ks_bytes* bytes, int pad)
{
    ks_bytes* ret = ks_alloc_obj(HANDLE(bytes)->stream, sizeof(ks_bytes), KS_TYPE_BYTES, sizeof(uint8_t), 0, 0);
    uint64_t len = bytes->length;

    ret->data_direct = ks_alloc_data(HANDLE(bytes)->stream->config, len);
    if (ks_bytes_get_data(bytes, ret->data_direct) != KS_ERROR_OKAY)
    {
        ret->length = 0;
        return ret;
    }

    while (len > 0 && ret->data_direct[len - 1] == pad)
        len--;

    ret->length = len;
    return ret;
}

ks_bytes* ks_bytes_terminate(ks_bytes* bytes, int term, ks_bool include)
{
    ks_bytes* ret = ks_alloc_obj(HANDLE(bytes)->stream, sizeof(ks_bytes), KS_TYPE_BYTES, sizeof(uint8_t), 0, 0);
    uint64_t len = 0;
    uint64_t max_len = bytes->length;

    ret->data_direct = ks_alloc_data(HANDLE(bytes)->stream->config, max_len);
    if (ks_bytes_get_data(bytes, ret->data_direct) != KS_ERROR_OKAY)
    {
        ret->length = 0;
        return ret;
    }

    while (len < max_len && ret->data_direct[len] != term)
        len++;

    if (include && len < max_len)
        len++;

    ret->length = len;
    return ret;
}

static int64_t array_get_int(ks_usertype_generic* array, void* data)
{
    ks_handle* handle = array->handle;
    switch(handle->type)
    {
        case KS_TYPE_ARRAY_UINT:
            switch(handle->type_size)
            {
                case 1:
                    return *(uint8_t*)data;
                case 2:
                    return *(uint16_t*)data;
                case 4:
                    return *(uint32_t*)data;
                case 8:
                    return *(uint64_t*)data;
            }
          case KS_TYPE_ARRAY_INT:
            switch(handle->type_size)
            {
                case 1:
                    return *(int8_t*)data;
                case 2:
                    return *(int16_t*)data;
                case 4:
                    return *(int32_t*)data;
                case 8:
                    return *(int64_t*)data;
            }
        default:
            break;
    }
    return 0;
}

static double array_get_float(ks_usertype_generic* array, void* data)
{
   ks_handle* handle = array->handle;
   if (handle->type == KS_TYPE_ARRAY_FLOAT)
   {
       switch (handle->type_size)
       {
           case 4:
               return *(float*)data;
           case 8:
               return *(double*)data;
       }
   }
    return 0;
}

static ks_bool array_min_max_func(ks_usertype_generic* array, ks_bool max, void* minmax, void* other)
{
    ks_handle* handle = array->handle;
    if (max)
    {
        switch (handle->type)
        {
            case KS_TYPE_ARRAY_UINT:
            case KS_TYPE_ARRAY_INT:
                return array_get_int(array, other) > array_get_int(array, minmax);
            case KS_TYPE_ARRAY_FLOAT:
                return array_get_float(array, other) > array_get_float(array, minmax);
            case KS_TYPE_ARRAY_STRING:
                return ks_string_compare(*(ks_string**)other, *(ks_string**)minmax) > 0;
            case KS_TYPE_ARRAY_BYTES:
                return ks_bytes_compare(*(ks_bytes**)other, *(ks_bytes**)minmax) > 0;
            default:
                break;
        }
    }
    else
    {
        switch (handle->type)
        {
            case KS_TYPE_ARRAY_UINT:
            case KS_TYPE_ARRAY_INT:
                return array_get_int(array, other) < array_get_int(array, minmax);
            case KS_TYPE_ARRAY_FLOAT:
                return array_get_float(array, other) < array_get_float(array, minmax);
            case KS_TYPE_ARRAY_STRING:
                return ks_string_compare(*(ks_string**)other, *(ks_string**)minmax) < 0;
            case KS_TYPE_ARRAY_BYTES:
                return ks_bytes_compare(*(ks_bytes**)other, *(ks_bytes**)minmax) < 0;
            default:
                break;
        }
    }
    return 0;
}

static void* array_min_max(ks_usertype_generic* array_in, ks_bool max)
{
    ks_handle* handle = array_in->handle;
    char* pointer;
    ks_array_generic array;
    int i;

    memcpy(&array, handle->data, sizeof(ks_array_generic)); /* Type punning */

    if (array.size == 0)
    {
        return 0;
    }

    pointer = array.data;
    for (i = 1; i < array.size; i++)
    {
        char* data_new = array.data + (i * handle->type_size);
        if (array_min_max_func(array_in, max, pointer, data_new))
        {
            pointer = data_new;
        }
    }
    return pointer;
}

int64_t ks_array_min_int(ks_usertype_generic* array)
{
    void* ret = array_min_max(array, 0);
    return array_get_int(array, ret);
}

int64_t ks_array_max_int(ks_usertype_generic* array)
{
    void* ret = array_min_max(array, 1);
    return array_get_int(array, ret);
}

double ks_array_min_float(ks_usertype_generic* array)
{
    void* ret = array_min_max(array, 0);
    return array_get_float(array, ret);
}

double ks_array_max_float(ks_usertype_generic* array)
{
    void* ret = array_min_max(array, 1);
    return array_get_float(array, ret);
}

ks_string* ks_array_min_string(ks_usertype_generic* array)
{
    void* ret = array_min_max(array, 0);
    return *(ks_string**)ret;
}

ks_string* ks_array_max_string(ks_usertype_generic* array)
{
    void* ret = array_min_max(array, 1);
    return *(ks_string**)ret;
}

ks_bytes* ks_array_min_bytes(ks_usertype_generic* array)
{
    void* ret = array_min_max(array, 0);
    return *(ks_bytes**)ret;
}

ks_bytes* ks_array_max_bytes(ks_usertype_generic* array)
{
    void* ret = array_min_max(array, 1);
    return *(ks_bytes**)ret;
}

static int64_t bytes_minmax(ks_bytes* bytes, ks_bool max)
{
    uint8_t minmax;
    uint8_t* data;
    int i;

    if (bytes->length == 0)
    {
        return 0;
    }

    data = malloc(bytes->length);
    if (ks_bytes_get_data(bytes, data) != KS_ERROR_OKAY)
    {
        free(data);
        return 0;
    }
    minmax = data[0];

    for (i = 1; i < bytes->length; i++)
    {
        if (max)
        {
            if (data[i] > minmax)
            {
                minmax = data[i];
            }
        }
        else
        {
            if (data[i] < minmax)
            {
                minmax = data[i];
            }
        }
    }

    free(data);
    return minmax;
}

int64_t ks_bytes_min(ks_bytes* bytes)
{
    return bytes_minmax(bytes, 0);
}

int64_t ks_bytes_max(ks_bytes* bytes)
{
    return bytes_minmax(bytes, 1);
}

ks_string* ks_string_concat(ks_string* s1, ks_string* s2)
{
    ks_string* ret = ks_alloc_obj(HANDLE(s1)->stream, sizeof(ks_string), KS_TYPE_STRING, sizeof(char), 0, 0);
    ret->len = s1->len + s2->len;
    ret->data = ks_alloc_data(HANDLE(s1)->stream->config, ret->len + 1);
    memcpy(ret->data, s1->data, s1->len);
    memcpy(ret->data + s1->len, s2->data, s2->len);

    return ret;
}

ks_string* ks_string_from_int(ks_config* config, int64_t i, int base)
{
    ks_string* ret = ks_alloc_obj(config->fake_stream, sizeof(ks_string), KS_TYPE_STRING, sizeof(char), 0, 0);
    char buf[50] = {0};
    if (base == 10)
    {
        sprintf(buf, "%lld", (long long int)i);
    }
    else if (base == 16)
    {
        sprintf(buf, "%llx", (long long int)i);
    }
    
    ret->len = strlen(buf);
    ret->data = ks_alloc_data(config, ret->len + 1);
    memcpy(ret->data, buf, ret->len);

    return ret;
}

int64_t ks_string_to_int(ks_string* str, int base)
{
    long long int i = 0;
    if (base == 10)
    {
        sscanf(str->data, "%lld", &i);
    }
    else if (base == 16)
    {
        sscanf(str->data, "%llx", &i);
    }
    else
    {
        return strtol(str->data, 0, base);
    }

    return i;
}

ks_string* ks_string_reverse(ks_string* str)
{
    int i;
    ks_string* ret = ks_alloc_obj(HANDLE(str)->stream, sizeof(ks_string), KS_TYPE_STRING, sizeof(char), 0, 0);
    ret->len = str->len;
    ret->data = ks_alloc_data(HANDLE(str)->stream->config, ret->len + 1);

    for (i = 0; i < str->len; i++)
    {
        ret->data[i] = str->data[str->len - i - 1];
    }
    return ret;
}

ks_string* ks_string_from_bytes(ks_bytes* bytes, ks_string* encoding)
{
    ks_string* tmp = ks_alloc_obj(HANDLE(bytes)->stream, sizeof(ks_string), KS_TYPE_STRING, sizeof(char), 0, 0);
    ks_string* ret;

    tmp->len = bytes->length;
    tmp->data = ks_alloc_data(HANDLE(bytes)->stream->config, tmp->len + 1);
    if(ks_bytes_get_data(bytes, tmp->data) != KS_ERROR_OKAY)
    {
        tmp->len = 0;
    }

    ret = HANDLE(bytes)->stream->config->str_decode(tmp, encoding->data);

    return ret;
}

ks_string* ks_string_from_cstr(ks_config* config, const char* data)
{
    ks_string* ret = ks_alloc_obj(config->fake_stream, sizeof(ks_string), KS_TYPE_STRING, sizeof(char), 0, 0);
    ret->len = strlen(data);
    ret->data = ks_alloc_data(config, ret->len + 1);
    memcpy(ret->data, data, ret->len);

    return ret;
}

ks_string* ks_string_substr(ks_string* str, int start, int end)
{
    ks_string* ret = ks_alloc_obj(HANDLE(str)->stream, sizeof(ks_string), KS_TYPE_STRING, sizeof(char), 0, 0);
    ret->len = end - start;
    ret->data = ks_alloc_data(HANDLE(str)->stream->config, ret->len + 1);
    memcpy(ret->data, str->data + start, ret->len);
    return ret;
}

#define ARRAY_FROM_DATA(config, type_array, type_element, type_enum) \
    type_array* ret = ks_alloc_obj(config->fake_stream, sizeof(type_array), type_enum, sizeof(type_element), 0, 0); \
    va_list list; \
    int i; \
    ret->size = count; \
    ret->data = ks_alloc_data(config, sizeof(type_element) * count); \
    va_start(list, count); \
    for (i = 0; i < count; i++) {  \
        ret->data[i] = va_arg(list, type_element);  \
    }  \
    va_end(list);  \
    return ret;

ks_array_int8_t* ks_array_int8_t_from_data(ks_config* config, uint64_t count, ...)
{
    ARRAY_FROM_DATA(config, ks_array_int8_t, int, KS_TYPE_ARRAY_INT);
}

ks_array_int16_t* ks_array_int16_t_from_data(ks_config* config, uint64_t count, ...)
{
    ARRAY_FROM_DATA(config, ks_array_int16_t, int, KS_TYPE_ARRAY_INT);
}

ks_array_int32_t* ks_array_int32_t_from_data(ks_config* config, uint64_t count, ...)
{
    ARRAY_FROM_DATA(config, ks_array_int32_t, int32_t, KS_TYPE_ARRAY_INT);
}

ks_array_int64_t* ks_array_int64_t_from_data(ks_config* config, uint64_t count, ...)
{
    ARRAY_FROM_DATA(config, ks_array_int64_t, int64_t, KS_TYPE_ARRAY_INT);
}

ks_array_uint8_t* ks_array_uint8_t_from_data(ks_config* config, uint64_t count, ...)
{
    ARRAY_FROM_DATA(config, ks_array_uint8_t, int, KS_TYPE_ARRAY_INT);
}

ks_array_uint16_t* ks_array_uint16_t_from_data(ks_config* config, uint64_t count, ...)
{
    ARRAY_FROM_DATA(config, ks_array_uint16_t, int, KS_TYPE_ARRAY_INT);
}

ks_array_uint32_t* ks_array_uint32_t_from_data(ks_config* config, uint64_t count, ...)
{
    ARRAY_FROM_DATA(config, ks_array_uint32_t, uint32_t, KS_TYPE_ARRAY_INT);
}

ks_array_uint64_t* ks_array_uint64_t_from_data(ks_config* config, uint64_t count, ...)
{
    ARRAY_FROM_DATA(config, ks_array_uint64_t, uint64_t, KS_TYPE_ARRAY_INT);
}

ks_array_float* ks_array_float_from_data(ks_config* config, uint64_t count, ...)
{
    ARRAY_FROM_DATA(config, ks_array_float, double, KS_TYPE_ARRAY_FLOAT);
}

ks_array_double* ks_array_double_from_data(ks_config* config, uint64_t count, ...)
{
    ARRAY_FROM_DATA(config, ks_array_double, double, KS_TYPE_ARRAY_FLOAT);
}

ks_array_string* ks_array_string_from_data(ks_config* config, uint64_t count, ...)
{
    ARRAY_FROM_DATA(config, ks_array_string, ks_string*, KS_TYPE_ARRAY_STRING);
}

ks_array_usertype_generic* ks_array_usertype_generic_from_data(ks_config* config, uint64_t count, ...)
{
    ARRAY_FROM_DATA(config, ks_array_usertype_generic, ks_usertype_generic*, KS_TYPE_ARRAY_USERTYPE);
}

int ks_string_compare(ks_string* left, ks_string* right)
{
    return strcmp(left->data, right->data);
}

int ks_bytes_compare(ks_bytes* left, ks_bytes* right)
{
    int ret;
    uint8_t* data_left;
    uint8_t* data_right;
    uint64_t len;

    data_left = malloc(left->length);
    data_right = malloc(right->length);

    if (ks_bytes_get_data(left, data_left) != KS_ERROR_OKAY || ks_bytes_get_data(right, data_right) != KS_ERROR_OKAY)
    {
        free(data_left);
        free(data_right);
        return 0;
    }

    len = min(left->length, right->length);

    ret = memcmp(data_left, data_right, len);

    if (ret == 0)
    {
        if (left->length < right->length)
        {
            ret = -1;
        }
        if (left->length > right->length)
        {
            ret = 1;
        }
    }

    free(data_left);
    free(data_right);
    return ret;
}

int64_t ks_mod(int64_t a, int64_t b)
{
    int64_t r;
    if (b <= 0)
    {
        return 0;
    }
    r = a % b;
    if (r < 0) r += b;
    return r;
}

int64_t ks_div(int64_t a, int64_t b)
{
    int64_t ret = a / b;
    int64_t mod = a %b;
    if (a < 0 && b > 0 && mod != 0)
    {
        ret--;
    }
    return ret;
}

ks_bytes* ks_bytes_process_xor_int(ks_bytes* bytes, uint64_t xor_int, int count_xor_bytes)
{
    uint64_t i;
    ks_bytes* ret = ks_alloc_obj(HANDLE(bytes)->stream, sizeof(ks_bytes), KS_TYPE_BYTES, sizeof(uint8_t), 0, 0);

    ret->length = bytes->length;
    ret->data_direct = ks_alloc_data(HANDLE(bytes)->stream->config, bytes->length);

    if (ks_bytes_get_data(bytes, ret->data_direct) != KS_ERROR_OKAY)
    {
        ret->length = 0;
        return ret;
    }

    for (i = 0; i < ret->length; i++)
    {
        ret->data_direct[i] ^= xor_int;
    }
    return ret;
}

ks_bytes* ks_bytes_process_xor_bytes(ks_bytes* bytes, ks_bytes* xor_bytes)
{
    uint64_t i;
    int xor_pos = 0;
    ks_bytes* ret = ks_alloc_obj(HANDLE(bytes)->stream, sizeof(ks_bytes), KS_TYPE_BYTES, sizeof(uint8_t), 0, 0);
    uint8_t* xor_data = malloc(xor_bytes->length);

    ret->length = bytes->length;
    ret->data_direct = ks_alloc_data(HANDLE(bytes)->stream->config, bytes->length);

    if (ks_bytes_get_data(bytes, ret->data_direct) != KS_ERROR_OKAY || ks_bytes_get_data(xor_bytes, xor_data) != KS_ERROR_OKAY)
    {
        free(xor_data);
        ret->length = 0;
        return ret;
    }

    for (i = 0; i < ret->length; i++)
    {
        ret->data_direct[i] ^= xor_data[xor_pos];
        xor_pos++;
        if (xor_pos >= xor_bytes->length)
        {
            xor_pos = 0;
        }
    }
    free(xor_data);
    return ret;
}

ks_bytes* ks_bytes_process_rotate_left(ks_bytes* bytes, int count)
{
    uint64_t i;
    ks_bytes* ret = ks_alloc_obj(HANDLE(bytes)->stream, sizeof(ks_bytes), KS_TYPE_BYTES, sizeof(uint8_t), 0, 0);

    ret->length = bytes->length;
    ret->data_direct = ks_alloc_data(HANDLE(bytes)->stream->config, bytes->length);

    if (ks_bytes_get_data(bytes, ret->data_direct) != KS_ERROR_OKAY)
    {
        ret->length = 0;
        return ret;
    }

    for (i = 0; i < ret->length; i++)
    {
        uint64_t b = ret->data_direct[i];
        ret->data_direct[i] = (b << count) | (b >> (8 - count));
    }
    return ret;
}

void ks_bytes_set_error(ks_bytes* bytes, ks_error error)
{
    ks_error* err = &HANDLE(bytes)->stream->config->error;
    if (*err == 0)
    {
        *err = error;
    }
}

void ks_string_set_error(ks_string* str, ks_error error)
{
    ks_error* err = &HANDLE(str)->stream->config->error;
    if (*err == 0)
    {
        *err = error;
    }
}

ks_usertype_generic* ks_usertype_get_root(ks_usertype_generic* data)
{
    while (data->handle->parent)
    {
        data = data->handle->parent;
    }
    return data;
}

ks_config* ks_usertype_get_config(ks_usertype_generic* base)
{
    return base->handle->stream->config;
}

ks_error ks_stream_get_error(ks_stream* stream)
{
    return stream->config->error;
}

ks_bytes* ks_inflate(ks_config* config, ks_bytes* bytes)
{
    return config->inflate(bytes);
}
