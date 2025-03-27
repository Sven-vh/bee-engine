#include "core.hpp"
#include "tools/file_dialog.hpp"

#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

fs::path bee::FileDialog::OpenFile(const char* filter)
{
    OPENFILENAMEA ofn;
    CHAR szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    void* window = bee::Engine.Device().GetWindow();
    ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)window);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    static std::string initialDir = (fs::current_path() / bee::Engine.Settings().projectPath).string();
    ofn.lpstrInitialDir = initialDir.c_str();

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        return fs::path(ofn.lpstrFile);
    }
    return fs::path();
}

std::vector<fs::path> bee::FileDialog::OpenFiles(const char* filter)
{
    OPENFILENAMEA ofn;
    CHAR szFile[65535] = {0};  // Larger buffer to store multiple file paths
    ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    void* window = bee::Engine.Device().GetWindow();
    ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)window);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    static std::string initialDir = (fs::current_path() / bee::Engine.Settings().projectPath).string();
    ofn.lpstrInitialDir = initialDir.c_str();

    std::vector<fs::path> paths;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        fs::path directory(szFile);
        CHAR* file = szFile + directory.string().size() + 1;

        if (*file == '\0')
        {
            paths.push_back(directory);
        }
        else
        {
            while (*file)
            {
                paths.push_back(directory / file);
                file += strlen(file) + 1;
            }
        }
    }

    return paths;
}

fs::path bee::FileDialog::SaveFile(const char* filter)
{
    OPENFILENAMEA ofn;
    CHAR szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    void* window = bee::Engine.Device().GetWindow();
    ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)window);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetSaveFileNameA(&ofn) == TRUE)
    {
        return fs::path(ofn.lpstrFile);
    }
    return fs::path();
}


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/