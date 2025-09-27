#include <string>
#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"
#include "mine/syscall/allocator.h"
#include "mine/x86/x86_i386.h"
#include "mine/x86/x86_instruction.inl"
#include "mine/x86/x86_register.inl"

namespace NVCompiler {

size_t RunNVCompile(mine* cpu, size_t(*symbol)(mine*, void*, const char*))
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;

    size_t NVCompileShader = symbol(cpu, nullptr, "NVCompileShader");
    if (NVCompileShader) {
        std::string gpu;
        if (ShaderCompiler::machines.size() > ShaderCompiler::machine_index)
            gpu = ShaderCompiler::machines[ShaderCompiler::machine_index];

        std::string version = "2.07.0804.1530";
        if (gpu.size() > 2 && gpu[2] == '3')
            version = "2.01.10000.0305";

        auto pSrcData = VirtualMachine::DataToMemory(ShaderCompiler::binary.data(), ShaderCompiler::binary.size(), allocator);
        auto SrcDataSize = ShaderCompiler::binary.size();
        auto pVersion = VirtualMachine::DataToMemory(version.data(), version.size() + 1, allocator);
        auto pGPU = VirtualMachine::DataToMemory(gpu.data(), gpu.size() + 1, allocator);

        Push32(0);
        auto ppCode = ESP;
        Push32('NVDA');

        Push32(ppCode);         // **ppCode
        Push32(pGPU);           // pGPU
        Push32(pVersion);       // pVersion
        Push32(SrcDataSize);    // SrcDataSize
        Push32(pSrcData);       // pSrcData

        return NVCompileShader;
    }

    return 0;
}

mine* RunNextProcess(mine* cpu)
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;
    auto* memory = (uint8_t*)allocator->address();
    auto* stack = (uint32_t*)(memory + cpu->Stack());
    switch (stack[0]) {
    case 'NVDA': {
        auto* blob = (uint32_t*)(memory + stack[1]);
        if (blob && EAX == 0) {
            auto& size = blob[2];
            auto& pointer = blob[3];
            auto* code = (char*)(memory + pointer);

            std::string& text = ShaderCompiler::binaries["NVIDIA"];
            text.assign(code, code + size);

            for (size_t i = 0, column = 0; i < text.size(); ++i, ++column) {
                switch (text[i]) {
                case '\n':
                    column = 0;
                    break;
                case '\t':
                    text.replace(i, 1, (8 - column % 8), ' ');
                    break;
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
