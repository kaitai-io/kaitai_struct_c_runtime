/* Kaitai Struct C Runtime Header

Usage:
1) Define KS_USE_ICONV or KS_USE_ZLIB if needed
2) Include {TYPENAME}.h
3) Create config with ks_config_init
4) Create stream, e.g. ks_stream_create_from_file
5) Read type: ksx_read_{TYPENAME}_from_stream;
*/

#ifndef KAITAI_STRUCT_H
#define KAITAI_STRUCT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef struct ks_stream ks_stream;
typedef struct ks_handle ks_handle;
typedef struct ks_bytes ks_bytes;
typedef struct ks_string ks_string;

typedef char ks_bool;
typedef void (*ks_callback)(void* data);
typedef void (*ks_log)(const char* text);
typedef ks_bytes* (*ks_ptr_inflate)(ks_bytes* bytes);
typedef ks_string* (*ks_ptr_str_decode)(ks_string* src, const char* src_enc);

typedef enum ks_error
{
    KS_ERROR_OKAY,
    KS_ERROR_OTHER,
    KS_ERROR_ZLIB,
    KS_ERROR_ZLIB_MISSING,
    KS_ERROR_ICONV,
    KS_ERROR_END_OF_STREAM,
    KS_ERROR_SEEK_FAILED,
    KS_ERROR_READ_FAILED,
    KS_ERROR_BIT_VAR_TOO_BIG,
    KS_ERROR_VALIDATION_FAILED,
    KS_ERROR_ENDIANESS_UNSPECIFIED,
    KS_ERROR_REALLOC_FAILED,
} ks_error;

typedef struct ks_config ks_config;

static ks_config* ks_config_create(ks_log log);
void ks_config_destroy(ks_config* config);

typedef struct ks_usertype_generic
{
    ks_handle* handle;
} ks_usertype_generic;

typedef struct ks_custom_decoder
{
    void* userdata;
    ks_bytes* (*decode)(void* userdata, ks_bytes* bytes);
} ks_custom_decoder;

struct ks_string
{
    ks_usertype_generic kaitai_base;
    int64_t len;
    char* data;
};

/* Public functions */

ks_stream* ks_stream_create_from_file(FILE* file, ks_config* config);
ks_stream* ks_stream_create_from_memory(uint8_t* data, int len, ks_config* config);

ks_bytes* ks_bytes_recreate(ks_bytes* original, void* data, uint64_t length);
ks_bytes* ks_bytes_create(ks_config* config, void* data, uint64_t length);

uint64_t ks_bytes_get_length(const ks_bytes* bytes);
ks_error ks_bytes_get_data(const ks_bytes* bytes, void* data);

ks_string* ks_string_from_cstr(ks_config* config, const char* data);

void ks_bytes_set_error(ks_bytes* bytes, ks_error error);
void ks_string_set_error(ks_string* str, ks_error error);

ks_config* ks_usertype_get_config(ks_usertype_generic* base);

/* Typeinfo */

typedef enum ks_type
{
    KS_TYPE_UNKNOWN = 0,
    KS_TYPE_ARRAY_UINT,
    KS_TYPE_ARRAY_INT,
    KS_TYPE_ARRAY_FLOAT,
    KS_TYPE_ARRAY_STRING,
    KS_TYPE_ARRAY_BYTES,
    KS_TYPE_ARRAY_USERENUM,
    KS_TYPE_ARRAY_USERTYPE,
    KS_TYPE_BYTES,
    KS_TYPE_USERTYPE,
    KS_TYPE_STRING,
} ks_type;

/* Array types */

typedef struct ks_array_generic
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    void* data;
} ks_array_generic;

typedef struct ks_array_uint8_t
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    uint8_t* data;
} ks_array_uint8_t;

typedef struct ks_array_uint16_t
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    uint16_t* data;
} ks_array_uint16_t;

typedef struct ks_array_uint32_t
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    uint32_t* data;
} ks_array_uint32_t;

typedef struct ks_array_uint64_t
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    uint64_t* data;
} ks_array_uint64_t;

typedef struct ks_array_int8_t
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    int8_t* data;
} ks_array_int8_t;

typedef struct ks_array_int16_t
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    int16_t* data;
} ks_array_int16_t;

typedef struct ks_array_int32_t
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    int32_t* data;
} ks_array_int32_t;

typedef struct ks_array_int64_t
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    int64_t* data;
} ks_array_int64_t;

typedef struct ks_array_float
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    float* data;
} ks_array_float;

typedef struct ks_array_double
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    double* data;
} ks_array_double;

