#pragma once
#include "common.hpp"

namespace bee
{

/// <summary>
/// The FileIO class provides a cross-platform way to read and write files.
/// </summary>
class FileIO
{
public:
    /// <summary>
    /// Types of paths that can be used. These can be very different on different platforms.
    /// </summary>
    enum class Directory
    {
        SharedAssets,
        Assets,
        SaveFiles,
        Editor,
        Root,
        None
    };

    /// <summary>
    /// Read a text file into a string. The string is empty if the file was not found.
    /// </summary>
    static std::string ReadTextFile(Directory type, const std::filesystem::path& path);

    /// <summary>
    /// Write a string to a text file. The file is created if it does not exist.
    /// Returns true if the file was written successfully.
    /// </summary>
    static bool WriteTextFile(Directory type, const std::filesystem::path& path, const std::string& content);

    /// <summary>
    /// Read a binary file into a string. The vector is empty if the file was not found.
    /// </summary>
    static std::vector<char> ReadBinaryFile(Directory type, const std::filesystem::path& path);

    /// <summary>
    /// Write a string to a binary file. The file is created if it does not exist.
    /// Returns true if the file was written successfully.
    /// </summary>
    static bool WriteBinaryFile(Directory type, const std::filesystem::path& path, const std::vector<char>& content);

    /// <summary>
    /// Get the full path of a file.
    /// </summary>
    static std::filesystem::path GetPath(Directory type, const std::filesystem::path& relativePath);

    /// <summary>
    /// Converts a system path to a relative path based on the directory type.
    /// </summary>
    static std::filesystem::path GetRelativePath(Directory type, const std::filesystem::path& pfullSystemPathath);

    /// <summary>
    /// Get the file name from a path.
    /// </summary>
    static std::string GetFileName(const std::filesystem::path& path);

    /// <summary>
    /// Check if a file exists.
    /// </summary>
    static bool Exists(Directory type, const std::filesystem::path& path);

    /// <summary>
    /// Check the last time a file was modified. Only used on desktop platforms.
    /// </summary>
    static uint64_t LastModified(Directory type, const std::filesystem::path& path);

    /// <summary>
    /// Get a list of files in a directory.
    /// </summary>
    static std::vector<std::string> GetFilesInDirectory(Directory type, const std::filesystem::path& path);

private:
    friend class EngineClass;

    /// <summary>
    /// Initializes the File IO system, filling in all the paths and
    /// mounting the virtual file system on platforms that need it.
    /// </summary>
    FileIO();

    /// <summary>
    /// Unmount the virtual file system and shut down the File IO system.
    /// </summary>
    ~FileIO();
};

}  // namespace bee
