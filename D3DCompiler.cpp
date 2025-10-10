#include <string>
#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"
#include "mine/syscall/allocator.h"
#include "mine/x86/x86_i386.h"
#include "mine/x86/x86_instruction.inl"
#include "mine/x86/x86_register.inl"

namespace D3DCompiler {

size_t RunD3DAssemble(mine* cpu, size_t(*symbol)(mine*, void*, const char*))
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;

    auto profile = ShaderCompiler::DetectProfile();
    auto macro = (uint32_t*)allocator->allocate(sizeof(uint32_t) * 6);
    if (macro) {
        char major[2] = { profile.size() > 3 ? profile[3] : '1' };
        char minor[2] = { profile.size() > 5 ? profile[5] : '0' };
        macro[0] = VirtualMachine::DataToMemory("__SHADER_TARGET_MAJOR", sizeof("__SHADER_TARGET_MAJOR"), allocator);
        macro[1] = VirtualMachine::DataToMemory(major, 2, allocator);
        macro[2] = VirtualMachine::DataToMemory("__SHADER_TARGET_MINOR", sizeof("__SHADER_TARGET_MINOR"), allocator);
        macro[3] = VirtualMachine::DataToMemory(minor, 2, allocator);
        macro[4] = 0;
        macro[5] = 0;
    }

    std::string text;
    if (profile == "vs_2_a")
        text += "vs_2_x";
    else if (profile == "ps_2_a" || profile == "ps_2_b")
        text += "ps_2_x";
    else
        text += profile;
    text += ';';
    text += ShaderCompiler::text;

    size_t D3DAssemble = symbol(cpu, nullptr, "D3DAssemble");
    if (D3DAssemble) {
        auto pSrcData = VirtualMachine::DataToMemory(text.data(), text.size(), allocator);
        auto SrcDataSize = text.size();
        auto pDefines = macro ? uint32_t((char*)macro - (char*)allocator->address()) : 0;

        Push32(0);
        auto ppErrorMsgs = ESP;
        Push32(0);
        auto ppCode = ESP;
        Push32('D3DA');

        Push32(ppErrorMsgs);    // **ppErrorMsgs    optional
        Push32(ppCode);         // **ppCode
        Push32(0);              // Flags
        Push32(0);              // *pInclude        optional
        Push32(pDefines);       // *pDefines        optional
        Push32(0);              // pSourceName      optional
        Push32(SrcDataSize);    // SrcDataSize
        Push32(pSrcData);       // pSrcData

        return D3DAssemble;
    }

    size_t D3DXAssembleShader = symbol(cpu, nullptr, "D3DXAssembleShader");
    if (D3DXAssembleShader) {
        auto pSrcData = VirtualMachine::DataToMemory(text.data(), text.size(), allocator);
        auto SrcDataSize = text.size();
        auto pDefines = macro ? uint32_t((char*)macro - (char*)allocator->address()) : 0;

        Push32(0);
        auto ppErrorMsgs = ESP;
        Push32(0);
        auto ppShader = ESP;
        Push32('D3DA');

        Push32(ppErrorMsgs);    // **ppErrorMsgs    optional
        Push32(ppShader);       // **ppShader
        Push32(0);              // Flags
        Push32(0);              // pInclude         optional
        Push32(pDefines);       // *pDefines        optional
        Push32(SrcDataSize);    // SrcDataLen
        Push32(pSrcData);       // pSrcData

        return D3DXAssembleShader;
    }

    return 0;
}

