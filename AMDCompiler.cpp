#include <string>
#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"
#include "mine/syscall/allocator.h"
#include "mine/x86/x86_i386.h"
#include "mine/x86/x86_instruction.inl"
#include "mine/x86/x86_register.inl"

namespace AMDCompiler {

struct Elf32_Ehdr {
    uint8_t     e_ident[16];
    uint16_t    e_type;
    uint16_t    e_machine;
    uint32_t    e_version;
    uint32_t    e_entry;
    uint32_t    e_phoff;
    uint32_t    e_shoff;
    uint32_t    e_flags;
    uint16_t    e_ehsize;
    uint16_t    e_phentsize;
    uint16_t    e_phnum;
    uint16_t    e_shentsize;
    uint16_t    e_shnum;
    uint16_t    e_shstrndx;
};

struct Elf32_Shdr {
    uint32_t    sh_name;
    uint32_t    sh_type;
    uint32_t    sh_flags;
    uint32_t    sh_addr;
    uint32_t    sh_offset;
    uint32_t    sh_size;
    uint32_t    sh_link;
    uint32_t    sh_info;
    uint32_t    sh_addralign;
    uint32_t    sh_entsize;
};

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
            std::string& disasm = ShaderCompiler::outputs["Machine"].disasm;
            disasm.assign(output, output + size);
            disasm.resize(strlen(disasm.c_str()));
        }
        else {
            Logger<CONSOLE>("Compile : %08X\n", EAX);
        }
        break;
    }
    case 'AMDD': {
        auto* output = (char*)(memory + stack[4 + 2]);
        auto size = stack[4 + 3];
        if (size && EAX == 0) {
            std::vector<char>& binary = ShaderCompiler::outputs["Machine"].binary;
            binary.assign(output, output + size);

            uint32_t elf = 0;
            memcpy(&elf, output, 4);
            if (elf == 0x464C457F) {
                auto& header = *(Elf32_Ehdr*)output;
                auto* sections = (Elf32_Shdr*)(output + header.e_shoff);
                const char* shstrtab = output + sections[header.e_shstrndx].sh_offset;
                for (uint16_t i = 0; i < header.e_shnum; ++i) {
                    if (strcmp(shstrtab + sections[i].sh_name, ".disassembly") == 0) {
                        auto sh_offset = sections[i].sh_offset;
                        auto sh_size = sections[i].sh_size;

                        std::string& disasm = ShaderCompiler::outputs["Machine"].disasm;
                        disasm.assign(output + sh_offset, output + sh_offset + sh_size);
                        disasm.resize(strlen(disasm.c_str()));
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

};  // namespace AMDCompiler
