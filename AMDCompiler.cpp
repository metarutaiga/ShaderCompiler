#include <string>
#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"
#include "mine/syscall/allocator.h"
#include "mine/x86/x86_i386.h"
#include "mine/x86/x86_instruction.inl"
#include "mine/x86/x86_register.inl"

namespace AMDCompiler {

size_t RunAMDCompile(mine* cpu, size_t(*symbol)(mine*, void*, const char*))
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;

    std::string gpu;
    if (ShaderCompiler::machines.size() > ShaderCompiler::machine_index)
        gpu = ShaderCompiler::machines[ShaderCompiler::machine_index];

    if (gpu.find("R") == 0) {
        size_t GSA_DisassembleHWShader = symbol(cpu, nullptr, "GSA_DisassembleHWShader");
        if (GSA_DisassembleHWShader) {
            auto pSrcData = VirtualMachine::DataToMemory(ShaderCompiler::binary.data(), ShaderCompiler::binary.size(), allocator);
            auto SrcDataSize = ShaderCompiler::binary.size();
            auto pOutput = VirtualMachine::DataToMemory(nullptr, 1048576, allocator);

            uint32_t Chip = 32;
            if (gpu.find("R600") == 0)          Chip = 32;
            else if (gpu.find("RV610") == 0)    Chip = 33;
            else if (gpu.find("RV630") == 0)    Chip = 34;
            else if (gpu.find("RV670") == 0)    Chip = 35;
            else if (gpu.find("RV770") == 0)    Chip = 36;
            else if (gpu.find("RV730") == 0)    Chip = 37;
            else if (gpu.find("RV710") == 0)    Chip = 38;
            else if (gpu.find("RV740") == 0)    Chip = 39;
//          else if (gpu.find("Cypress") == 0)  Chip = 41;
//          else if (gpu.find("Juniper") == 0)  Chip = 42;
//          else if (gpu.find("Redwood") == 0)  Chip = 43;
//          else if (gpu.find("Cedar") == 0)    Chip = 44;
            else if (gpu.find("RV870") == 0)    Chip = 41;
            else if (gpu.find("RV840") == 0)    Chip = 42;
            else if (gpu.find("RV830") == 0)    Chip = 43;
            else if (gpu.find("RV810") == 0)    Chip = 44;
//          else if (gpu.find("Cayman") == 0)   Chip = 48;
//          else if (gpu.find("Barts") == 0)    Chip = 49;
//          else if (gpu.find("Turks") == 0)    Chip = 50;
//          else if (gpu.find("Caicos") == 0)   Chip = 51;
            else if (gpu.find("RV970") == 0)    Chip = 48;
            else if (gpu.find("RV940") == 0)    Chip = 49;
            else if (gpu.find("RV930") == 0)    Chip = 50;
            else if (gpu.find("RV910") == 0)    Chip = 51;

            Push32(0);
            Push32(0);
            Push32(0);
            Push32(0);
            Push32(0);
            Push32(0);
            Push32(SrcDataSize);
            Push32(pSrcData);
            Push32(0x121);
            auto pGSA = ESP;

            Push32(1048576);
            auto pSize = ESP;
            Push32(pOutput);
            Push32('AMDH');

            Push32(pSize);      // pSize
            Push32(pOutput);    // pOutput
            Push32(Chip);       // GPU
            Push32(pGSA);       // pGSA

            return GSA_DisassembleHWShader;
        }
    }
    else if (gpu.find("AMDIL") == 0) {
        size_t GSA_DisassembleILShader = symbol(cpu, nullptr, "GSA_DisassembleILShader");
        if (GSA_DisassembleILShader) {
            auto pSrcData = VirtualMachine::DataToMemory(ShaderCompiler::binary.data(), ShaderCompiler::binary.size(), allocator);
            auto SrcDataSize = ShaderCompiler::binary.size();
            auto pOutput = VirtualMachine::DataToMemory(nullptr, 1048576, allocator);

            Push32(0);
            Push32(0);
            Push32(0);
            Push32(0);
            Push32(0);
            Push32(0);
            Push32(SrcDataSize);
            Push32(pSrcData);
            Push32(0x121);
            auto pGSA = ESP;

            Push32(1048576);
            auto pSize = ESP;
            Push32(pOutput);
            Push32('AMDI');

            Push32(0);          // Padding
            Push32(pSize);      // pSize
            Push32(pOutput);    // pOutput
            Push32(pGSA);       // pGSA

            return GSA_DisassembleILShader;
        }
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
    switch (stack[4 + 0]) {
    case 'AMDH':
    case 'AMDI': {
        auto* output = (char*)(memory + stack[4 + 1]);
        auto size = stack[4 + 2];
        if (size && (EAX == 0 || EAX == 1)) {
            std::string& text = ShaderCompiler::binaries["AMD"];
            text.assign(output, output + size);
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
