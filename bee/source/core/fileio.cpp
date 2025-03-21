#include <cassert>
#include <filesystem>
#include <fstream>

#include "core/engine.hpp"

#if defined(BEE_PLATFORM_PC)
#include <filesystem>
#endif

#include "core/fileio.hpp"
#include "tools/log.hpp"

using namespace bee;
using namespace std;

namespace fs = std::filesystem;

std::string FileIO::ReadTextFile(Directory type, const fs::path& path)
{
    const auto fullPath = GetPath(type, path);
    ifstream file(fullPath);
    if (!file.is_open())
    {
        Log::Error("Reading text file {} with full path {} was not found!", path.string(), fullPath.string());
        return string();
    }
    file.seekg(0, std::ios::end);
    const size_t size = file.tellg();
    string buffer(size, '\0');
    file.seekg(0);
    file.read(buffer.data(), size);
    return buffer;
}

bool FileIO::WriteTextFile(Directory type, const fs::path& path, const std::string& content)
{
    const auto fullPath = GetPath(type, path);
    ofstream file(fullPath);
    if (!file.is_open())
    {
        Log::Error("Writing text file {} with full path {} was not found!", path.string(), fullPath.string());
        return false;
    }
    file << content;
    file.close();
    return true;
}

std::vector<char> FileIO::ReadBinaryFile(Directory type, const fs::path& path)
{
    const auto fullPath = GetPath(type, path);
    ifstream file(fullPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        Log::Error("Reading bin file {} with full path {} was not found!", path.string(), fullPath.string());
        return vector<char>();
    }
    const streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size)) return buffer;
    assert(false);
    return vector<char>();
}

bool FileIO::WriteBinaryFile(Directory type, const fs::path& path, const std::vector<char>& content)
{
    const auto fullPath = GetPath(type, path);
    // Ensure the directory exists
    std::filesystem::path dir = std::filesystem::path(fullPath).parent_path();
    if (!std::filesystem::exists(dir))
    {
        std::filesystem::create_directories(dir);
    }
    ofstream file(fullPath, std::ios::binary);
    if (!file.is_open())
    {
        Log::Error("Writing bin file {} with full path {} was not found!", path.string(), fullPath.string());
        return false;
    }
    file.write(content.data(), content.size());
    file.close();
    return true;
}

fs::path FileIO::GetPath(Directory type, const fs::path& relativePath)
{
    fs::path path(relativePath);

    switch (type)
    {
#ifdef BEE_PLATFORM_PC
        case Directory::Assets:
            return bee::Engine.Settings().projectPath / fs::path("assets") / relativePath;
        case Directory::SharedAssets:
            return fs::path("bee/assets") / relativePath;
        case Directory::SaveFiles:
            return bee::Engine.Settings().projectPath / fs::path("save") / relativePath;
        case Directory::Editor:
            return fs::path("bee/editor/assets") / relativePath;
        case Directory::Root:
            return path;
        case Directory::None:
            return path;
#else
        case Directory::Assets:
            return fs::path("/app0") / relativePath;
        case Directory::SharedAssets:
            return fs::path("/app0/shared") / relativePath;
        case Directory::SaveFiles:
            return fs::path("/app0/save") / relativePath;
        case Directory::Editor:
            return fs::path("/app0/editor/assets") / relativePath;
        case Directory::Root:
            return fs::path("/app0") / relativePath;
        case Directory::None:
            return path;
        default:
            return path;
#endif
    }

    return path;
}

#include <filesystem>
#include <algorithm>

fs::path NormalizePath(const fs::path& path)
{
    // Use lexically_normal to handle normalization (e.g., resolving ".." and ".")
    return path.lexically_normal();
}

fs::path bee::FileIO::GetRelativePath(Directory, const fs::path& fullSystemPath)
{
    fs::path projectPath = fs::current_path();
    fs::path relativePath = fullSystemPath.lexically_relative(projectPath);
    if (relativePath.empty())
    {
        // Handle the case where no relative path could be computed
        return NormalizePath(fullSystemPath);
    }
    return NormalizePath(relativePath);
}

std::string bee::FileIO::GetFileName(const fs::path& path)
{
    std::filesystem::path p(path);
    return p.filename().string();
}

bool FileIO::Exists(Directory type, const fs::path& path)
{
    const auto fullPath = GetPath(type, path);
    ifstream f(fullPath.c_str());
    auto good = f.good();
    f.close();
    return good;
}

#ifdef BEE_PLATFORM_PC
uint64_t FileIO::LastModified(Directory type, const fs::path& path)
{
    const auto fullPath = GetPath(type, path);
    std::filesystem::file_time_type ftime = std::filesystem::last_write_time(fullPath);
    return static_cast<uint64_t>(ftime.time_since_epoch().count());
}
std::vector<std::string> bee::FileIO::GetFilesInDirectory(Directory type, const fs::path& path)
{
    const auto fullPath = GetPath(type, path);
    std::vector<std::string> files;
    // check if the path exists
    if (!std::filesystem::exists(fullPath))
    {
        Log::Error("Directory {} does not exist!", fullPath.string());
        return files;
    }
    for (const auto& entry : std::filesystem::directory_iterator(fullPath))
    {
        files.push_back(entry.path().string());
    }
    return files;
}
#else
uint64_t FileIO::LastModified(Directory, const fs::path&) { return 0; }
#endif  // BEE_PLATFORM_PC
