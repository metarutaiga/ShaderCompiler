#pragma once

struct mine;

namespace NVCompiler {

size_t RunNVCompile(mine* cpu, size_t(*symbol)(mine*, void*, const char*));
mine* RunNextProcess(mine* cpu);

};  // namespace D3DCompiler
