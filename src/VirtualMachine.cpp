#include "Logger.h"
#include "VirtualMachine.h"
#include "../mine/format/coff/pe.h"
#include "../mine/syscall/allocator.h"
#include "../mine/syscall/extend_allocator.h"
#include "../mine/syscall/syscall.h"
#include "../mine/syscall/windows/syscall_windows.h"
#include "../mine/x86/x86_i386.h"
#include "../mine/x86/x86_i686.h"
#include "../mine/x86/x86_ia32.h"
#include "../mine/x86/x86_instruction.inl"
#include "../mine/x86/x86_register.inl"

namespace VirtualMachine {

void Close(mine* cpu)
{
    if (cpu == nullptr)
        return;

    auto allocator = cpu->Allocator;
    if (allocator) {
        Logger<SYSTEM>("Peek allocated size : %zd", allocator->peek_size());
    }
    syscall_windows_delete(cpu);
    syscall_i386_delete(cpu);
    delete cpu;
}

mine* RunDLL(const std::string& dll, size_t(*parameter)(mine*, size_t(*)(mine*, void*, const char*)), bool debug)
{
    static const size_t allocator_size = 2 * 1024 * 1024;
    static const size_t stack_size = 1 * 1024 * 1024;

    mine* cpu = new x86_ia32;
    cpu->Initialize(extend_allocator<16>::construct(allocator_size), stack_size);
    cpu->Exception = RunException;

    void* image = PE::Load(dll.c_str(), [](size_t base, size_t size, void* userdata) {
        mine* cpu = (mine*)userdata;
        return cpu->Memory(base, size);
    }, cpu, Logger<SYSTEM>);
    if (image) {
        std::string file = "./" + dll.substr(dll.find_last_of("/\\") + 1);
        std::string path = dll.substr(0, dll.find_last_of("/\\") + 1);

        Syscall syscall = {
            .path = path.c_str(),
            .printf = Logger<CONSOLE>,
            .vprintf = LoggerV<CONSOLE>,
            .debugPrintf = debug ? Logger<SYSTEM> : nullptr,
            .debugVprintf = debug ? LoggerV<SYSTEM> : nullptr,
        };
        syscall_i386_new(cpu, &syscall);

        SyscallWindows syscall_windows = {
            .stack_base = allocator_size,
            .stack_limit = allocator_size - stack_size,
            .image = (size_t)((uint8_t*)image - cpu->Memory()),
            .symbol = GetSymbol,
        };
        syscall_windows_new(cpu, &syscall_windows);
        syscall_windows_import(cpu, file.c_str(), syscall_windows.image, true);

        size_t address = parameter(cpu, GetProcAddress);
        if (address) {
            auto* i386 = (x86_i386*)cpu;
            auto& x86 = i386->x86;
            Push32(0);
            cpu->Jump(address);
            size_t entry = PE::Entry(image);
            if (entry) {
                Push32(0);
                Push32(1);
                Push32(0);
                Push32(address);
                cpu->Jump(entry);
            }
        }
    }

    return cpu;
}

size_t RunException(mine* cpu, size_t index)
{
    size_t result = SIZE_MAX;
    if (result == SIZE_MAX) {
        result = syscall_windows_execute(cpu, index);
    }
    if (result == SIZE_MAX) {
        result = syscall_i386_execute(cpu, index);
    }
    return result;
}

size_t GetSymbol(const char* file, const char* name, void* symbol_data)
{
    size_t address = 0;
    if (address == 0) {
        address = syscall_windows_symbol(file, name);
    }
    if (address == 0) {
        address = syscall_i386_symbol(file, name);
    }
    return address;
}

uint32_t DataToMemory(const void* data, size_t size, struct allocator_t* allocator)
{
    auto* pointer = (char*)allocator->allocate(size);
    if (pointer) {
        auto* memory = (char*)allocator->address();
        if (data) {
            memcpy(pointer, data, size);
        }
        else {
            memset(pointer, 0, size);
        }
        return (uint32_t)(pointer - memory);
    }
    return 0;
}

size_t GetProcAddress(mine* cpu, void* image, const char* name)
{
    if (image == nullptr) {
        auto* allocator = cpu->Allocator;
        auto* memory = (char*)allocator->address();
        uint32_t stack[2] = {};
        image = memory + syscall_GetModuleHandleA(memory, stack);
    }
    struct ExportData {
        const char* name;
        size_t address;
    } export_data = { .name = name };
    PE::Exports(image, [](const char* name, size_t address, void* userdata) {
        auto export_data = (ExportData*)userdata;
        if (strcmp(export_data->name, name) == 0) {
            export_data->address = address;
            Logger<SYSTEM>("%-12s : [%08zX] %s", "Symbol", address, name);
        }
    }, &export_data);
    return export_data.address;
}

};  // namespace VirtualMachine
