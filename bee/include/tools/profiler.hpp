// implemented from my own small engine project, which was inspired by the xs engine
#pragma once
#include <string>
#include <chrono>

#define PROFILE_FUNCTION() bee::profiler::ProfilerSection section(__FUNCTION__)
#define PROFILE_SECTION(name) bee::profiler::ProfilerSection section(name)

namespace bee::profiler
{

using time_t = std::chrono::time_point<std::chrono::high_resolution_clock>;

class ProfilerSection
{
public:
    ProfilerSection(std::string name);
    ~ProfilerSection();

private:
    std::string name;
};

void BeginSection(const std::string& name);
void EndSection(const std::string& name);
void OnImGuiRender();

time_t now();
}  // namespace bee::profiler
