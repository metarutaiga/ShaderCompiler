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
#include "D3DCompiler.h"
#include "ImGuiHelper.h"
#include "Logger.h"
#include "ShaderCompiler.h"
#include "VirtualMachine.h"

static std::chrono::system_clock::time_point begin_execute;
static ImGuiID binary_dockid;
static mine* cpu;
static bool debug_vm;

using namespace ShaderCompiler;
namespace ShaderCompiler {

std::string shader_path;
std::string compiler_path;

std::string text;
std::vector<std::string> shaders;
int shader_index;
std::vector<std::string> compilers;
int compiler_index;
std::string entry;
std::vector<std::string> profiles;
int profile_index;
std::map<std::string, std::string> binaries;
std::vector<char> binary;
int binary_index;

const char* DetectProfile()
{
    int type = 'vert';
    if (text.find(") : COLOR") != std::string::npos)
        type = 'frag';
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
            fseek(file, 0, SEEK_END);
            text.resize(ftell(file));
            fseek(file, 0, SEEK_SET);
            text.resize(fread(text.data(), 1, text.size(), file));
            fclose(file);
        }
    }
}

static bool Text()
{
    bool refresh = false;
    if (ImGui::Begin("Text")) {
        ImVec2 region = ImGui::GetContentRegionAvail();
        refresh = ImGui::InputTextMultiline("##100", text, region);
    }
    ImGui::End();
    return refresh;
}

static bool Option()
{
    bool refresh = false;
    if (ImGui::Begin("Option")) {
        ImVec2 region = ImGui::GetContentRegionAvail();

        // Shader
        ImGui::TextUnformatted("Shader");
        ImGui::SetNextItemWidth(region.x);
        if (ImGui::Combo("##200", &shader_index, [](void* user_data, int index) {
            auto* shaders = (std::string*)user_data;
            return shaders[index].c_str();
        }, shaders.data(), (int)shaders.size())) {
            refresh = true;
            LoadShader();
        }
        if (ImGui::ScrollCombo(&shader_index, shaders.size())) {
            refresh = true;
            LoadShader();
        }

        // Compiler
        ImGui::TextUnformatted("Compiler");
        ImGui::SetNextItemWidth(region.x);
        if (ImGui::Combo("##201", &compiler_index, [](void* user_data, int index) {
            auto* compilers = (std::string*)user_data;
            return compilers[index].c_str();
        }, compilers.data(), (int)compilers.size())) {
            refresh = true;
            LoadCompiler();
        }
        if (ImGui::ScrollCombo(&compiler_index, compilers.size())) {
            refresh = true;
            LoadShader();
        }

        // Entry
        ImGui::TextUnformatted("Entry");
        refresh |= ImGui::InputTextEx("##202", nullptr, entry, ImVec2(region.x, 0));

        // Profile
        ImGui::TextUnformatted("Profile");
        ImGui::SetNextItemWidth(region.x);
        if (ImGui::Combo("##203", &profile_index, [](void* user_data, int index) {
            auto* profiles = (std::string*)user_data;
            return profiles[index].c_str();
        }, profiles.data(), (int)profiles.size())) {
            refresh = true;
        }
        if (ImGui::ScrollCombo(&profile_index, profiles.size())) {
            refresh = true;
        }

        // Debug
        ImGui::NewLine();
        ImGui::Checkbox("Debug Virtual Machine", &debug_vm);
    }
    ImGui::End();
    return refresh;
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
            snprintf(line, 256, "%04X: %08X", i, code[i / 4 + 0]);
            if ((i +  4) < size) snprintf(line, 256, "%s, %08X", line, code[i / 4 + 1]);
            if ((i +  8) < size) snprintf(line, 256, "%s, %08X", line, code[i / 4 + 2]);
            if ((i + 12) < size) snprintf(line, 256, "%s, %08X", line, code[i / 4 + 3]);

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
        ImGui::ListBox("##400", &logs_index[SYSTEM], [](void* user_data, int index) {
            auto* logs = (std::string*)user_data;
            return logs[index].c_str();
        }, logs[SYSTEM].data(), (int)logs[SYSTEM].size());
    }
    ImGui::End();
}

