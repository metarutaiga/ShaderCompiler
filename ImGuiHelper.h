#include <string>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

//---------------------------------------------------------------------------
namespace ImGui
{
//---------------------------------------------------------------------------
inline bool InputTextEx(const char* label, const char* hint, std::string& text, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL)
{
    struct Chain
    {
        std::string*            Str;
        ImGuiInputTextCallback  ChainCallback;
        void*                   ChainCallbackUserData;
        static int Callback(ImGuiInputTextCallbackData* data)
        {
            Chain* chain = (Chain*)data->UserData;
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
            {
                chain->Str->resize(data->BufTextLen);
                data->Buf = chain->Str->data();
            }
            else if (chain->ChainCallback)
            {
                data->UserData = chain->ChainCallbackUserData;
                return chain->ChainCallback(data);
            }
            return 0;
        }
    };
    Chain chain = { &text, callback, user_data };
    return InputTextEx(label, hint, text.data(), (int)text.capacity() + 1, size, flags | ImGuiInputTextFlags_CallbackResize, Chain::Callback, &chain);
}
//---------------------------------------------------------------------------
inline bool InputTextMultiline(const char* label, std::string& text, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL)
{
    return InputTextEx(label, nullptr, text, size, flags | ImGuiInputTextFlags_Multiline, callback, user_data);
}
//---------------------------------------------------------------------------
} // namespace ImGui
//---------------------------------------------------------------------------
