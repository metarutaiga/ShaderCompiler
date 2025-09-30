#pragma once

struct mine;

namespace AMDCompiler {

size_t RunAMDCompile(mine* cpu, size_t(*symbol)(mine*, void*, const char*));
mine* RunNextProcess(mine* cpu);

};  // namespace AMDCompiler
