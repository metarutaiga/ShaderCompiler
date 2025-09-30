#define IMGUI_DEFINE_MATH_OPERATORS
#if defined(_WIN32)
#else
#include <unistd.h>
#include <sys/dir.h>
#include <mach-o/dyld.h>
#endif
#include <algorithm>
#include <chrono>
#include <map>
#include <vector>
#include "AMDCompiler.h"
#include "D3DCompiler.h"
#include "ImGuiHelper.h"
#include "Logger.h"
#include "NVCompiler.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"

static bool refresh_compiler;
static bool refresh_machine;
static std::chrono::system_clock::time_point begin_execute;

static ImGuiID binary_dockid;
static mine* cpu;
static bool debug_vm;

using namespace ShaderCompiler;
namespace ShaderCompiler {

std::string shader_path;
std::string compiler_path;
std::string machine_path;

std::string text;
std::vector<std::string> shaders;
int shader_index;
std::vector<std::string> compilers;
int compiler_index;
std::string entry;
std::vector<std::string> profiles;
int profile_index;
std::vector<std::string> machines;
int machine_index;

std::map<std::string, std::string> binaries;
std::vector<char> binary;
int binary_index;

std::string DetectProfile()
{
    int type = 'vert';
    if (text.find("ps_") != std::string::npos ||
        text.find("):COLOR") != std::string::npos ||
        text.find("): COLOR") != std::string::npos ||
        text.find(") : COLOR") != std::string::npos) {
        type = 'frag';
    }
    if (profiles.size() > profile_index) {
        std::string profile = profiles[profile_index];
        if (profile.find("1.0") != std::string::npos)   return (type == 'vert') ? "vs_1_0" : "ps_1_0";
        if (profile.find("1.1") != std::string::npos)   return (type == 'vert') ? "vs_1_1" : "ps_1_1";
        if (profile.find("1.2") != std::string::npos)   return (type == 'vert') ? "vs_1_1" : "ps_1_2";
        if (profile.find("1.3") != std::string::npos)   return (type == 'vert') ? "vs_1_1" : "ps_1_3";
        if (profile.find("1.4") != std::string::npos)   return (type == 'vert') ? "vs_1_1" : "ps_1_4";
        if (profile.find("2.0") != std::string::npos)   return (type == 'vert') ? "vs_2_0" : "ps_2_0";
        if (profile.find("2.A") != std::string::npos)   return (type == 'vert') ? "vs_2_a" : "ps_2_a";
        if (profile.find("2.B") != std::string::npos)   return (type == 'vert') ? "vs_2_0" : "ps_2_b";
        if (profile.find("3.0") != std::string::npos)   return (type == 'vert') ? "vs_3_0" : "ps_3_0";
        if (profile.find("4.0") != std::string::npos)   return (type == 'vert') ? "vs_4_0" : "ps_4_0";
        if (profile.find("4.1") != std::string::npos)   return (type == 'vert') ? "vs_4_1" : "ps_4_1";
        if (profile.find("5.0") != std::string::npos)   return (type == 'vert') ? "vs_5_0" : "ps_5_0";
    }
    return "ps_1_0";
}

};  // namespace ShaderCompiler

static void LoadCompiler()
{
    profiles.clear();
    if (compilers.size() > compiler_index) {
        std::string compiler = compilers[compiler_index];
        if (strncasecmp(compiler.data(), "d3dx9", 5) == 0 ||
            strncasecmp(compiler.data(), "d3dcompiler", 11) == 0) {
            profiles.push_back("Shader Model 1.0");
            profiles.push_back("Shader Model 1.1");
            profiles.push_back("Shader Model 1.2");
            profiles.push_back("Shader Model 1.3");
            profiles.push_back("Shader Model 1.4");
            profiles.push_back("Shader Model 2.0");
            profiles.push_back("Shader Model 2.A");
            profiles.push_back("Shader Model 2.B");
            profiles.push_back("Shader Model 3.0");
            profiles.push_back("Shader Model 4.0");
            profiles.push_back("Shader Model 4.1");
            profiles.push_back("Shader Model 5.0");
//          profiles.push_back("Shader Model 5.1");
//          profiles.push_back("Shader Model 6.0");
//          profiles.push_back("Shader Model 6.1");
//          profiles.push_back("Shader Model 6.2");
//          profiles.push_back("Shader Model 6.3");
//          profiles.push_back("Shader Model 6.4");
//          profiles.push_back("Shader Model 6.5");
//          profiles.push_back("Shader Model 6.6");
//          profiles.push_back("Shader Model 6.7");
//          profiles.push_back("Shader Model 6.8");
        }
    }
}

