#include <string>
#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"
#include "../mine/syscall/allocator.h"
#include "../mine/x86/x86_i386.h"
#include "../mine/x86/x86_instruction.inl"
#include "../mine/x86/x86_register.inl"

namespace ATICompiler {

mine* NextProcess(mine* cpu)
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;
    auto* memory = (uint8_t*)allocator->address();
    auto* stack = (uint32_t*)(memory + cpu->Stack());
    if ((ESP + sizeof(uint32_t) * 2) >= x86.memory_size)
        return nullptr;

    switch (stack[0]) {
    case 'R200': {
        auto binary = stack[1] ? (uint32_t*)(memory + stack[1]) : nullptr;
        auto binary_data_size = stack[2];
        if (binary) {
            auto size = binary_data_size;
            auto code = (char*)binary;

            std::vector<char>& binary = ShaderCompiler::outputs["Machine"].binary;
            binary.assign(code, code + size);
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

};  // namespace ATICompiler
