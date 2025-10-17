#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"
#include "../mine/syscall/allocator.h"
#include "../mine/x86/x86_i386.h"
#include "../mine/x86/x86_instruction.inl"
#include "../mine/x86/x86_register.inl"

extern "C" {
    int mesa_fprintf(FILE* fp, const char* format, ...);
    void mesa_cleanup();
#   include "../disassembler/utgard/gp/codegen.h"
#   include "../disassembler/utgard/pp/codegen.h"
#   include "../disassembler/midgard/disassemble.h"
};

namespace MaliCompiler {

mine* NextProcess(mine* cpu)
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;
    auto* memory = (uint8_t*)allocator->address();
    auto* stack = (uint32_t*)(memory + cpu->Stack());
    if ((ESP + sizeof(uint32_t) * 10) >= x86.memory_size)
        return nullptr;

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
                        gpir_codegen_instr* instr = (gpir_codegen_instr *)bin;
                        gpir_disassemble_program(instr, size / sizeof(gpir_codegen_instr), (FILE*)&disasm);
                    }
                    else if (type == 'frag') {
                        size >>= 2;
                        uint32_t offset = 0;
                        do {
                            ppir_codegen_ctrl *ctrl = (ppir_codegen_ctrl *)bin;
                            mesa_fprintf((FILE*)&disasm, "@%6d: ", offset);
                            ppir_disassemble_instr(bin, offset, (FILE*)&disasm);
                            bin += ctrl->count;
                            offset += ctrl->count;
                            size -= ctrl->count;
                        } while (size > 0);
                    }
                    mesa_cleanup();
                    break;
                }
                if (chunks[i] == __builtin_bswap32('OBJC')) {
                    int size = chunks[i + 1];
                    uint32_t* bin = chunks + i + 2;
                    disassemble_midgard((FILE*)&disasm, bin, size, 0, false);
                    mesa_cleanup();
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
