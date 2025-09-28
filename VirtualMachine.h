#pragma once

#include "mine/mine.h"

namespace VirtualMachine {

void Close(mine* cpu);
mine* RunDLL(const std::string& dll, size_t(*parameter)(mine*, size_t(*)(mine*, void*, const char*)), bool debug);
size_t RunException(mine* cpu, size_t index);
size_t GetSymbol(const char* file, const char* name, void* symbol_data);
uint32_t DataToMemory(const void* data, size_t size, struct allocator_t* allocator);
size_t GetProcAddress(mine* cpu, void* image, const char* name);

};  // namespace VirtualMachine
