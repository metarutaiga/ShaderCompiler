#include <string>
#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"
#include "../mine/syscall/allocator.h"
#include "../mine/x86/x86_i386.h"
#include "../mine/x86/x86_instruction.inl"
#include "../mine/x86/x86_register.inl"

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