size_t RunD3DCompile(mine* cpu, size_t(*symbol)(mine*, void*, const char*))
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;

    auto profile = ShaderCompiler::DetectProfile();
    auto macro = (uint32_t*)allocator->allocate(sizeof(uint32_t) * 6);
    if (macro) {
        char major[2] = { profile.size() > 3 ? profile[3] : '1' };
        char minor[2] = { profile.size() > 5 ? profile[5] : '0' };
        macro[0] = VirtualMachine::DataToMemory("__SHADER_TARGET_MAJOR", sizeof("__SHADER_TARGET_MAJOR"), allocator);
        macro[1] = VirtualMachine::DataToMemory(major, 2, allocator);
        macro[2] = VirtualMachine::DataToMemory("__SHADER_TARGET_MINOR", sizeof("__SHADER_TARGET_MINOR"), allocator);
        macro[3] = VirtualMachine::DataToMemory(minor, 2, allocator);
        macro[4] = 0;
        macro[5] = 0;
    }

    size_t D3DCompile = symbol(cpu, nullptr, "D3DCompile");
    if (D3DCompile == 0)
        D3DCompile = symbol(cpu, nullptr, "D3DCompileFromMemory");
    if (D3DCompile) {
        auto pSrcData = VirtualMachine::DataToMemory(ShaderCompiler::text.data(), ShaderCompiler::text.size(), allocator);
        auto SrcDataSize = ShaderCompiler::text.size();
        auto pDefines = macro ? uint32_t((char*)macro - (char*)allocator->address()) : 0;
        auto pEntrypoint = VirtualMachine::DataToMemory(ShaderCompiler::entry.data(), ShaderCompiler::entry.size() + 1, allocator);
        auto pTarget = VirtualMachine::DataToMemory(profile.data(), profile.size() + 1, allocator);

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
        Push32(pDefines);       // *pDefines        optional
        Push32(0);              // pSourceName      optional
        Push32(SrcDataSize);    // SrcDataSize
        Push32(pSrcData);       // pSrcData

        return D3DCompile;
    }

    size_t D3DXCompileShader = symbol(cpu, nullptr, "D3DXCompileShader");
    if (D3DXCompileShader) {
        auto pSrcData = VirtualMachine::DataToMemory(ShaderCompiler::text.data(), ShaderCompiler::text.size(), allocator);
        auto srcDataLen = ShaderCompiler::text.size();
        auto pDefines = macro ? uint32_t((char*)macro - (char*)allocator->address()) : 0;
        auto pFunctionName = VirtualMachine::DataToMemory(ShaderCompiler::entry.data(), ShaderCompiler::entry.size() + 1, allocator);
        auto pProfile = VirtualMachine::DataToMemory(profile.data(), profile.size() + 1, allocator);

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
        Push32(pDefines);       // *pDefines
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
        auto& output = ShaderCompiler::outputs[""];
        auto pSrcData = VirtualMachine::DataToMemory(output.binary.data(), output.binary.size(), allocator);
        auto SrcDataSize = output.binary.size();

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
        auto& output = ShaderCompiler::outputs[""];
        auto pShader = VirtualMachine::DataToMemory(output.binary.data(), output.binary.size(), allocator);

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

mine* NextProcess(mine* cpu)
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;
    auto* memory = (uint8_t*)allocator->address();
    auto* stack = (uint32_t*)(memory + cpu->Stack());
    switch (stack[0]) {
    case 'D3DA':
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
            auto* code = (char*)(memory + pointer);

            auto& output = ShaderCompiler::outputs[""];
            output.binary.assign(code, code + size);

            size_t address = D3DCompiler::RunD3DDisassemble(cpu, VirtualMachine::GetProcAddress);
            if (address) {
                Push32(0);
                cpu->Jump(address);
                return cpu;
            }
            Logger<CONSOLE>("Disassemble : %s\n", "Symbol is not found");
        }
        else {
            Logger<CONSOLE>("Compile : %08X\n", EAX);
        }
        break;
    }
    case 'D3DD': {
        auto* blob = (uint32_t*)(memory + stack[1]);
        if (blob && EAX == 0) {
            auto& size = blob[2];
            auto& pointer = blob[3];
            auto* code = (char*)(memory + pointer);

            auto& output = ShaderCompiler::outputs[""];
            output.disasm.assign(code, code + size);
            output.disasm.resize(strlen(output.disasm.c_str()));
        }
        else {
            Logger<CONSOLE>("Disassemble : %08X\n", EAX);
        }
        break;
    }
    default:
        break;
    }

    return nullptr;
}

};  // namespace D3DCompiler
