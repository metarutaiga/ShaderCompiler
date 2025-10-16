#include <math.h>
#include <string>
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
static int inside_fputs(const char* str, FILE* fp)
{
    if (fp) {
        auto& string = *(std::string*)fp;
        string += str;
    }
    return 0;
}
#define fprintf inside_fprintf
#define fputs inside_fputs
#define MAX2(a, b) (a > b ? a : b)
#define MIN2(a, b) (a < b ? a : b)
#define NDEBUG
#define unreachable(...)
#define UNUSED __attribute__((unused))
#define util_bitcount __builtin_popcount
#define util_logbase2(n) ((sizeof(unsigned) * 8 - 1) - __builtin_clz(n | 1))
#define util_sign_extend(v, w) (((int)(v) << (32 - w)) >> w)
#define _mesa_half_to_float(v) (float)(*(__fp16*)&v)
#include <assert.h>
#pragma clang diagnostic ignored "-Winitializer-overrides"
namespace utgard {
#   include "../disassembler/utgard/gp/disasm.c"
#   include "../disassembler/utgard/pp/disasm.c"
};
namespace midgard {
#   include "../disassembler/midgard/disassemble.c"
#   include "../disassembler/midgard/midgard_ops.c"
#   include "../disassembler/midgard/midgard_print_constant.c"
};

namespace MaliCompiler {

mine* NextProcess(mine* cpu)
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;
    auto* memory = (uint8_t*)allocator->address();
    auto* stack = (uint32_t*)(memory + cpu->Stack());
    switch (stack[2]) {
    case 'MALI': {
        auto binary_data_size = stack[5];
        auto binary = stack[6] ? (uint32_t*)(memory + stack[6]) : nullptr;
        auto number_of_errors = stack[7];
        auto errors = stack[8] ? (uint32_t*)(memory + stack[8]) : nullptr;
        auto number_of_warnings = stack[9];
        auto warnings = stack[10] ? (uint32_t*)(memory + stack[10]) : nullptr;
        if (binary) {
            auto size = binary_data_size;
            auto code = (char*)binary;

            std::vector<char>& binary = ShaderCompiler::outputs["Machine"].binary;
            binary.assign(code, code + size);

            std::string& disasm = ShaderCompiler::outputs["Machine"].disasm;
            uint32_t* chunks = (uint32_t*)binary.data();
            int type = 0;
            for (size_t i = 0, size = binary.size() / 4; i < size; ++i) {
                if (chunks[i] == __builtin_bswap32('CVER')) {
                    type = 'vert';
                    continue;
                }
                if (chunks[i] == __builtin_bswap32('CFRA')) {
                    type = 'frag';
                    continue;
                }
                if (chunks[i] == __builtin_bswap32('DBIN')) {
                    int size = chunks[i + 1];
                    uint32_t* bin = chunks + i + 2;
                    if (type == 'vert') {
                        utgard::gpir_codegen_instr* instr = (utgard::gpir_codegen_instr *)bin;
                        utgard::gpir_disassemble_program(instr, size / sizeof(utgard::gpir_codegen_instr), (FILE*)&disasm);
                    }
                    else if (type == 'frag') {
                        size >>= 2;
                        uint32_t offset = 0;
                        do {
                            utgard::ppir_codegen_ctrl *ctrl = (utgard::ppir_codegen_ctrl *)bin;
                            inside_fprintf((FILE*)&disasm, "@%6d: ", offset);
                            utgard::ppir_disassemble_instr(bin, offset, (FILE*)&disasm);
                            bin += ctrl->count;
                            offset += ctrl->count;
                            size -= ctrl->count;
                        } while (size > 0);
                    }
                    break;
                }
                if (chunks[i] == __builtin_bswap32('OBJC')) {
                    int size = chunks[i + 1];
                    uint32_t* bin = chunks + i + 2;
                    midgard::disassemble_midgard((FILE*)&disasm, bin, size, 0, false);
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
        for (size_t i = 0; i < number_of_errors; ++i) {
            auto error = errors[i] ? (char*)(memory + errors[i]) : nullptr;
            if (error) {
                Logger<CONSOLE>("Error : %s\n", error);
            }
        }
        for (size_t i = 0; i < number_of_warnings; ++i) {
            auto warning = warnings[i] ? (char*)(memory + warnings[i]) : nullptr;
            if (warning) {
                Logger<CONSOLE>("Warning : %s\n", warning);
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

};  // namespace MaliCompiler
