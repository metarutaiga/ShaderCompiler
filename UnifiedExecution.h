#pragma once

struct mine;

namespace UnifiedExecution {

size_t RunDriver(mine* cpu, size_t(*symbol)(mine*, void*, const char*));

};  // namespace UnifiedExecution