static void LoadShader()
{
    text.clear();
    if (shaders.size() > shader_index) {
        std::string path = shader_path + "/" + shaders[shader_index];
        FILE* file = fopen(path.c_str(), "rb");
        if (file) {
            std::string context;
            fseek(file, 0, SEEK_END);
            context.resize(ftell(file));
            fseek(file, 0, SEEK_SET);
            context.resize(fread(context.data(), 1, context.size(), file));
            fclose(file);
            text = context;
        }
    }
}

static void Text()
{
    if (ImGui::Begin("Text")) {
        ImVec2 region = ImGui::GetContentRegionAvail();
        if (ImGui::InputTextMultiline("##100", text, region)) {
            refresh_compiler = true;
            refresh_machine = true;
        }
    }
    ImGui::End();
}

static void Option()
{
    if (ImGui::Begin("Option")) {
        ImVec2 region = ImGui::GetContentRegionAvail();

        // Shader
        ImGui::TextUnformatted("Shader");
        ImGui::SetNextItemWidth(region.x);
        if (ImGui::Combo("##200", &shader_index, [](void* user_data, int index) {
            auto* shaders = (std::string*)user_data;
            return shaders[index].c_str();
        }, shaders.data(), (int)shaders.size()) || ImGui::ScrollCombo(&shader_index, shaders.size())) {
            LoadShader();
            refresh_compiler = true;
            refresh_machine = true;
        }

        // Compiler
        ImGui::NewLine();
        ImGui::TextUnformatted("Compiler");
        ImGui::SetNextItemWidth(region.x);
        if (ImGui::Combo("##201", &compiler_index, [](void* user_data, int index) {
            auto* compilers = (std::string*)user_data;
            return compilers[index].c_str();
        }, compilers.data(), (int)compilers.size()) || ImGui::ScrollCombo(&compiler_index, compilers.size())) {
            LoadCompiler();
            refresh_compiler = true;
            refresh_machine = true;
        }

        // Entry
        ImGui::TextUnformatted("Entry");
        if (ImGui::InputTextEx("##202", nullptr, entry, ImVec2(region.x, 0))) {
            refresh_compiler = true;
            refresh_machine = true;
        }

        // Profile
        ImGui::TextUnformatted("Profile");
        ImGui::SetNextItemWidth(region.x);
        if (ImGui::Combo("##203", &profile_index, [](void* user_data, int index) {
            auto* profiles = (std::string*)user_data;
            return profiles[index].c_str();
        }, profiles.data(), (int)profiles.size()) || ImGui::ScrollCombo(&profile_index, profiles.size())) {
            refresh_compiler = true;
            refresh_machine = true;
        }

        // Machine
        ImGui::NewLine();
        ImGui::TextUnformatted("Machine");
        ImGui::SetNextItemWidth(region.x);
        if (ImGui::Combo("##204", &machine_index, [](void* user_data, int index) {
            auto* machines = (std::string*)user_data;
            return machines[index].c_str();
        }, machines.data(), (int)machines.size()) || ImGui::ScrollCombo(&machine_index, machines.size())) {
            refresh_machine = true;
        }

        // Debug
        ImGui::NewLine();
        ImGui::Checkbox("Debug Virtual Machine", &debug_vm);
    }
    ImGui::End();
}

