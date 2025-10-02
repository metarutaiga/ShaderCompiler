#include <string>
#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"
#include "mine/syscall/allocator.h"
#include "mine/x86/x86_i386.h"
#include "mine/x86/x86_instruction.inl"
#include "mine/x86/x86_register.inl"

namespace AMDCompiler {

mine* NextProcess(mine* cpu)
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;
    auto* memory = (uint8_t*)allocator->address();
    auto* stack = (uint32_t*)(memory + cpu->Stack());
    switch (stack[4 + 0]) {
    case 'AMDH':
    case 'AMDI': {
        auto* output = (char*)(memory + stack[4 + 1]);
        auto size = stack[4 + 2];
        if (size && (EAX == 0 || EAX == 1)) {
            std::string& text = ShaderCompiler::binaries["AMD"];
            text.assign(output, output + size);
            text.resize(strlen(text.c_str()));
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

};  // namespace AMDCompiler
