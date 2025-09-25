#pragma once

#include "mine/mine.h"

namespace VirtualMachine {

mine* RunDLL(const char* dll, size_t(*parameter)(mine*, size_t(*)(const char*, void*), void*));
size_t RunException(mine* data, size_t index);
size_t GetSymbol(const char* file, const char* name, void* symbol_data);
uint32_t DataToMemory(const void* data, size_t size, struct allocator_t* allocator);

};  // namespace VirtualMachine
