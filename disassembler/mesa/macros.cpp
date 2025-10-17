#include <stdlib.h>
#include <unordered_map>
#include <vector>
#include "macros.h"

int mesa_fprintf(FILE* fp, const char* format, ...)
{
   va_list args;
   va_start(args, format);
   char temp[256];
   int ret = vsnprintf(temp, 256, format, args);
   va_end(args);

   if (fp) {
      auto& string = *(std::string*)fp;
      string += temp;
   }
   return ret;
}

int mesa_fputs(const char* str, FILE* fp)
{
   if (fp) {
      auto& string = *(std::string*)fp;
      string += str;
   }
   return 0;
}

int mesa_fputc(char c, FILE* fp)
{
   if (fp) {
      auto& string = *(std::string*)fp;
      string += c;
   }
   return 0;
}

static std::vector<void*> allocated_pool;

void* ralloc_size(void*, size_t size)
{
   allocated_pool.emplace_back(malloc(size));
   return allocated_pool.back();
}

void* rzalloc_size(void*, size_t size)
{
   allocated_pool.emplace_back(calloc(1, size));
   return allocated_pool.back();
}

void ralloc_free(void* pointer)
{
   for (auto& allocated : allocated_pool) {
      if (allocated == pointer) {
         std::swap(allocated, allocated_pool.back());
         allocated_pool.pop_back();
         free(pointer);
         break;
      }
   }
}

static std::vector<std::unordered_map<void*, hash_entry>*> hash_table_pool;

struct hash_table *_mesa_pointer_hash_table_create(void*)
{
   auto* map = new std::unordered_map<void*, hash_entry>;
   hash_table_pool.emplace_back(map);
   return (struct hash_table *)map;
}

struct hash_entry *_mesa_hash_table_search(void* table, void* key)
{
   auto& map = *(std::unordered_map<void*, hash_entry>*)table;
   auto it = map.find(key);
   if (it != map.end())
      return &(*it).second;
   return nullptr;
}

void _mesa_hash_table_insert(void* table, void* key, void* value)
{
   auto& map = *(std::unordered_map<void*, hash_entry>*)table;
   map[key] = hash_entry{ value };
}

union fi {
   float f;
   int32_t i;
   uint32_t ui;
};

/**
 * Convert a 2-byte half float to a 4-byte float.
 * Based on code from:
 * http://www.opengl.org/discussion_boards/ubb/Forum3/HTML/008786.html
 */
float
_mesa_half_to_float(uint16_t val)
{
   union fi infnan;
   union fi magic;
   union fi f32;

   infnan.ui = 0x8f << 23;
   infnan.f = 65536.0f;
   magic.ui = 0xef << 23;

   /* Exponent / Mantissa */
   f32.ui = (val & 0x7fff) << 13;

   /* Adjust */
   f32.f *= magic.f;
   /* XXX: The magic mul relies on denorms being available */

   /* Inf / NaN */
   if (f32.f >= infnan.f)
      f32.ui |= 0xff << 23;

   /* Sign */
   f32.ui |= (uint32_t)(val & 0x8000) << 16;

   return f32.f;
}

void mesa_cleanup()
{
   for (auto* pointer : allocated_pool) {
      free(pointer);
   }
   allocated_pool.clear();
   for (auto* map : hash_table_pool) {
      delete map;
   }
   hash_table_pool.clear();
}
