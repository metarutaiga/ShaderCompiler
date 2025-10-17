#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"
#include "../mine/syscall/allocator.h"
#include "../mine/x86/x86_i386.h"
#include "../mine/x86/x86_instruction.inl"
#include "../mine/x86/x86_register.inl"

extern "C" {
    void mesa_cleanup();
#   include "../disassembler/adreno/disasm.h"
};

namespace QCOMCompiler {

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
                        disasm_a3xx_set_debug(PRINT_RAW);
                        try_disasm_a3xx(&datas[offset / sizeof(uint32_t)], size / sizeof(uint32_t), 0, (FILE*)&disasm, gpu_id);
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
