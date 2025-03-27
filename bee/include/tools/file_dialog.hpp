#pragma once
#include "common.hpp"

// currenltyonly works with windows this one
#define FILE_FILTER(description, extension) description " (*" extension ")\0 * " extension "\0All Files(* .*)\0 *.*\0 "
#define FILE_MULTIPLE_FILTER(description, extension1, extension2)                                                  \
    description " (*" extension1 ")\0 * " extension1 "\0" description " (*" extension2 ")\0 * " extension2 \
                "\0All Files(* .*)\0 *.*\0 "

namespace bee
{
class FileDialog
{
public:
    static fs::path OpenFile(const char* filter);
    static std::vector<fs::path> OpenFiles(const char* filter);

    static fs::path SaveFile(const char* filter);
};
}  // namespace bee


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/