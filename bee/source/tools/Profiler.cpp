#include "core.hpp"

#define NOW std::chrono::high_resolution_clock::now()
#define HISTORY_DURATION 5.0f

namespace bee::profiler::internal
{
using duration_t = std::chrono::nanoseconds;

struct entry
{
    time_t start;
    time_t end;
    duration_t duration;
    int count = 0;
    float avg = 0.0f;
    std::deque<float> history;      // Stores durations
    std::deque<time_t> timestamps;  // Stores timestamps corresponding to each duration
};

struct fpsEntry
{
    float fps;
    time_t timestamp;
};
std::deque<fpsEntry> fpsHistory;

time_t timer{};
std::unordered_map<std::string, entry> entries;
}  // namespace bee::profiler::internal

using namespace bee::profiler;
using namespace bee::profiler::internal;

bee::profiler::ProfilerSection::ProfilerSection(std::string _name) : name(std::move(_name)) { BeginSection(name); }

bee::profiler::ProfilerSection::~ProfilerSection() { EndSection(name); }

void bee::profiler::BeginSection(const std::string& name) { entries[name].start = NOW; }

void bee::profiler::EndSection(const std::string& name)
{
    entry& e = entries[name];
    e.end = NOW;
    auto elapsed = e.end - e.start;
    e.duration += elapsed;
    e.count++;
}

void bee::profiler::OnImGuiRender()
{
    PROFILE_FUNCTION();
    auto now = NOW;
    float fps = 1.0f / std::chrono::duration<float>(now - timer).count();
    timer = now;
    fpsHistory.push_back({fps, now});

    auto maxTimestamp = now - std::chrono::duration<float>(HISTORY_DURATION);

    for (auto& entr : entries)
    {
        entry& e = entr.second;
        float duration = static_cast<float>(e.duration.count()) / 1000000.0f;  // Convert to milliseconds

        // Remove old data beyond the history duration
        while (!e.timestamps.empty() && e.timestamps.front() < maxTimestamp)
        {
            e.timestamps.pop_front();
            e.history.pop_front();
        }

        // Add the latest timestamp and duration
        e.timestamps.push_back(now);
        e.history.push_back(duration);

        // Calculate average duration
        e.avg = std::accumulate(e.history.begin(), e.history.end(), 0.0f) / e.history.size();
    }
    ImGui::Begin(ICON_FA_CHART_LINE TAB_FA "Profiler");

    // Display FPS
    ImGui::Text("FPS: %.1f", fps);

    ImPlotFlags plotFlags = ImPlotFlags_NoTitle;
    ImPlotAxisFlags xAxisFlags = ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax;
    ImPlotAxisFlags yAxisFlags = ImPlotAxisFlags_None | ImPlotAxisFlags_LockMin;

    if (ImPlot::BeginPlot("Performance Profiling", ImVec2(-1, 0), plotFlags))
    {
        ImPlot::SetupAxes("Time (s)", "Time (ms)", xAxisFlags, yAxisFlags);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 100.0);
        ImPlot::SetupAxisLimits(ImAxis_X1, -HISTORY_DURATION, 0.0);

        for (auto& entr : entries)
        {
            entry& e = entr.second;

            // Calculate the X-axis values as time offsets from "now"
            std::vector<float> x_data(e.timestamps.size());
            for (size_t i = 0; i < e.timestamps.size(); ++i)
            {
                x_data[i] = -std::chrono::duration<float>(now - e.timestamps[i]).count();
            }

            // Convert std::deque to a vector for Y-axis (durations)
            std::vector<float> y_data(e.history.begin(), e.history.end());

            // Plot shaded area and line independently
            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
            ImPlot::PlotShaded(entr.first.c_str(), x_data.data(), y_data.data(), static_cast<int>(y_data.size()), 0.0f);
            ImPlot::PlotLine(entr.first.c_str(), x_data.data(), y_data.data(), static_cast<int>(y_data.size()));
        }

        ImPlot::EndPlot();
    }

    // Display FPS history just like the above
    if (ImPlot::BeginPlot("FPS", ImVec2(-1, 0), plotFlags))
    {
        ImPlot::SetupAxes("Time (s)", "FPS", xAxisFlags, yAxisFlags);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 300.0);
        ImPlot::SetupAxisLimits(ImAxis_X1, -HISTORY_DURATION, 0.0);

        std::vector<float> x_data(fpsHistory.size());
        std::vector<float> y_data(fpsHistory.size());
        for (size_t i = 0; i < fpsHistory.size(); ++i)
        {
            x_data[i] = -std::chrono::duration<float>(now - fpsHistory[i].timestamp).count();
            y_data[i] = fpsHistory[i].fps;
        }

        ImPlot::PlotShaded("FPS", x_data.data(), y_data.data(), static_cast<int>(y_data.size()), 0.0f);
        ImPlot::PlotLine("FPS", x_data.data(), y_data.data(), static_cast<int>(y_data.size()));

        ImPlot::EndPlot();
    }

    ImGui::End();

    // Reset durations and counts after plotting
    for (auto& entr : entries)
    {
        entry& e = entr.second;
        e.duration = duration_t::zero();
        e.count = 0;
    }

    // Remove old FPS data
    while (!fpsHistory.empty() && fpsHistory.front().timestamp < maxTimestamp)
    {
        fpsHistory.pop_front();
    }
}

bee::profiler::time_t bee::profiler::now() { return NOW; }



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/