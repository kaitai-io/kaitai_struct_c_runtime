/* Kaitai Struct C Runtime Header */

#ifndef KAITAI_STRUCT_INTERNAL_H
#define KAITAI_STRUCT_INTERNAL_H

#include "kaitaistruct.h"

/* Private functions */

ks_config* ks_config_create_internal(ks_log log, ks_ptr_inflate inflate, ks_ptr_str_decode str_decode);

ks_stream* ks_stream_create_from_bytes(ks_bytes* bytes);
ks_stream* ks_stream_get_root(ks_stream* stream);
ks_error ks_stream_get_error(ks_stream* stream);
ks_usertype_generic* ks_usertype_get_root(ks_usertype_generic* data);
ks_usertype_generic* ks_usertype_get_parent(ks_usertype_generic* base);
ks_config* ks_stream_get_config(ks_stream* stream);
ks_stream* ks_usertype_get_stream(ks_usertype_generic* base);
void* ks_usertype_get_internal_read(ks_usertype_generic* base);

uint8_t ks_stream_read_u1(ks_stream* stream);
uint16_t ks_stream_read_u2le(ks_stream* stream);
uint32_t ks_stream_read_u4le(ks_stream* stream);
uint64_t ks_stream_read_u8le(ks_stream* stream);
uint16_t ks_stream_read_u2be(ks_stream* stream);
uint32_t ks_stream_read_u4be(ks_stream* stream);
uint64_t ks_stream_read_u8be(ks_stream* stream);

int8_t ks_stream_read_s1(ks_stream* stream);
int16_t ks_stream_read_s2le(ks_stream* stream);
int32_t ks_stream_read_s4le(ks_stream* stream);
int64_t ks_stream_read_s8le(ks_stream* stream);
int16_t ks_stream_read_s2be(ks_stream* stream);
int32_t ks_stream_read_s4be(ks_stream* stream);
int64_t ks_stream_read_s8be(ks_stream* stream);

float ks_stream_read_f4le(ks_stream* stream);
float ks_stream_read_f4be(ks_stream* stream);
double ks_stream_read_f8le(ks_stream* stream);
double ks_stream_read_f8be(ks_stream* stream);

uint64_t ks_stream_read_bits_le(ks_stream* stream, int width);
uint64_t ks_stream_read_bits_be(ks_stream* stream, int width);
void ks_stream_align_to_byte(ks_stream* stream);

ks_bytes* ks_stream_read_bytes(ks_stream* stream, int len);
ks_bytes* ks_stream_read_bytes_term(ks_stream* stream, uint8_t terminator, ks_bool include, ks_bool consume, ks_bool eos_error);
ks_bytes* ks_stream_read_bytes_full(ks_stream* stream);
ks_bool ks_stream_is_eof(ks_stream* stream);
uint64_t ks_stream_get_pos(ks_stream* stream);
uint64_t ks_stream_get_length(ks_stream* stream);
void ks_stream_seek(ks_stream* stream, uint64_t pos);

ks_bytes* ks_bytes_from_data(ks_config* config, uint64_t count, ...);
ks_bytes* ks_bytes_from_data_terminated(ks_config* config, ...);
ks_bytes* ks_array_min_bytes(ks_usertype_generic* array);
ks_bytes* ks_array_max_bytes(ks_usertype_generic* array);
ks_bytes* ks_bytes_strip_right(ks_bytes* bytes, int pad);
ks_bytes* ks_bytes_terminate(ks_bytes* bytes, int term, ks_bool include);
ks_bytes* ks_bytes_process_xor_int(ks_bytes* bytes, uint64_t xor_int, int count_xor_bytes);
ks_bytes* ks_bytes_process_xor_bytes(ks_bytes* bytes, ks_bytes* xor_bytes);
ks_bytes* ks_bytes_process_rotate_left(ks_bytes* bytes, int count);
int64_t ks_bytes_get_at(const ks_bytes* bytes, uint64_t index);

ks_string* ks_string_concat(ks_string* s1, ks_string* s2);
ks_string* ks_string_from_int(ks_config* config, int64_t i, int base);
int64_t ks_string_to_int(ks_string* str, int base);
ks_string* ks_string_from_bytes(ks_bytes* bytes, ks_string* encoding);
ks_string* ks_string_reverse(ks_string* str);
ks_string* ks_string_substr(ks_string* str, int start, int end);

