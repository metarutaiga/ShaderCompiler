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
