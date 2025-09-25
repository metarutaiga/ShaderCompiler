#include "Logger.h"
#include "VirtualMachine.h"
#include "mine/format/coff/pe.h"
#include "mine/syscall/simple_allocator.h"
#include "mine/syscall/syscall.h"
#include "mine/syscall/windows/syscall_windows.h"
#include "mine/x86/x86_i386.h"
#include "mine/x86/x86_instruction.inl"
#include "mine/x86/x86_register.inl"

namespace VirtualMachine {

mine* RunDLL(const char* dll, size_t(*parameter)(mine*, size_t(*)(const char*, void*), void*))
{
    static const int allocator_size = 256 * 1024 * 1024;
    static const int stack_size = 1 * 1024 * 1024;

    mine* cpu = new x86_i386;
    cpu->Initialize(simple_allocator<16>::construct(allocator_size), stack_size);
    cpu->Exception = RunException;

    void* image = PE::Load(dll, [](size_t base, size_t size, void* userdata) {
        mine* cpu = (mine*)userdata;
        return cpu->Memory(base, size);
    }, cpu, Logger<SYSTEM>);
    if (image) {
        Syscall syscall = {
            .path = ".",
            .printf = Logger<CONSOLE>,
            .vprintf = LoggerV<CONSOLE>,
            .debugPrintf = Logger<SYSTEM>,
            .debugVprintf = LoggerV<SYSTEM>,
        };
        syscall_i386_new(cpu, &syscall);

        SyscallWindows syscall_windows = {
            .stack_base = allocator_size,
            .stack_limit = allocator_size - stack_size,
            .image = image,
            .symbol = GetSymbol,
        };
        syscall_windows_new(cpu, &syscall_windows);
        syscall_windows_import(cpu, "main", image, true);

        auto symbol = [](const char* name, void* image) {
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
        };

        size_t address = parameter(cpu, symbol, image);
        if (address) {
            auto* i386 = (x86_i386*)cpu;
            auto& x86 = i386->x86;
            Push32(0);
            cpu->Jump(address);
            size_t entry = PE::Entry(image);
            if (entry) {
                Push32(0);
                Push32(2);
                Push32(0);
                Push32(address);
                Push32(0);
                Push32(1);
                Push32(0);
                Push32(entry);
                cpu->Jump(entry);
            }
        }
    }

    return cpu;
}

size_t RunException(mine* data, size_t index)
{
    size_t result = 0;
    if (result == 0) {
        result = syscall_windows_execute(data, index);
    }
    if (result == 0) {
        result = syscall_i386_execute(data, index);
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
    auto* memory = (char*)allocator->address();
    auto* pointer = (char*)allocator->allocate(size);
    if (pointer) {
        memcpy(pointer, data, size);
        return (uint32_t)(pointer - memory);
    }
    return 0;
}

};  // namespace VirtualMachine