static void Binary()
{
    int id = 300;

    if (ImGui::Begin("Binary", nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImVec2 region = ImGui::GetContentRegionAvail();
        ImGui::SetNextWindowSize(region);
        ImGui::PushID(id++);
        ImGui::ListBox("", &binary_index, [](void* user_data, int index) -> const char* {
            auto& binary = *(std::vector<char>*)user_data;
            auto code = (uint32_t*)binary.data();
            auto size = binary.size();
            auto i = index * 16;

            static char line[256];
            int width = (4 + 10 + 10 + 10 + 10 + 1);
            int offset = snprintf(line, 256, "%04X", i);
            for (int j = 0; j < 16; j += 4) {
                if ((i + j) >= size)
                    break;
                char c = (j == 0) ? ':' : ',';
                offset += snprintf(line + offset, 256 - offset, "%c %08X", c, code[(i + j) / 4]);
            }
            if (width > offset) {
                memset(line + offset, ' ', width - offset);
            }

            for (int j = 0; j < 16; ++j) {
                if ((i + j) >= size)
                    break;
                char c = binary[i + j];
                line[width + j] = (c >= 0x20 && c <= 0x7E) ? c : ' ';
            }
            line[width + 16] = 0;

            return line;
        }, &binary, (int)(binary.size() + 15) / 16);
        ImGui::PopID();
    }
    ImGui::End();

    for (auto& [title, binary] : binaries) {
        ImGui::DockBuilderDockWindow(title.c_str(), binary_dockid);
        if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
            ImVec2 region = ImGui::GetContentRegionAvail();
            ImGui::PushID(id++);
            ImGui::InputTextMultiline("", binary, region, ImGuiInputTextFlags_ReadOnly);
            ImGui::PopID();
        }
        ImGui::End();
    }
}

static void System()
{
    if (ImGui::Begin("System")) {
        ImVec2 region = ImGui::GetContentRegionAvail();
        ImGui::SetNextWindowSize(region);
        ImGui::ListBox("##400", &logs_index[SYSTEM], &logs_focus[SYSTEM], [](void* user_data, int index) {
            auto* logs = (std::string*)user_data;
            return logs[index].c_str();
        }, logs[SYSTEM].data(), (int)logs[SYSTEM].size());
        logs_focus[SYSTEM] = -1;
    }
    ImGui::End();
}

static void Console()
{
    if (ImGui::Begin("Console")) {
        ImVec2 region = ImGui::GetContentRegionAvail();
        ImGui::SetNextWindowSize(region);
        ImGui::ListBox("##500", &logs_index[CONSOLE], &logs_focus[CONSOLE], [](void* user_data, int index) {
            auto* logs = (std::string*)user_data;
            return logs[index].c_str();
        }, logs[CONSOLE].data(), (int)logs[CONSOLE].size());
        logs_focus[CONSOLE] = -1;
    }
    ImGui::End();
}

static void RefreshCompiler()
{
    if (refresh_compiler == false)
        return;
    refresh_compiler = false;

    VirtualMachine::Close(cpu);
    cpu = nullptr;

    for (auto& [title, binary] : binaries) {
        binary.clear();
    }
    binary.clear();

    logs[SYSTEM].clear();
    logs[CONSOLE].clear();

    if (compilers.size() > compiler_index) {
        std::string compiler = compilers[compiler_index];
        std::string path = compiler_path + "/" + compiler;

        if (strncmp(text.c_str(), "!!ARB", 5) == 0) {
            binary.assign(text.begin(), text.end());
            return;
        }

        if (strncasecmp(compiler.data(), "d3dx9", 5) == 0 ||
            strncasecmp(compiler.data(), "d3dcompiler", 11) == 0) {
            if (text.find('{') == std::string::npos) {
                cpu = VirtualMachine::RunDLL(path, D3DCompiler::RunD3DAssemble, debug_vm);
            }
            else {
                cpu = VirtualMachine::RunDLL(path, D3DCompiler::RunD3DCompile, debug_vm);
            }
            if (cpu) {
                begin_execute = std::chrono::system_clock::now();
            }
        }
    }
}

