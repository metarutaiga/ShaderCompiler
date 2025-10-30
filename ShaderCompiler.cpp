#define IMGUI_DEFINE_MATH_OPERATORS
#if defined(_WIN32)
#else
#include <unistd.h>
#include <sys/dir.h>
#include <mach-o/dyld.h>
#endif
#include <sys/stat.h>
#include <algorithm>
#include <chrono>
#include <map>
#include <vector>
#include "mine/mine.h"
#include "mine/syscall/allocator.h"
#include "src/AMDCompiler.h"
#include "src/ATICompiler.h"
#include "src/D3DCompiler.h"
#include "src/MaliCompiler.h"
#include "src/NVCompiler.h"
#include "src/QCOMCompiler.h"
#include "src/UnifiedExecution.h"
#include "src/VirtualMachine.h"
#include "ImGuiHelper.h"
#include "Logger.h"
#include "ShaderCompiler.h"

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
std::string driver_path;

std::string text;
std::vector<std::string> shaders;
int shader_index;

std::vector<Compiler> compilers;
int compiler_index;

std::string entry;

std::vector<std::string> profiles;
int profile_index;

std::vector<std::string> types;
int type_index;

std::vector<Driver> drivers;
int driver_index;
int machine_index;

std::map<std::string, Output> outputs;

int GetShaderType()
{
    std::string type;
    if (types.size() > type_index)
        type = types[type_index];

    if (type == "Auto Detect") {
        if (text.find("precision mediump float;") != std::string::npos ||
            text.find("gl_FragColor") != std::string::npos) {
            return 'frag';
        }
        if (text.find("ps_") != std::string::npos ||
            text.find("):COLOR") != std::string::npos ||
            text.find("): COLOR") != std::string::npos ||
            text.find(") : COLOR") != std::string::npos) {
            return 'frag';
        }
    }
    else if (type == "Vertex") {
        return 'vert';
    }
    else if (type == "Pixel") {
        return 'frag';
    }
    return 'vert';
}

std::string GetProfile()
{
    auto type = GetShaderType();
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
        auto& compiler = compilers[compiler_index];
        if (strcasestr(compiler.path.c_str(), "d3dx9") ||
            strcasestr(compiler.path.c_str(), "d3dcompiler")) {
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

    types.clear();
    types.push_back("Auto Detect");
    types.push_back("Vertex");
    types.push_back("Pixel");
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
            auto* compilers = (Compiler*)user_data;
            return compilers[index].name.c_str();
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

        // Type
        ImGui::TextUnformatted("Type");
        ImGui::SetNextItemWidth(region.x);
        if (ImGui::Combo("##204", &type_index, [](void* user_data, int index) {
            auto* types = (std::string*)user_data;
            return types[index].c_str();
        }, types.data(), (int)types.size()) || ImGui::ScrollCombo(&type_index, types.size())) {
            refresh_compiler = true;
            refresh_machine = true;
        }

        // Driver
        ImGui::NewLine();
        ImGui::TextUnformatted("Driver");
        ImGui::SetNextItemWidth(region.x);
        if (ImGui::Combo("##205", &driver_index, [](void* user_data, int index) {
            auto* drivers = (Driver*)user_data;
            return drivers[index].name[0].c_str();
        }, drivers.data(), (int)drivers.size()) || ImGui::ScrollCombo(&driver_index, drivers.size())) {
            refresh_machine = true;
        }

        // Machine
        static std::vector<std::vector<std::string>> empty_machines;
        auto& machines = (drivers.size() > driver_index) ? drivers[driver_index].machines : empty_machines;
        if (machine_index >= (int)machines.size())
            machine_index = (int)machines.size() - 1;
        ImGui::TextUnformatted("Machine");
        ImGui::SetNextItemWidth(region.x);
        if (ImGui::Combo("##206", &machine_index, [](void* user_data, int index) {
            auto* machines = (std::vector<std::string>*)user_data;
            return machines[index].front().c_str();
        }, machines.data(), (int)machines.size()) || ImGui::ScrollCombo(&machine_index, machines.size())) {
            refresh_machine = true;
        }

        // Debug
        ImGui::NewLine();
        ImGui::Checkbox("Debug Virtual Machine", &debug_vm);
        if (cpu) {
            auto allocator = cpu->Allocator;
            ImGui::Text("%08zX : %.2fMB", cpu->Program(), allocator->used_size() / 1048576.0f);
        }
    }
    ImGui::End();
}