ks_array_int8_t* ks_array_int8_t_from_data(ks_config* config, uint64_t count, ...);
ks_array_int16_t* ks_array_int16_t_from_data(ks_config* config, uint64_t count, ...);
ks_array_int32_t* ks_array_int32_t_from_data(ks_config* config, uint64_t count, ...);
ks_array_int64_t* ks_array_int64_t_from_data(ks_config* config, uint64_t count, ...);
ks_array_uint8_t* ks_array_uint8_t_from_data(ks_config* config, uint64_t count, ...);
ks_array_uint16_t* ks_array_uint16_t_from_data(ks_config* config, uint64_t count, ...);
ks_array_uint32_t* ks_array_uint32_t_from_data(ks_config* config, uint64_t count, ...);
ks_array_uint64_t* ks_array_uint64_t_from_data(ks_config* config, uint64_t count, ...);
ks_array_float* ks_array_float_from_data(ks_config* config, uint64_t count, ...);
ks_array_double* ks_array_double_from_data(ks_config* config, uint64_t count, ...);

ks_array_string* ks_array_string_from_data(ks_config* config, uint64_t count, ...);
ks_array_usertype_generic* ks_array_usertype_generic_from_data(ks_config* config, uint64_t count, ...);

int ks_string_compare(ks_string* left, ks_string* right);
int ks_bytes_compare(ks_bytes* left, ks_bytes* right);

ks_string* ks_array_min_string(ks_usertype_generic* array);
ks_string* ks_array_max_string(ks_usertype_generic* array);
int64_t ks_array_min_int(ks_usertype_generic* array);
int64_t ks_array_max_int(ks_usertype_generic* array);
int64_t ks_bytes_min(ks_bytes* bytes);
int64_t ks_bytes_max(ks_bytes* bytes);
double ks_array_min_float(ks_usertype_generic* array);
double ks_array_max_float(ks_usertype_generic* array);

int64_t ks_mod(int64_t a, int64_t b);
int64_t ks_div(int64_t a, int64_t b);

void* ks_alloc_data(ks_config* config, uint64_t len);
void* ks_alloc_obj(ks_stream* stream, int size, ks_type type, int type_size, int internal_read_size, ks_usertype_generic* parent);
void* ks_realloc(ks_config* config, void* old, uint64_t len);

ks_bytes* ks_inflate(ks_config* config, ks_bytes* bytes);
void ks_log_error(const ks_stream* stream, ks_error error, const char* message, const char* file, int line);
ks_bool ks_check_error(const ks_stream* stream, const char* file, int line);

/* Internal Macros */

#define KS_ASSERT(expr, message, errorcode, DEFAULT) \
    if (expr) { \
        ks_log_error(stream, errorcode, message, __FILE__, __LINE__); \
        return DEFAULT; \
    }

#define KS_ASSERT_VOID(expr, message, errorcode) \
    KS_ASSERT(expr, message, errorcode, ;)

#define KS_ASSERT_DATA(expr, message, errorcode) \
    KS_ASSERT(expr, message, errorcode, data)

#define KS_CHECK(expr, DEFAULT) \
    expr; \
    if (ks_check_error(stream, __FILE__, __LINE__)) { \
        return DEFAULT; \
    }

#define KS_CHECK_VOID(expr) \
    KS_CHECK(expr, ;)

#define KS_CHECK_DATA(expr) \
    KS_CHECK(expr, data)

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8) || __clang__
#define FIELD(expr, type, field)                                          \
    ({                                                              \
        ks_usertype_generic* expr_ = (ks_usertype_generic*)(expr);                                 \
        __auto_type ret = ((type##_internal*)ks_usertype_get_internal_read(expr_))->_get_##field((type*)expr_);    \
        ret;                                                        \
    })
#else
#define FIELD(expr, type, field) \
    ((type##_internal*)ks_usertype_get_internal_read((ks_usertype_generic*)(expr)))->_get_##field((type*)expr)
#endif

#endif