static void RefreshMachine()
{
    if (binary.empty() || cpu)
        return;

    if (refresh_machine == false)
        return;
    refresh_machine = false;

    VirtualMachine::Close(cpu);
    cpu = nullptr;

    for (auto& [title, binary] : ShaderCompiler::binaries) {
        if (title == "AMD" || title == "NVIDIA") {
            binary.clear();
        }
    }

//  logs[SYSTEM].clear();
//  logs[CONSOLE].clear();

    if (machines.size() > machine_index) {
        std::string machine = machines[machine_index];

        // AMD
        if (machine.find("AMDIL") == 0 || machine.find("R") == 0) {
            std::string path = machine_path + "/" + "GPUShaderAnalyzer_Catalyst_11_7_DX9.dll";
            cpu = VirtualMachine::RunDLL(path, AMDCompiler::RunAMDCompile, debug_vm);
            if (cpu) {
                begin_execute = std::chrono::system_clock::now();
            }
            return;
        }

        // NVIDIA
        std::string path = machine_path + "/" + "NVShaderPerf.dll";
        if (machine.find("NV20") == 0) {
            path = machine_path + "/" + "NVKelvinR7.dll";
        }
        cpu = VirtualMachine::RunDLL(path, NVCompiler::RunNVCompile, debug_vm);
        if (cpu) {
            begin_execute = std::chrono::system_clock::now();
        }
    }
}

static void Loop()
{
    if (cpu == nullptr)
        return;

    uint32_t begin = 0;
    for (;;) {
        if (cpu->Step(1000) == false) {
            Logger<SYSTEM>("%s", cpu->Disassemble(1).c_str());
            Logger<SYSTEM>("%s", cpu->Status().c_str());

            auto stack = cpu->Stack();
            for (int i = 0; i < 16; ++i) {
                auto* value = (uint32_t*)(cpu->Memory(stack + i * 4));
                if (value == nullptr)
                    break;
                Logger<SYSTEM>("%08X : %08X", stack + i * 4, (*value));
            }

            logs_index[CONSOLE] = (int)logs[CONSOLE].size();
            logs_index[SYSTEM] = (int)logs[SYSTEM].size();

            mine* origin = cpu;
            cpu = D3DCompiler::RunNextProcess(origin);
            if (cpu == nullptr)
                cpu = AMDCompiler::RunNextProcess(origin);
            if (cpu == nullptr)
                cpu = NVCompiler::RunNextProcess(origin);
            if (cpu == nullptr) {
                auto end_execute = std::chrono::system_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_execute - begin_execute).count();
                Logger<CONSOLE>("Duration : %lldms\n", duration);
                delete origin;
            }
            return;
        }
#if defined(_WIN32)
        uint32_t now = GetTickCount();
#else
        struct timespec ts = {};
        clock_gettime(CLOCK_REALTIME, &ts);
        uint32_t now = uint32_t(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
        if (begin == 0)
            begin = now;
        if (begin < now - 16)
            break;
    }
}

