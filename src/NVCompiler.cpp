#include <string>
#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"
#include "../mine/syscall/allocator.h"
#include "../mine/x86/x86_i386.h"
#include "../mine/x86/x86_instruction.inl"
#include "../mine/x86/x86_register.inl"

namespace NVCompiler {

mine* NextProcess(mine* cpu)
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;
    auto* memory = (uint8_t*)allocator->address();
    auto* stack = (uint32_t*)(memory + cpu->Stack());
    switch (stack[0]) {
    case 'NVDA': {
        auto* binary_blob = stack[1] ? (uint32_t*)(memory + stack[1]) : nullptr;
        auto* disasm_blob = stack[2] ? (uint32_t*)(memory + stack[2]) : nullptr;
        if ((binary_blob || disasm_blob) && EAX == 0) {
            if (binary_blob) {
                auto& size = binary_blob[2];
                auto& pointer = binary_blob[3];
                auto* code = (char*)(memory + pointer);

                std::vector<char>& binary = ShaderCompiler::outputs["Machine"].binary;
                binary.assign(code, code + size);
            }
            if (disasm_blob) {
                auto& size = disasm_blob[2];
                auto& pointer = disasm_blob[3];
                auto* code = (char*)(memory + pointer);

                std::string& disasm = ShaderCompiler::outputs["Machine"].disasm;
                disasm.assign(code, code + size);
                disasm.resize(strlen(disasm.c_str()));

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
        else {
            Logger<CONSOLE>("Compile : %08X\n", EAX);
        }
        break;
    }
    default:
        break;
    }

    return nullptr;
}

};  // namespace NVCompiler
