#include <string>
#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"
#include "mine/syscall/allocator.h"
#include "mine/syscall/syscall_internal.h"
#include "mine/x86/x86_i386.h"
#include "mine/x86/x86_instruction.inl"
#include "mine/x86/x86_register.inl"

namespace D3DCompiler {

size_t RunD3DCompile(mine* cpu, size_t(*symbol)(mine*, void*, const char*))
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;

    size_t D3DCompile = symbol(cpu, nullptr, "D3DCompile");
    if (D3DCompile == 0)
        D3DCompile = symbol(cpu, nullptr, "D3DCompileFromMemory");
    if (D3DCompile) {
        auto profile = ShaderCompiler::DetectProfile();

        auto pSrcData = VirtualMachine::DataToMemory(ShaderCompiler::text.data(), ShaderCompiler::text.size(), allocator);
        auto SrcDataSize = ShaderCompiler::text.size();
        auto pEntrypoint = VirtualMachine::DataToMemory(ShaderCompiler::entry.data(), ShaderCompiler::entry.size() + 1, allocator);
        auto pTarget = VirtualMachine::DataToMemory(profile, strlen(profile) + 1, allocator);

        Push32(0);
        auto ppErrorMsgs = ESP;
        Push32(0);
        auto ppCode = ESP;
        Push32('D3DC');

        Push32(ppErrorMsgs);    // **ppErrorMsgs    optional
        Push32(ppCode);         // **ppCode
        Push32(0);              // Flags2
        Push32(1 << 12);        // Flags1
        Push32(pTarget);        // pTarget
        Push32(pEntrypoint);    // pEntrypoint      optional
        Push32(0);              // *pInclude        optional
        Push32(0);              // *pDefines        optional
        Push32(0);              // pSourceName      optional
        Push32(SrcDataSize);    // SrcDataSize
        Push32(pSrcData);       // pSrcData

        return D3DCompile;
    }

    size_t D3DXCompileShader = symbol(cpu, nullptr, "D3DXCompileShader");
    if (D3DXCompileShader) {
        auto profile = ShaderCompiler::DetectProfile();

        auto pSrcData = VirtualMachine::DataToMemory(ShaderCompiler::text.data(), ShaderCompiler::text.size(), allocator);
        auto srcDataLen = ShaderCompiler::text.size();
        auto pFunctionName = VirtualMachine::DataToMemory(ShaderCompiler::entry.data(), ShaderCompiler::entry.size() + 1, allocator);
        auto pProfile = VirtualMachine::DataToMemory(profile, strlen(profile) + 1, allocator);

        Push32(0);
        auto ppErrorMsgs = ESP;
        Push32(0);
        auto ppShader = ESP;
        Push32('D3DC');

        Push32(0);              // *ppConstantTable
        Push32(ppErrorMsgs);    // *ppErrorMsgs
        Push32(ppShader);       // *ppShader
        Push32(0);              // Flags
        Push32(pProfile);       // pProfile
        Push32(pFunctionName);  // pFunctionName
        Push32(0);              // pInclude
        Push32(0);              // *pDefines
        Push32(srcDataLen);     // srcDataLen
        Push32(pSrcData);       // pSrcData

        return D3DXCompileShader;
    }

    return 0;
}

size_t RunD3DDisassemble(mine* cpu, size_t(*symbol)(mine*, void*, const char*))
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;

    size_t D3DDisassemble = symbol(cpu, nullptr, "D3DDisassemble");
    if (D3DDisassemble == 0)
        D3DDisassemble = symbol(cpu, nullptr, "D3DDisassembleCode");
    if (D3DDisassemble) {
        auto pSrcData = VirtualMachine::DataToMemory(ShaderCompiler::binary.data(), ShaderCompiler::binary.size(), allocator);
        auto SrcDataSize = ShaderCompiler::binary.size();

        Push32(0);
        auto ppDisassembly = ESP;
        Push32('D3DD');

        Push32(ppDisassembly);  // **ppDisassembly
        Push32(0);              // szComments       optional
        Push32(0);              // Flags
        Push32(SrcDataSize);    // SrcDataSize
        Push32(pSrcData);       // pSrcData

        return D3DDisassemble;
    }

    size_t D3DXDisassembleShader = symbol(cpu, nullptr, "D3DXDisassembleShader");
    if (D3DXDisassembleShader) {
        auto pShader = VirtualMachine::DataToMemory(ShaderCompiler::binary.data(), ShaderCompiler::binary.size(), allocator);

        Push32(0);
        auto ppDisassembly = ESP;
        Push32('D3DD');

        Push32(ppDisassembly);  // *ppDisassembly
        Push32(0);              // pComments        optional
        Push32(0);              // EnableColorCode
        Push32(pShader);        // *pShader

        return D3DXDisassembleShader;
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
    case 'D3DC': {
        auto* error = (uint32_t*)(memory + stack[2]);
        if (error) {
//          auto& size = error[2];
            auto& pointer = error[3];
            auto* message = (uint32_t*)(memory + pointer);

            if (message[0]) {
                Logger<CONSOLE>("%s", message);
            }
        }
        auto* blob = (uint32_t*)(memory + stack[1]);
        if (blob && EAX == 0) {
            auto& size = blob[2];
            auto& pointer = blob[3];
            auto* code = (uint32_t*)(memory + pointer);

            std::string d3dsio;
            for (auto i = 0; i < size; i += 16) {
                char line[256];
                snprintf(line, 256, "%04X: %08X", i, code[i / 4 + 0]);
                if ((i +  4) < size) snprintf(line, 256, "%s, %08X", line, code[i / 4 + 1]);
                if ((i +  8) < size) snprintf(line, 256, "%s, %08X", line, code[i / 4 + 2]);
                if ((i + 12) < size) snprintf(line, 256, "%s, %08X", line, code[i / 4 + 3]);
                snprintf(line, 256, "%s\n", line);
                d3dsio += line;
            }
            ShaderCompiler::binaries["Binary"] = d3dsio;
            ShaderCompiler::binary.assign((char*)code, (char*)code + size);

            size_t address = D3DCompiler::RunD3DDisassemble(cpu, VirtualMachine::GetProcAddress);
            if (address) {
                Push32(0);
                cpu->Jump(address);
                return cpu;
            }
        }
        else {
            Logger<CONSOLE>("Compile : %08X", EAX);
        }
        break;
    }
    case 'D3DD': {
        auto* blob = (uint32_t*)(memory + stack[1]);
        if (blob && EAX == 0) {
            auto& size = blob[2];
            auto& pointer = blob[3];
            auto* code = (char*)(memory + pointer);

            std::string disassembly(code, code + size);
            ShaderCompiler::binaries["Disassembly"] = disassembly;
        }
        else {
            Logger<CONSOLE>("Disassemble : %08X", EAX);
        }
        break;
    }
    default:
        break;
    }

    return nullptr;
}

};  // namespace D3DCompiler
