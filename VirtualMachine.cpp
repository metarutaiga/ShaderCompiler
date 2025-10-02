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

static int silent_logger(const char* format, ...)
{
    return 0;
}

static int silent_vlogger(const char* format, va_list)
{
    return 0;
}

void Close(mine* cpu)
{
    if (cpu == nullptr)
        return;

    syscall_windows_delete(cpu);
    syscall_i386_delete(cpu);
    delete cpu;
}

mine* RunDLL(const std::string& dll, size_t(*parameter)(mine*, size_t(*)(mine*, void*, const char*)), bool debug)
{
    static const int allocator_size = 256 * 1024 * 1024;
    static const int stack_size = 1 * 1024 * 1024;

    mine* cpu = new x86_i386;
    cpu->Initialize(simple_allocator<16>::construct(allocator_size), stack_size);
    cpu->Exception = RunException;

    void* image = PE::Load(dll.c_str(), [](size_t base, size_t size, void* userdata) {
        mine* cpu = (mine*)userdata;
        return cpu->Memory(base, size);
    }, cpu, Logger<SYSTEM>);
    if (image) {
        std::string path = dll.substr(0, dll.find_last_of("/\\") + 1);

        Syscall syscall = {
            .path = path.c_str(),
            .printf = Logger<CONSOLE>,
            .vprintf = LoggerV<CONSOLE>,
            .debugPrintf = debug ? Logger<SYSTEM> : silent_logger,
            .debugVprintf = debug ? LoggerV<SYSTEM> : silent_vlogger,
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

        size_t address = parameter(cpu, GetProcAddress);
        if (address) {
            auto* i386 = (x86_i386*)cpu;
            auto& x86 = i386->x86;
            Push32(0);
            cpu->Jump(address);
            size_t entry = PE::Entry(image);
            if (entry) {
#if 1
                Push32(0);
                Push32(1);
                Push32(0);
                Push32(address);
#else
                Push32(0);
                Push32(2);
                Push32(0);
                Push32(address);
                Push32(0);
                Push32(1);
                Push32(0);
                Push32(entry);
#endif
                cpu->Jump(entry);
            }
        }
    }

    return cpu;
}

size_t RunException(mine* cpu, size_t index)
{
    size_t result = 0;
    if (result == 0) {
        result = syscall_windows_execute(cpu, index);
    }
    if (result == 0) {
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
    auto* memory = (char*)allocator->address();
    auto* pointer = (char*)allocator->allocate(size);
    if (pointer) {
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
