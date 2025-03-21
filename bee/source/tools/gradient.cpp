#include "tools/gradient.hpp"
#include "core.hpp"
#include "tools/imguiHelper.hpp"

void bee::ColorGradient::addColor(const float position, const glm::vec4& color)
{
    positions.push_back(position);
    colors.push_back(color);
}

void bee::ColorGradient::clear()
{
    positions.clear();
    colors.clear();
}

glm::vec4 bee::ColorGradient::getColor(const float position) const
{
    if (positions.empty()) return color(0, 0, 0, 1);

    if (position <= positions.front()) return colors.front();
    if (position >= positions.back()) return colors.back();

    for (size_t i = 0; i < positions.size() - 1; ++i)
    {
        if (position >= positions[i] && position <= positions[i + 1])
        {
            float t = (position - positions[i]) / (positions[i + 1] - positions[i]);
            return colors[i] * (1.0f - t) + colors[i + 1] * t;
        }
    }

    return color(0, 0, 0, 1);
}

bool bee::ColorGradient::OnImGuiRender()
{
    bool changed = false;  // Flag to track changes

    ImGui::Text("Gradient Preview");

    // Draw the gradient visualization
    ImVec2 gradientSize = ImVec2(300, 20);
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    for (int i = 0; i < gradientSize.x; ++i)
    {
        float t = static_cast<float>(i) / gradientSize.x;
        glm::vec4 col = getColor(t);  // Use getColor function to interpolate color at position t
        ImU32 color32 = ImGui::ColorConvertFloat4ToU32(ImVec4(col.r, col.g, col.b, col.a));
        drawList->AddRectFilled(ImVec2(canvasPos.x + i, canvasPos.y),
                                ImVec2(canvasPos.x + i + 1, canvasPos.y + gradientSize.y),
                                color32);
    }
    ImGui::Dummy(ImVec2(gradientSize.x, gradientSize.y));  // Reserve space for visualization

    // To store user inputs for new color position and value
    static float newPosition = 0.5f;
    static glm::vec4 newColor = glm::vec4(1, 1, 1, 1);  // White color initially

    // Display existing colors and allow modification
    for (size_t i = 0; i < positions.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));  // To uniquely identify controls

        float& position = positions[i];
        glm::vec4& color = colors[i];

        // Allow modification of position and color
        if (bee::ImGuiHelper::DragControl("Position", &position, 0.0f, 1.0f)) changed = true;
        if (bee::ImGuiHelper::Color("Color", color)) changed = true;

        if (ImGui::Button("Remove"))
        {
            positions.erase(positions.begin() + i);
            colors.erase(colors.begin() + i);
            --i;  // Adjust index after removal
            changed = true;
        }

        ImGui::PopID();
    }

    // Sort the colors based on their positions after any changes
    std::vector<std::pair<float, glm::vec4>> sortedColors;
    for (size_t i = 0; i < positions.size(); ++i)
    {
        sortedColors.emplace_back(positions[i], colors[i]);
    }

    // Sort the vector based on positions
    std::sort(sortedColors.begin(),
              sortedColors.end(),
              [](const std::pair<float, glm::vec4>& a, const std::pair<float, glm::vec4>& b) { return a.first < b.first; });

    // Update the positions and colors vectors to be in sorted order
    for (size_t i = 0; i < sortedColors.size(); ++i)
    {
        if (positions[i] != sortedColors[i].first || colors[i] != sortedColors[i].second) changed = true;

        positions[i] = sortedColors[i].first;
        colors[i] = sortedColors[i].second;
    }

    ImGui::Separator();

    // Controls for adding a new color
    ImGui::Text("Add New Color");
    if (bee::ImGuiHelper::DragControl("Position", &newPosition, 0.0f, 1.0f)) changed = true;
    if (bee::ImGuiHelper::Color("Color", newColor)) changed = true;
    if (ImGui::Button("Add Color"))
    {
        // Check if the position is already occupied
        bool positionExists = false;
        for (const auto& pos : positions)
        {
            if (pos == newPosition)
            {
                positionExists = true;
                break;
            }
        }

        if (positionExists)
        {
            ImGui::Text("Cannot add color at this position, already occupied.");
        }
        else
        {
            addColor(newPosition, newColor);  // Add the new color
            changed = true;
        }
    }

    ImGui::Separator();

    return changed;  // Return the change flag
}