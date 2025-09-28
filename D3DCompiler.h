#pragma once

struct mine;

namespace D3DCompiler {

size_t RunD3DAssemble(mine* cpu, size_t(*symbol)(mine*, void*, const char*));
size_t RunD3DCompile(mine* cpu, size_t(*symbol)(mine*, void*, const char*));
size_t RunD3DDisassemble(mine* cpu, size_t(*symbol)(mine*, void*, const char*));
mine* RunNextProcess(mine* cpu);

};  // namespace D3DCompiler