typedef struct ks_array_string
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    ks_string** data;
} ks_array_string;

typedef struct ks_array_bytes
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    ks_bytes** data;
} ks_array_bytes;

typedef struct ks_array_any
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    void** data;
} ks_array_any;

typedef struct ks_array_usertype_generic
{
    ks_usertype_generic kaitai_base;
    int64_t size;
    ks_usertype_generic** data;
} ks_array_usertype_generic;

/* Dynamic functions */

ks_config* ks_config_create_internal(ks_log log, ks_ptr_inflate inflate, ks_ptr_str_decode str_decode);

#ifdef KS_USE_ZLIB
#include <zlib.h>
static ks_bytes* ks_inflate_handler(ks_bytes* bytes)
{
    uint64_t length_in = ks_bytes_get_length(bytes);
    uint8_t* data_in = 0;
    uint8_t* data_out = 0;
    uint64_t length_out = 0;
    z_stream strm = {0};
    uint8_t outbuffer[1024*64];
    int ret_zlib;
    ks_bytes* ret;
    ks_error err;

    data_in = (uint8_t*)malloc(length_in);
    err = ks_bytes_get_data(bytes, data_in);
    if (err != KS_ERROR_OKAY)
    {
        ks_bytes_set_error(bytes, KS_ERROR_ZLIB);
        return 0;
    }

    if (inflateInit(&strm) != Z_OK)
        goto error;

    strm.next_in = (Bytef*)data_in;
    strm.avail_in = length_in;

    do {
        strm.next_out = outbuffer;
        strm.avail_out = sizeof(outbuffer);

        ret_zlib = inflate(&strm, 0);

        if (length_out < strm.total_out) {
            data_out = (uint8_t*)realloc(data_out, strm.total_out);
            memcpy(data_out + length_out, outbuffer, strm.total_out - length_out);
            length_out = strm.total_out;
        }
    } while (ret_zlib == Z_OK);

    if (ret_zlib != Z_STREAM_END)
        goto error;

    if (inflateEnd(&strm) != Z_OK)
        goto error;

    ret = ks_bytes_recreate(bytes, data_out, length_out);
    free(data_in);
    free(data_out);
    return ret;

 error:
    free(data_in);
    free(data_out);
    ks_bytes_set_error(bytes, KS_ERROR_ZLIB);
    return 0;
}
#else
static ks_bytes* ks_inflate_handler(ks_bytes* bytes)
{
    ks_bytes_set_error(bytes, KS_ERROR_ZLIB_MISSING);
    return 0;
}
#endif

#ifdef KS_USE_ICONV
#include <iconv.h>
#include <errno.h>
static ks_string* ks_str_decode_handler(ks_string* src, const char* src_enc) {
    iconv_t cd = iconv_open("UTF-8", src_enc);
    size_t src_left = src->len;
    size_t dst_len = src->len * 2;
    char* dst = (char*)calloc(1, dst_len + 1); /* Alloc one more for null terminator */
    char* dst_ptr = dst;
    char* src_ptr = src->data;
    size_t dst_left = dst_len;
    size_t res = -1;
    ks_string* ret;

    if (cd == (iconv_t) -1) {
        if (errno == EINVAL) {
            ks_string_set_error(src, KS_ERROR_ICONV);
        } else {
            ks_string_set_error(src, KS_ERROR_ICONV);
        }
    }

    while (res == (size_t) -1) {
        res = iconv(cd, &src_ptr, &src_left, &dst_ptr, &dst_left);
        if (res == (size_t) -1) {
            if (errno == E2BIG) {
                size_t dst_used = dst_len - dst_left;
                dst_left += dst_len;
                dst_len += dst_len;
                dst = (char*)realloc(dst, dst_len + 1); /* Alloc one more for null terminator */
                dst_ptr = &dst[dst_used];
                memset(dst_ptr, 0, dst_left + 1); /* Alloc one more for null terminator */
            } else {
                ks_string_set_error(src, KS_ERROR_ICONV);
            }
        }
    }

    if (iconv_close(cd) != 0) {
        ks_string_set_error(src, KS_ERROR_ICONV);
    }

    ret = ks_string_from_cstr(ks_usertype_get_config(&src->kaitai_base), dst);
    free(dst);
    return ret;
}
#else
static ks_string* ks_str_decode_handler(ks_string* src, const char* src_enc)
{
    return src;
}
#endif

static ks_config* ks_config_create(ks_log log)
{
    return ks_config_create_internal(log, ks_inflate_handler, ks_str_decode_handler);
}

#endif
