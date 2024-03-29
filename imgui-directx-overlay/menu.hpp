#include "include.h"

inline void DrawStyle()
{
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.LogFilename = NULL;

    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImGuiStyle& style = ImGui::GetStyle();

    style.AntiAliasedFill = true;
    style.AntiAliasedLines = true;

    ImVec4* colors = ImGui::GetStyle().Colors;
}

inline void DrawMenu()
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowSize({ 784, 375 }, ImGuiCond_Once);
    ImGui::SetNextWindowPos({ 64, 32 }, ImGuiCond_Once);

    ImGui::Begin("Window one", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);
    {
        ImGui::Text("screen %g x %g", io.DisplaySize.x, io.DisplaySize.y);
        ImGui::Text("mouse %g x %g", io.MousePos.x, io.MousePos.y);
    }
    ImGui::End();
}

inline void WindowClamp()
{
    ImGuiIO& io = ImGui::GetIO();

    ImVec2 min = ImVec2(0, 0);
    ImVec2 max = ImVec2(io.DisplaySize.x, io.DisplaySize.y);

    ImGui::GetStyle().WindowMinSize = ImVec2(0, 0);

    ImVector<ImGuiWindow*>& windows = ImGui::GetCurrentContext()->Windows;

    for (int i = 0; i < windows.Size; i++)
    {
        ImGuiWindow* window = windows[i];

        window->Pos = ImVec2(ImClamp(window->Pos.x, min.x, max.x - window->Size.x), ImClamp(window->Pos.y, min.y, max.y - window->Size.y));
    }
}