static void Init()
{
    ImGuiID id = ImGui::GetID("Shader Compiler");

    static bool initialize = false;
    if (initialize) {
        ImGui::DockSpace(id);
        return;
    }
    initialize = true;

    ImGuiID dockid = ImGui::DockBuilderAddNode(id);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(dockid, ImGuiDir_Down, 1.0f / 4.0f, nullptr, &dockid);
    ImGuiID left = ImGui::DockBuilderSplitNode(dockid, ImGuiDir_Left, 3.0f / 7.0f, nullptr, &dockid);
    ImGuiID middle = ImGui::DockBuilderSplitNode(dockid, ImGuiDir_Left, 1.0f / 4.0f, nullptr, &dockid);
    ImGuiID right = binary_dockid = dockid;
    ImGui::DockBuilderDockWindow("Text", left);
    ImGui::DockBuilderDockWindow("Option", middle);
    ImGui::DockBuilderDockWindow("Binary", right);
    ImGui::DockBuilderDockWindow("System", bottom);
    ImGui::DockBuilderDockWindow("Console", bottom);
    ImGui::DockBuilderFinish(dockid);

    std::string cwd(1024, 0);
    uint32_t size = (uint32_t)cwd.size();
    _NSGetExecutablePath(cwd.data(), &size);
    cwd.resize(strlen(cwd.c_str()));

    shader_path.resize(1024);
    realpath((cwd + "/../../../../../../shader").c_str(), shader_path.data());
    shader_path.resize(strlen(shader_path.c_str()));
    if (shader_path.empty() == false) {
        for (int type = 0; type < 2; ++type) {
            DIR* dir = opendir(shader_path.c_str());
            if (dir) {
                while (struct dirent* dirent = readdir(dir)) {
                    if (dirent->d_name[0] == '.')
                        continue;
                    switch (type) {
                    case 0:
                        if (strcasestr(dirent->d_name, ".asm") == nullptr)
                            continue;
                        break;
                    case 1:
                        if (strcasestr(dirent->d_name, ".hlsl") == nullptr)
                            continue;
                        break;
                    }
                    shaders.push_back(dirent->d_name);
                }
                closedir(dir);
            }
        }
    }
    std::stable_sort(shaders.begin(), shaders.end());

    compiler_path.resize(1024, 0);
    realpath((cwd + "/../../../../../../compiler").c_str(), compiler_path.data());
    compiler_path.resize(strlen(compiler_path.c_str()));
    if (compiler_path.empty() == false) {
        DIR* dir = opendir(compiler_path.c_str());
        if (dir) {
            while (struct dirent* dirent = readdir(dir)) {
                if (dirent->d_name[0] == '.')
                    continue;
                if (strcasestr(dirent->d_name, ".dll") == nullptr)
                    continue;
                compilers.push_back(dirent->d_name);
            }
            closedir(dir);
        }
    }
    std::stable_sort(compilers.begin(), compilers.end());

    machine_path.resize(1024, 0);
    realpath((cwd + "/../../../../../../machine").c_str(), machine_path.data());
    machine_path.resize(strlen(machine_path.c_str()));
    if (machine_path.empty() == false) {
        std::vector<std::string> orders[4];
        DIR* dir = opendir(machine_path.c_str());
        if (dir) {
            while (struct dirent* dirent = readdir(dir)) {
                if (dirent->d_name[0] == '.')
                    continue;
                if (strcmp(dirent->d_name, "GPUShaderAnalyzer_Catalyst_11_7_DX9.dll") == 0) {
                    orders[0].push_back("AMDIL - 11.7");
                    orders[0].push_back("R600 - 11.7");
                    orders[0].push_back("RV610 - 11.7");
                    orders[0].push_back("RV630 - 11.7");
                    orders[0].push_back("RV670 - 11.7");
                    orders[0].push_back("RV770 - 11.7");
                    orders[0].push_back("RV730 - 11.7");
                    orders[0].push_back("RV710 - 11.7");
                    orders[0].push_back("RV740 - 11.7");
                    orders[0].push_back("RV870 - 11.7");
                    orders[0].push_back("RV840 - 11.7");
                    orders[0].push_back("RV830 - 11.7");
                    orders[0].push_back("RV810 - 11.7");
                    orders[0].push_back("RV970 - 11.7");
                    orders[0].push_back("RV940 - 11.7");
                    orders[0].push_back("RV930 - 11.7");
                    orders[0].push_back("RV910 - 11.7");
                    continue;
                }
                if (strcmp(dirent->d_name, "NVKelvinR7.dll") == 0) {
                    orders[1].push_back("NV20 - 7.58");
                    continue;
                }
                if (strcmp(dirent->d_name, "2.01.10000.0305") == 0) {
                    orders[2].push_back("NV30 - 101.31");
                    orders[2].push_back("NV35 - 101.31");
                    orders[2].push_back("NV40 - 101.31");
                    orders[2].push_back("G70 - 101.31");
                    continue;
                }
                if (strcmp(dirent->d_name, "2.07.0804.1530") == 0) {
                    orders[3].push_back("NV40 - 174.74");
                    orders[3].push_back("G70 - 174.74");
                    orders[3].push_back("G80 - 174.74");
                    continue;
                }
            }
            closedir(dir);
        }
        for (auto& order : orders) {
            machines.insert(machines.end(), order.begin(), order.end());
        }
    }

    LoadCompiler();
    LoadShader();
    entry = "Main";

    ImGui::DockSpace(id);
}

bool ShaderCompilerGUI(ImVec2 screen)
{
    bool show = true;
    ImVec2 window_size = ImVec2(1536.0f, 864.0f);

    ImGui::SetNextWindowPos(ImVec2((screen.x - window_size.x) / 2.0f, (screen.y - window_size.y) / 2.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Once);
    if (ImGui::Begin("Shader Compiler", &show, ImGuiWindowFlags_NoCollapse)) {
        Init();
        Text();
        Option();
        Binary();
        System();
        Console();
        RefreshCompiler();
        RefreshMachine();
        Loop();
    }
    ImGui::End();
    return show;
}
