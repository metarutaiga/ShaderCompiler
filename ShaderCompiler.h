#pragma once

#include <map>
#include <string>
#include <vector>

namespace ShaderCompiler {

extern std::string shader_path;
extern std::string compiler_path;
extern std::string driver_path;

extern std::string text;

extern std::vector<std::string> shaders;
extern int shader_index;

extern std::vector<std::string> compilers;
extern int compiler_index;

extern std::string entry;

extern std::vector<std::string> profiles;
extern int profile_index;

struct Driver {
    std::vector<std::string> name;
    std::vector<std::vector<std::string>> machines;
};
extern std::vector<Driver> drivers;
extern int driver_index;
extern int machine_index;

extern std::map<std::string, std::string> binaries;
extern std::vector<char> binary;

extern std::string DetectProfile();

};  // namespace ShaderCompiler