static void Binary()
{
    int id = 300;

    auto hex = [](std::vector<char>& data, int& index) {
        ImGui::ListBox("", &index, [](void* user_data, int index) -> const char* {
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

            char* ascii = line + width;
            for (int j = 0; j < 16; ++j) {
                if ((i + j) >= size)
                    break;
                uint8_t c = binary[i + j];
                if (c == 0x00 || c == '\n') {
                    (*ascii++) = ' ';
                }
                else if (c >= 0x01 && c <= 0x7F) {
                    (*ascii++) = c;
                }
                else {
                    for (int i = 0; i < 4; ++i) {
                        char u = eascii[c - 0x80][i];
                        if (u == 0)
                            break;
                        (*ascii++) = u;
                    }
                }
            }
            (*ascii++) = 0;

            return line;
        }, &data, (int)(data.size() + 15) / 16);
    };

    for (auto& [title, output] : outputs) {
        char name[64];

        // Binary
        snprintf(name, 64, "%s", title.empty() ? "Output" : title.c_str());
        ImGui::DockBuilderDockWindow(name, binary_dockid);
        if (ImGui::Begin(name, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
            ImVec2 region = ImGui::GetContentRegionAvail();
            ImGui::PushID(id++);
            ImGui::SetNextWindowSize(region);
            hex(output.binary, output.binary_index);
            ImGui::PopID();
        }
        ImGui::End();

        // Disassembly
        snprintf(name, 64, "%s:%s", title.empty() ? "Output" : title.c_str(), "Disassembly");
        ImGui::DockBuilderDockWindow(name, binary_dockid);
        if (ImGui::Begin(name, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
            ImVec2 region = ImGui::GetContentRegionAvail();
            ImGui::PushID(id++);
            ImGui::InputTextMultiline("", output.disasm, region, ImGuiInputTextFlags_ReadOnly);
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

    for (auto& [title, output] : outputs) {
        output.binary.clear();
        output.disasm.clear();
    }

    logs[SYSTEM].clear();
    logs[CONSOLE].clear();

    if (compilers.size() > compiler_index) {
        auto& compiler = compilers[compiler_index];
        std::string path = compiler_path + "/" + compiler.path;

        if (strncasecmp(text.c_str(), "#version", 8) == 0) {
            auto& output = outputs[""];
            output.binary.assign(text.begin(), text.end());
            return;
        }

        if (strncmp(text.c_str(), "!!ARB", 5) == 0) {
            auto& output = outputs[""];
            output.binary.assign(text.begin(), text.end());
            return;
        }

        if (strcasestr(compiler.path.c_str(), "d3dx9") ||
            strcasestr(compiler.path.c_str(), "d3dcompiler")) {
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
    auto& output = outputs[""];
    if (output.binary.empty() || cpu)
        return;

    if (refresh_machine == false)
        return;
    refresh_machine = false;

    VirtualMachine::Close(cpu);
    cpu = nullptr;

    for (auto& [title, output] : outputs) {
        if (title.empty() == false) {
            output.binary.clear();
            output.disasm.clear();
        }
    }

//  logs[SYSTEM].clear();
//  logs[CONSOLE].clear();

    if (drivers.size() > driver_index && drivers[driver_index].machines.size() > machine_index) {
        auto& driver = drivers[driver_index];
        if (driver.name.size() > 1) {
            std::string path = driver_path + "/" + driver.name[1];
            cpu = VirtualMachine::RunDLL(path, UnifiedExecution::RunDriver, debug_vm);
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

    uint32_t begin = 0;
    for (;;) {
        if (cpu->Step(1000) == false) {
            Logger<SYSTEM>("%s", cpu->Disassemble(1).c_str());
            Logger<SYSTEM>("%s", cpu->Status().c_str());

            auto stack = cpu->Stack();
            for (int i = -4; i < 16; ++i) {
                auto* value = (uint32_t*)(cpu->Memory(stack + i * 4));
                if (value == nullptr)
                    break;
                Logger<SYSTEM>("%s%08X : %08X", i == 0 ? ">" : " ", stack + i * 4, (*value));
            }

            logs_index[CONSOLE] = (int)logs[CONSOLE].size();
            logs_index[SYSTEM] = (int)logs[SYSTEM].size();

            mine* origin = cpu;
            cpu = D3DCompiler::NextProcess(origin);
            if (cpu == nullptr)
                cpu = AMDCompiler::NextProcess(origin);
            if (cpu == nullptr)
                cpu = ATICompiler::NextProcess(origin);
            if (cpu == nullptr)
                cpu = MaliCompiler::NextProcess(origin);
            if (cpu == nullptr)
                cpu = NVCompiler::NextProcess(origin);
            if (cpu == nullptr)
                cpu = QCOMCompiler::NextProcess(origin);
            if (cpu == nullptr) {
                auto end_execute = std::chrono::system_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_execute - begin_execute).count();
                Logger<CONSOLE>("Duration : %lldms\n", duration);
                VirtualMachine::Close(origin);
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
    ImGuiID left = ImGui::DockBuilderSplitNode(dockid, ImGuiDir_Left, 2.0f / 5.0f, nullptr, &dockid);
    ImGuiID middle = ImGui::DockBuilderSplitNode(dockid, ImGuiDir_Left, 1.0f / 3.0f, nullptr, &dockid);
    ImGuiID right = binary_dockid = dockid;
    ImGui::DockBuilderDockWindow("Text", left);
    ImGui::DockBuilderDockWindow("Option", middle);
    ImGui::DockBuilderDockWindow("Output", right);
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
        DIR* dir = opendir(shader_path.c_str());
        if (dir) {
            while (struct dirent* dirent = readdir(dir)) {
                if (dirent->d_name[0] == '.')
                    continue;
                if (strcasestr(dirent->d_name, ".asm") == nullptr &&
                    strcasestr(dirent->d_name, ".glsl") == nullptr &&
                    strcasestr(dirent->d_name, ".hlsl") == nullptr)
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
        FILE* file = fopen((compiler_path + "/compiler.ini").c_str(), "rb");
        if (file) {
            char line[256];
            while (fgets(line, 256, file)) {
                Compiler compiler;
                for (int i = 0; i < 256; ++i) {
                    char c = line[i];
                    if (c == 0 || c == '\r' || c == '\n')
                        break;
                    if (c == '=') {
                        compiler.name.swap(compiler.path);
                        continue;
                    }
                    compiler.name.push_back(c);
                }
                compilers.push_back(compiler);
            }
            fclose(file);
        }

        // Remove unavailable
        struct stat st;
        for (int64_t i = compilers.size() - 1; i >= 0; --i) {
            auto path = compiler_path + "/" + compilers[i].path;
            if (stat(path.c_str(), &st) != 0) {
                compilers.erase(compilers.begin() + i);
            }
        }
    }

    driver_path.resize(1024, 0);
    realpath((cwd + "/../../../../../../driver").c_str(), driver_path.data());
    driver_path.resize(strlen(driver_path.c_str()));
    if (driver_path.empty() == false) {
        FILE* file = fopen((driver_path + "/driver.ini").c_str(), "rb");
        if (file) {
            auto finish = []() {
                for (auto& driver : drivers) {
                    if (driver.machines.empty()) {
                        driver.machines = drivers.back().machines;
                    }
                }
            };

            bool quotation_mark = false;
            char line[256];
            while (fgets(line, 256, file)) {
                switch (line[0]) {
                case 0:
                    break;
                case '[':
                    finish();

                    drivers.push_back(Driver());
                    for (int i = 1; i < 256; ++i) {
                        char c = line[i];
                        if (c == 0 || c == '\r' || c == '\n' || c == ']')
                            break;
                        if (c == '=') {
                            drivers.back().name.push_back(std::string());
                            continue;
                        }
                        if (drivers.back().name.empty())
                            drivers.back().name.push_back(std::string());
                        drivers.back().name.back().push_back(c);
                    }
                    break;
                default:
                    std::vector<std::string> machine;
                    for (int i = 0; i < 256; ++i) {
                        char c = line[i];
                        if (c == 0 || c == '\r' || c == '\n')
                            break;
                        if ((c == '=' || c == ',') && quotation_mark == false) {
                            machine.push_back(std::string());
                            continue;
                        }
                        if (c == '\"') {
                            quotation_mark = !quotation_mark;
                        }
                        if (machine.empty())
                            machine.push_back(std::string());
                        machine.back().push_back(c);
                    }
                    if (drivers.empty() == false && machine.empty() == false) {
                        drivers.back().machines.push_back(machine);
                    }
                    break;
                }
            }
            fclose(file);

            finish();
        }

        // Remove unavailable
        struct stat st;
        for (int64_t i = drivers.size() - 1; i >= 0; --i) {
            auto path = driver_path + "/" + drivers[i].name.back();
            if (stat(path.c_str(), &st) != 0) {
                drivers.erase(drivers.begin() + i);
            }
        }
    }

    LoadCompiler();
    LoadShader();
    entry = "Main";

    ImGui::DockSpace(id);
}

bool ShaderCompilerGUI(ImVec2 screen)
{
    static int resize = 1;
    static ImVec2 window_size = ImVec2(1536.0f, 864.0f);

    bool show = true;
    const char* title = "Shader Compiler";
    ImGui::SetNextWindowSize(window_size, resize > 0 ? ImGuiCond_Always : ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2((screen.x - window_size.x) / 2.0f, (screen.y - window_size.y) / 2.0f), resize > 0 ? ImGuiCond_Always : ImGuiCond_Once);
    resize = -abs(resize);

    if (ImGui::Begin(title, &show, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse)) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive));
        ImGui::BeginChild(title, ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight()));
        if (ImGui::Button("X")) {
            show = false;
        }
        ImGui::SameLine();
        ImVec2 region = ImGui::GetContentRegionAvail();
        float offset = (region.x - ImGui::CalcTextSize(title).x) / 2.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
        ImGui::TextUnformatted(title);
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            switch (abs(resize)) {
            case 1:
                resize = 2;
                window_size.x = screen.x;
                window_size.y = screen.y - 128.0f;
                break;
            case 2:
                resize = 1;
                window_size.x = 1536.0f;
                window_size.y = 864.0f;
                break;
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

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
