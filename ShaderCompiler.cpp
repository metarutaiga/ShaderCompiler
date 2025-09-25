#define IMGUI_DEFINE_MATH_OPERATORS
#if defined(_WIN32)
#else
#include <unistd.h>
#include <sys/dir.h>
#include <mach-o/dyld.h>
#endif
#include <map>
#include <vector>
#include "ImGuiHelper.h"
#include "Logger.h"
#include "VirtualMachine.h"

#include "mine/syscall/allocator.h"
#include "mine/syscall/syscall_internal.h"
#include "mine/x86/x86_i386.h"
#include "mine/x86/x86_instruction.inl"
#include "mine/x86/x86_register.inl"

static std::string shader_path;
static std::string compiler_path;
static std::string text;
static std::vector<std::string> shaders;
static int shader_index;
static std::vector<std::string> compilers;
static int compiler_index;
static std::string entry;
static std::vector<std::string> profiles;
static int profile_index;
static std::map<std::string, std::string> binaries;
static std::vector<char> binary;
static ImGuiID binary_dockid;
static mine* cpu;

static const char* DetectProfile()
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

static void LoadCompiler()
{
    profiles.clear();
    if (compilers.size() > compiler_index) {
        std::string compiler = compilers[compiler_index];
        if (strncasecmp(compiler.data(), "d3dcompiler", 11) == 0) {
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
        ImGui::TextUnformatted("Shader");
        ImGui::SetNextItemWidth(region.x);
        if (ImGui::Combo("##200", &shader_index, [](void* user_data, int index) {
            auto* shaders = (std::string*)user_data;
            return shaders[index].c_str();
        }, shaders.data(), (int)shaders.size())) {
            refresh = true;
            LoadShader();
        }
        ImGui::TextUnformatted("Compiler");
        ImGui::SetNextItemWidth(region.x);
        if (ImGui::Combo("##201", &compiler_index, [](void* user_data, int index) {
            auto* compilers = (std::string*)user_data;
            return compilers[index].c_str();
        }, compilers.data(), (int)compilers.size())) {
            refresh = true;
            LoadCompiler();
        }
        ImGui::TextUnformatted("Entry");
        refresh |= ImGui::InputTextEx("##202", nullptr, entry, ImVec2(region.x, 0));
        ImGui::TextUnformatted("Profile");
        ImGui::SetNextItemWidth(region.x);
        if (ImGui::Combo("##203", &profile_index, [](void* user_data, int index) {
            auto* profiles = (std::string*)user_data;
            return profiles[index].c_str();
        }, profiles.data(), (int)profiles.size())) {
            refresh = true;
        }
    }
    ImGui::End();
    return refresh;
}

static void Binary()
{
    static std::map<std::string, std::string> dummy = { { "Binary", "" } };

    int id = 300;
    for (auto& [title, binary] : binaries.empty() ? dummy : binaries) {
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

static size_t RunD3DCompile(mine* cpu, size_t(*symbol)(const char*, void*), void* image)
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;

    size_t D3DCompile = symbol("D3DCompile", image);
    if (D3DCompile) {
        auto profile = DetectProfile();

        auto pSrcData = VirtualMachine::DataToMemory(text.data(), text.size(), allocator);
        auto SrcDataSize = text.size();
        auto pEntrypoint = VirtualMachine::DataToMemory(entry.data(), entry.size() + 1, allocator);
        auto pTarget = VirtualMachine::DataToMemory(profile, strlen(profile) + 1, allocator);

        Push32(0);
        auto ppErrorMsgs = ESP;
        Push32(0);
        auto ppCode = ESP;
        Push32('D3DC');

        Push32(ppErrorMsgs);    // **ppErrorMsgs    optional
        Push32(ppCode);         // **ppCode
        Push32(0);              // Flags2
        Push32(1 << 12);        // Flags1
        Push32(pTarget);        // pTarget
        Push32(pEntrypoint);    // pEntrypoint      optional
        Push32(0);              // *pInclude        optional
        Push32(0);              // *pDefines        optional
        Push32(0);              // pSourceName      optional
        Push32(SrcDataSize);    // SrcDataSize
        Push32(pSrcData);       // pSrcData
    }

    return D3DCompile;
}

static size_t RunD3DDisassemble(mine* cpu, size_t(*symbol)(const char*, void*), void* image)
{
    auto* allocator = cpu->Allocator;
    auto* i386 = (x86_i386*)cpu;
    auto& x86 = i386->x86;

    size_t D3DDisassemble = symbol("D3DDisassemble", image);
    if (D3DDisassemble) {
        auto pSrcData = VirtualMachine::DataToMemory(binary.data(), binary.size(), allocator);
        auto SrcDataSize = binary.size();

        Push32(0);
        auto ppDisassembly = ESP;
        Push32('D3DD');

        Push32(ppDisassembly);  // **ppDisassembly
        Push32(0);              // szComments       optional
        Push32(0);              // Flags
        Push32(SrcDataSize);    // SrcDataSize
        Push32(pSrcData);       // pSrcData
    }

    return D3DDisassemble;
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
        std::string path = compiler_path + "/" + compilers[compiler_index];

        cpu = VirtualMachine::RunDLL(path.c_str(), RunD3DCompile);
    }
}

static void Loop()
{
    if (cpu) {
        uint32_t count = 0;
        uint32_t begin = 0;
        for (;;) {
            if (cpu->Step('INTO') == false) {
                Logger<SYSTEM>("%s", cpu->Disassemble(1).c_str());
                Logger<SYSTEM>("%s", cpu->Status().c_str());
                logs_index[CONSOLE] = (int)logs[CONSOLE].size();
                logs_index[SYSTEM] = (int)logs[SYSTEM].size();

                auto* allocator = cpu->Allocator;
                auto* i386 = (x86_i386*)cpu;
                auto& x86 = i386->x86;
                auto* memory = (uint8_t*)allocator->address();
                auto* stack = (uint32_t*)(memory + cpu->Stack());
                switch (stack[0]) {
                case 'D3DC': {
                    auto* error = (uint32_t*)(memory + stack[2]);
                    if (error) {
//                      auto& size = error[2];
                        auto& pointer = error[3];
                        auto* message = (uint32_t*)(memory + pointer);

                        Logger<CONSOLE>("%s", message);
                    }
                    auto* blob = (uint32_t*)(memory + stack[1]);
                    if (blob && EAX == 0) {
                        auto& size = blob[2];
                        auto& pointer = blob[3];
                        auto* code = (uint32_t*)(memory + pointer);

                        std::string d3dsio;
                        for (auto i = 0; i < (size & ~15); i += 16) {
                            char line[256];
                            snprintf(line, 256, "%04X: %08X, %08X, %08X, %08X\n",
                                     i,
                                     code[i / 4 + 0],
                                     code[i / 4 + 1],
                                     code[i / 4 + 2],
                                     code[i / 4 + 3]);
                            d3dsio += line;
                        }
                        binaries["D3DSIO"] = d3dsio;
                        binary.assign((char*)code, (char*)code + size);

                        if (compilers.size() > compiler_index) {
                            std::string path = compiler_path + "/" + compilers[compiler_index];

                            delete cpu;
                            cpu = VirtualMachine::RunDLL(path.c_str(), RunD3DDisassemble);
                            return;
                        }
                    }
                    break;
                }
                case 'D3DD': {
                    auto* blob = (uint32_t*)(memory + stack[1]);
                    if (blob && EAX == 0) {
                        auto& size = blob[2];
                        auto& pointer = blob[3];
                        auto* code = (char*)(memory + pointer);

                        std::string disassembly(code, code + size);
                        binaries["D3DSIO Disassembly"] = disassembly;
                    }
                    break;
                }
                default:
                    break;
                }

                delete cpu;
                cpu = nullptr;
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
}

static void Init()
{
    ImGuiID id = ImGui::GetID("Shader Compiler");
    if (ImGui::DockBuilderGetNode(id) == nullptr) {
        id = ImGui::DockBuilderAddNode(id);

        ImGuiID bottom = ImGui::DockBuilderSplitNode(id, ImGuiDir_Down, 1.0f / 2.0f, nullptr, &id);
        ImGuiID left = ImGui::DockBuilderSplitNode(id, ImGuiDir_Left, 2.0f / 5.0f, nullptr, &id);
        ImGuiID middle = ImGui::DockBuilderSplitNode(id, ImGuiDir_Left, 1.0f / 3.0f, nullptr, &id);
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
                    shaders.push_back(dirent->d_name);
                }
                closedir(dir);
            }
        }

        compiler_path.resize(1024, 0);
        realpath((cwd + "/../../../../../../compiler").c_str(), compiler_path.data());
        compiler_path.resize(strlen(compiler_path.c_str()));
        if (compiler_path.empty() == false) {
            DIR* dir = opendir(compiler_path.c_str());
            if (dir) {
                while (struct dirent* dirent = readdir(dir)) {
                    if (dirent->d_name[0] == '.')
                        continue;
                    compilers.push_back(dirent->d_name);
                }
                closedir(dir);
            }
        }

        LoadCompiler();
        LoadShader();
        entry = "Main";

        id = ImGui::GetID("Shader Compiler");
    }
    ImGui::DockSpace(id);
}

void ShaderCompiler()
{
    ImVec2 window_size = ImVec2(1536.0f, 864.0f);

    ImGuiViewport* viewport = ImGui::GetWindowViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos + ImVec2((viewport->WorkSize.x - window_size.x) / 2.0f, (viewport->WorkSize.y - window_size.y) / 2.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Appearing);
    if (ImGui::Begin("Shader Compiler")) {
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
}
