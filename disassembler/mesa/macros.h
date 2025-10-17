#pragma once

#pragma clang diagnostic ignored "-Winitializer-overrides"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wunused-function"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define BITFIELD_BIT(b) (1u << (b))
#define BITFIELD_MASK(b) ((b) == 32 ? (~0u) : BITFIELD_BIT((b) & 31) - 1)
#define DIV_ROUND_UP(A, B) (((A) + (B) - 1) / (B))
#define FALLTHROUGH
#define IS_POT(v) (((v) & ((v) - 1)) == 0)
#define IS_POT_NONZERO(v) ((v) != 0 && IS_POT(v))
#define MAX2(a, b) (a > b ? a : b)
#define MIN2(a, b) (a < b ? a : b)
#define NDEBUG
#define PRINTFLIKE(...)
#define STATIC_ASSERT static_assert
#define UNUSED __attribute__((unused))
#define fprintf mesa_fprintf
#define fputs mesa_fputs
#define fputc mesa_fputc
#define mesa_loge(...)
#define uif(v) (*(float*)&v)
#define util_bitcount __builtin_popcount
#define util_logbase2(n) ((sizeof(unsigned) * 8 - 1) - __builtin_clz(n | 1))
#define util_sign_extend(v, w) (((int)(v) << (32 - w)) >> w)
#define _util_printf_format(...)

#undef unreachable
#define unreachable(...)

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

int mesa_fprintf(FILE* fp, const char* format, ...);
int mesa_fputs(const char* str, FILE* fp);
int mesa_fputc(char c, FILE* fp);

#define ralloc_array(p, s, c) ralloc_size(p, sizeof(s) * c)
void* ralloc_size(void*, size_t size);
void* rzalloc_size(void*, size_t size);
void ralloc_free(void* pointer);

struct hash_entry { void* data; };
struct hash_table *_mesa_pointer_hash_table_create(void*);
struct hash_entry *_mesa_hash_table_search(void* table, void* key);
void _mesa_hash_table_insert(void* table, void* key, void* value);

float _mesa_half_to_float(uint16_t val);

void mesa_cleanup();

#ifdef __cplusplus
};
#endif