static void Console()
{
    if (ImGui::Begin("Console")) {
        ImVec2 region = ImGui::GetContentRegionAvail();
        ImGui::SetNextWindowSize(region);
        ImGui::ListBox("##500", &logs_index[CONSOLE], [](void* user_data, int index) {
            auto* logs = (std::string*)user_data;
            return logs[index].c_str();
        }, logs[CONSOLE].data(), (int)logs[CONSOLE].size());
    }
    ImGui::End();
}

static void Refresh()
{
    delete cpu;
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

        if (strncasecmp(compiler.data(), "d3dx9", 5) == 0 ||
            strncasecmp(compiler.data(), "d3dcompiler", 11) == 0) {
            cpu = VirtualMachine::RunDLL(path.c_str(), D3DCompiler::RunD3DCompile, debug_vm);
            if (cpu) {
                begin_execute = std::chrono::system_clock::now();
            }
        }
    }
}

static void Loop()
{
    if (cpu == nullptr)
        return;

    uint32_t count = 0;
    uint32_t begin = 0;
    for (;;) {
        if (cpu->Step('INTO') == false) {
            Logger<SYSTEM>("%s", cpu->Disassemble(1).c_str());
            Logger<SYSTEM>("%s", cpu->Status().c_str());
            logs_index[CONSOLE] = (int)logs[CONSOLE].size();
            logs_index[SYSTEM] = (int)logs[SYSTEM].size();

            mine* origin = cpu;
            cpu = D3DCompiler::RunNextProcess(origin);
            if (cpu == nullptr) {
                auto end_execute = std::chrono::system_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_execute - begin_execute).count();
                Logger<CONSOLE>("Duration : %lldms", duration);
                delete origin;
            }
            return;
        }
        if (count == 0) {
            count = 1000;
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
        count--;
    }
}

static void Init()
{
    ImGuiID id = ImGui::GetID("Shader Compiler");
    if (ImGui::DockBuilderGetNode(id) == nullptr) {
        id = ImGui::DockBuilderAddNode(id);

        ImGuiID bottom = ImGui::DockBuilderSplitNode(id, ImGuiDir_Down, 1.0f / 4.0f, nullptr, &id);
        ImGuiID left = ImGui::DockBuilderSplitNode(id, ImGuiDir_Left, 3.0f / 7.0f, nullptr, &id);
        ImGuiID middle = ImGui::DockBuilderSplitNode(id, ImGuiDir_Left, 1.0f / 4.0f, nullptr, &id);
        ImGuiID right = binary_dockid = id;
        ImGui::DockBuilderDockWindow("Text", left);
        ImGui::DockBuilderDockWindow("Option", middle);
        ImGui::DockBuilderDockWindow("Binary", right);
        ImGui::DockBuilderDockWindow("System", bottom);
        ImGui::DockBuilderDockWindow("Console", bottom);
        ImGui::DockBuilderFinish(id);

        std::string cwd(1024, 0);
        uint32_t size = (uint32_t)cwd.size();;
        _NSGetExecutablePath(cwd.data(), &size);
        cwd.resize(strlen(cwd.c_str()));

        shader_path.resize(1024);
        realpath((cwd + "/../../../../../../shader").c_str(), shader_path.data());
        shader_path.resize(strlen(shader_path.c_str()));
        if (shader_path.empty() == false) {
            DIR* dir = opendir(shader_path.c_str());
            if (dir) {
                while (struct dirent* dirent = readdir(dir)) {
                    if (dirent->d_name[0] == '.')
                        continue;
                    if (strcasestr(dirent->d_name, ".hlsl") == nullptr)
                        continue;
                    shaders.push_back(dirent->d_name);
                }
                closedir(dir);
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

        LoadCompiler();
        LoadShader();
        entry = "Main";

        id = ImGui::GetID("Shader Compiler");
    }
    ImGui::DockSpace(id);
}

bool ShaderCompilerGUI(ImVec2 screen)
{
    bool show = true;
    ImVec2 window_size = ImVec2(1536.0f, 864.0f);

    ImGui::SetNextWindowPos(ImVec2((screen.x - window_size.x) / 2.0f, (screen.y - window_size.y) / 2.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Once);
    if (ImGui::Begin("Shader Compiler", &show, ImGuiWindowFlags_NoCollapse)) {
        bool refresh = false;
        Init();
        refresh |= Text();
        refresh |= Option();
        Binary();
        System();
        Console();
        if (refresh) {
            Refresh();
        }
        Loop();
    }
    ImGui::End();
    return show;
}
