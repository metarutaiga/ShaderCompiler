#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"
#include "../mine/syscall/allocator.h"
#include "../mine/x86/x86_i386.h"
#include "../mine/x86/x86_instruction.inl"
#include "../mine/x86/x86_register.inl"

static int inside_fprintf(FILE* fp, const char* format, ...)
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
static int inside_fputc(char c, FILE* fp)
{
    if (fp) {
        auto& string = *(std::string*)fp;
        string += c;
    }
    return 0;
}
#define fprintf inside_fprintf
#define fputc inside_fputc
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define BITFIELD_BIT(b) (1u << (b))
#define BITFIELD_MASK(b) ((b) == 32 ? (~0u) : BITFIELD_BIT((b) & 31) - 1)
#define DIV_ROUND_UP(A, B) (((A) + (B) - 1) / (B))
#define FALLTHROUGH
#define MAX2(a, b) (a > b ? a : b)
#define MIN2(a, b) (a < b ? a : b)
#define PRINTFLIKE(...)
#define STATIC_ASSERT static_assert
#define util_bitcount __builtin_popcount
#define util_sign_extend(v, w) (((int)(v) << (32 - w)) >> w)
#define util_last_bit(u) (u == 0 ? 0 : 32 - __builtin_clz(u))
#define mesa_loge(...)
#define uif(v) (*(float*)&v)
#define _mesa_half_to_float(v) (float)(*(__fp16*)&v)
#define _util_printf_format(...)
static std::vector<void*> allocated_pool;
#define ralloc_array(p, s, c) ralloc_size(p, sizeof(s) * c)
static void* ralloc_size(void*, size_t size) {
    allocated_pool.emplace_back(malloc(size));
    return allocated_pool.back();
}
static void* rzalloc_size(void*, size_t size) {
    allocated_pool.emplace_back(calloc(1, size));
    return allocated_pool.back();
}
static void ralloc_free(void* pointer) {
    for (auto& allocated : allocated_pool) {
        if (allocated == pointer) {
            std::swap(allocated, allocated_pool.back());
            allocated_pool.pop_back();
            free(pointer);
            break;
        }
    }
}
struct hash_entry { void* data; };
static std::vector<std::unordered_map<void*, hash_entry>*> hash_table_pool;
static struct hash_table *_mesa_pointer_hash_table_create(void*) {
    auto* map = new std::unordered_map<void*, hash_entry>;
    hash_table_pool.emplace_back(map);
    return (struct hash_table *)map;
}
static struct hash_entry *_mesa_hash_table_search(void* table, void* key) {
    auto& map = *(std::unordered_map<void*, hash_entry>*)table;
    auto it = map.find(key);
    if (it != map.end())
        return &(*it).second;
    return nullptr;
}
static void _mesa_hash_table_insert(void* table, void* key, void* value) {
    auto& map = *(std::unordered_map<void*, hash_entry>*)table;
    map[key] = hash_entry{ value };
}
namespace Adreno {
#   include "../disassembler/adreno/disasm-a3xx.c"
#   include "../disassembler/adreno/ir3-isa.c"
#   include "../disassembler/adreno/isaspec.c"
};

namespace QCOMCompiler {

mine* NextProcess(mine* cpu)
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;
    auto* memory = (uint8_t*)allocator->address();
    auto* stack = (uint32_t*)(memory + cpu->Stack());
    switch (stack[0]) {
    case 'QCOM': {
        auto binary = stack[1] ? (uint32_t*)(memory + stack[1]) : nullptr;
        auto binary_data_size = stack[2];
        if (binary) {
            auto size = binary_data_size;
            auto code = (char*)binary;

            std::vector<char>& binary = ShaderCompiler::outputs["Machine"].binary;
            binary.assign(code, code + size);

            std::string& disasm = ShaderCompiler::outputs["Machine"].disasm;
            if (binary.size() > 6) {
                uint32_t* datas = (uint32_t*)binary.data();
                uint32_t section_binary = datas[1];
                uint32_t section_gpu = datas[4];
                uint32_t section_offset = datas[5];
                uint32_t section_count = datas[6];

                int gpu_id = 300;
                switch (section_gpu) {
                case 0: gpu_id = 300;   break;
                case 1: gpu_id = 400;   break;
                case 2: gpu_id = 500;   break;
                case 3: gpu_id = 600;   break;
                case 4: gpu_id = 700;   break;
                }

                for (size_t i = 0; i < section_count; ++i) {
                    uint32_t* section = &datas[section_offset / sizeof(uint32_t) + i * 5];
                    uint32_t number = section[0];
                    uint32_t offset = section[1];
                    uint32_t size = section[2];
                    if (number == section_binary) {
                        Adreno::disasm_a3xx_set_debug(Adreno::PRINT_RAW);
                        Adreno::try_disasm_a3xx(&datas[offset / sizeof(uint32_t)], size / sizeof(uint32_t), 0, (FILE*)&disasm, gpu_id);
                        for (auto* pointer : allocated_pool) {
                            free(pointer);
                        }
                        allocated_pool.clear();
                        for (auto* map : hash_table_pool) {
                            delete map;
                        }
                        hash_table_pool.clear();
                        break;
                    }
                }

                for (size_t i = 0, column = 0; i < disasm.size(); ++i, ++column) {
                    switch (disasm[i]) {
                    case '\n':
                        column = -1;
                        break;
                    case '\t':
                        disasm.replace(i, 1, (8 - column % 8), ' ');
                        break;
                    }
                }
            }
        }
        if (EAX != 0) {
            Logger<CONSOLE>("Compile : %08X\n", EAX);
        }
        break;
    }
    default:
        break;
    }

    return nullptr;
}

};  // namespace QCOMCompiler
