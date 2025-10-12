#include <string>
#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"
#include "../mine/syscall/allocator.h"
#include "../mine/x86/x86_i386.h"
#include "../mine/x86/x86_instruction.inl"
#include "../mine/x86/x86_register.inl"

namespace UnifiedExecution {

size_t RunDriver(mine* cpu, size_t(*symbol)(mine*, void*, const char*)) __attribute__((optnone))
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;

    if (ShaderCompiler::drivers.size() > ShaderCompiler::driver_index &&
        ShaderCompiler::drivers[ShaderCompiler::driver_index].machines.size() > ShaderCompiler::machine_index) {
        auto& driver = ShaderCompiler::drivers[ShaderCompiler::driver_index];
        auto& machine = driver.machines[ShaderCompiler::machine_index];
        if (machine.size() < 2)
            return 0;
        size_t SrcDataSize = 0;
        size_t pSrcData = 0;
        size_t SHDRDataSize = 0;
        size_t pSHDRData = 0;
        size_t ShaderType = 0;
        size_t OutputSize = 0;
        size_t pOutput = 0;

        size_t entry = symbol(cpu, nullptr, machine[1].c_str());
        if (entry) {
            for (size_t i = 2; i < machine.size(); ++i) {
                const char* parameter = machine[i].c_str();
                switch (parameter[0]) {
                case 0:
                    break;
                case '0' ... '9':
                    if (parameter[1] == 'x') {
                        Push32(strtol(parameter, nullptr, 16));
                        break;
                    }
                    Push32(strtol(parameter, nullptr, 10));
                    break;
                case '(': {
                    auto count = strtol(parameter + 1, nullptr, 10) - 1;
                    auto stack = ESP + sizeof(uint32_t) * count;
                    Push32(stack);
                    break;
                }
                case '\'': {
                    uint32_t type = 0;
                    for (size_t i = 1; i < 5; ++i) {
                        type <<= 8;
                        type |= uint8_t(parameter[i]);
                    }
                    Push32(type);
                    break;
                }
                case '\"': {
                    auto string = machine[i];
                    if (string.empty() == false)
                        string.erase(string.begin());
                    if (string.empty() == false)
                        string.pop_back();
                    auto pString = VirtualMachine::DataToMemory(string.data(), string.size() + 1, allocator);
                    Push32(pString);
                    break;
                }
                default:
                    if (machine[i] == "pSrcData" || machine[i] == "SrcDataSize") {
                        if (pSrcData == 0 && SrcDataSize == 0) {
                            auto& output = ShaderCompiler::outputs[""];
                            pSrcData = VirtualMachine::DataToMemory(output.binary.data(), output.binary.size(), allocator);
                            SrcDataSize = output.binary.size();
                        }
                        if (machine[i] == "pSrcData")
                            Push32(pSrcData);
                        if (machine[i] == "SrcDataSize")
                            Push32(SrcDataSize);
                    }
                    else if (machine[i] == "pSHDRData" || machine[i] == "SHDRDataSize") {
                        if (pSHDRData == 0 && SHDRDataSize == 0) {
                            auto& output = ShaderCompiler::outputs[""];
                            if (output.binary.empty() == false) {
                                uint32_t* chunks = (uint32_t*)output.binary.data();
                                size_t size = output.binary.size() / sizeof(uint32_t);
                                for (size_t i = 0; i < size; ++i) {
                                    if (chunks[i] == 'XEHS' || chunks[i] == 'RDHS') {
                                        pSHDRData = VirtualMachine::DataToMemory(chunks + i + 2, chunks[i + 1], allocator);
                                        SHDRDataSize = chunks[i + 1];
                                        break;
                                    }
                                }
                            }
                        }
                        if (machine[i] == "pSHDRData")
                            Push32(pSHDRData);
                        if (machine[i] == "SHDRDataSize")
                            Push32(SHDRDataSize);
                    }
                    else if (machine[i] == "ShaderType" || machine[i] == "shader_type") {
                        if (ShaderType == 0) {
                            std::string type;
                            if (ShaderCompiler::types.size() > ShaderCompiler::type_index) {
                                type = ShaderCompiler::types[ShaderCompiler::type_index];
                                if (machine[i] == "shader_type") {
                                    if (type == "Vertex")
                                        type = "vertex";
                                    if (type == "Pixel")
                                        type = "fragment";
                                }
                            }
                            ShaderType = VirtualMachine::DataToMemory(type.data(), type.size() + 1, allocator);
                        }
                        Push32(ShaderType);
                    }
                    else if (machine[i] == "pOutput" || machine[i] == "OutputSize") {
                        if (pOutput == 0 && OutputSize == 0) {
                            pOutput = VirtualMachine::DataToMemory("", 1048576, allocator);
                            OutputSize = 1048576;
                        }
                        if (machine[i] == "pOutput")
                            Push32(pOutput);
                        if (machine[i] == "OutputSize")
                            Push32(OutputSize);
                    }
                    else {
                        printf("%s\n", machine[i].c_str());
                    }
                    break;
                }
            }

            return entry;
        }
    }

    return 0;
}

}   // namespace UnifiedExecution
