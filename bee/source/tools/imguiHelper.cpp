#include "tools/imguiHelper.hpp"
#include "core.hpp"

using namespace bee;

// Define other non-template functions

ImGuiHelper::RightClickOption ImGuiHelper::ItemLabel(const std::string& title, ItemLabelFlag flags)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImVec2 lineStart = ImGui::GetCursorScreenPos();
    const ImGuiStyle& style = ImGui::GetStyle();
    float fullWidth = ImGui::GetContentRegionAvail().x;
    float itemWidth = ImGui::CalcItemWidth() + style.ItemSpacing.x;
    ImVec2 textSize = ImGui::CalcTextSize(title.data(), title.data() + title.size(), false, 0.0f);
    ImRect textRect;
    textRect.Min = ImGui::GetCursorScreenPos();
    if (flags & ItemLabelFlag::Right) textRect.Min.x += itemWidth;
    textRect.Max = textRect.Min;
    textRect.Max.x += fullWidth - itemWidth;
    textRect.Max.y += textSize.y;

    ImGui::SetCursorScreenPos(textRect.Min);

    ImGui::AlignTextToFramePadding();
    // Adjust text rect manually because we render it directly into a drawlist instead of using public functions.
    textRect.Min.y += window->DC.CurrLineTextBaseOffset;
    textRect.Max.y += window->DC.CurrLineTextBaseOffset;

    std::string popUpLabel = "ItemLabelPopup##" + title;

    ImGui::ItemSize(textRect);
    if (ImGui::ItemAdd(textRect, window->GetID(title.c_str())))
    {
        ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(),
                                  textRect.Min,
                                  textRect.Max,
                                  textRect.Max.x,
                                  textRect.Max.x,
                                  title.data(),
                                  title.data() + title.size(),
                                  &textSize);

        if (textRect.GetWidth() < textSize.x && ImGui::IsItemHovered())
            ImGui::SetTooltip("%.*s", static_cast<int>(title.size()), title.data());

        // Check for right-click and open a pop-up
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup(popUpLabel.c_str());
        }
    }
    if (flags & ItemLabelFlag::Left)
    {
        ImGui::SetCursorScreenPos(ImVec2{0 - textRect.Max.x, textSize.y + window->DC.CurrLineTextBaseOffset - textRect.Max.y});
        ImGui::SameLine();
    }
    else if (flags & ItemLabelFlag::Right)
    {
        ImGui::SetCursorScreenPos(lineStart);
    }

    RightClickOption option = RightClickOption::None;
    if (ImGui::BeginPopup(popUpLabel.c_str()))
    {
        if (ImGui::Selectable("Copy"))
        {
            bee::Log::Info("Copy");
            option = RightClickOption::Copy;
        }
        if (ImGui::Selectable("Paste"))
        {
            bee::Log::Info("Paste");
            option = RightClickOption::Paste;
        }
        if (ImGui::Selectable("Reset"))
        {
            bee::Log::Info("Reset");
            option = RightClickOption::Reset;
        }
        ImGui::EndPopup();
    }

    return option;
}

bool ImGuiHelper::DragControlWithPadding(const char* label, float* value, float min, float max)
{
    bool updated = false;
    float before = *value;
    RightClickOption option = ItemLabel(label, ItemLabelFlag::Left);

    DefaultOptionCode(option, *value);

    updated = ImGui::SliderFloat(("##" + std::string(label)).c_str(), value, min, max, "%.3f", ImGuiSliderFlags_None);

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        auto command = CreateScope<TypeCommand<float>>(*value, before, *value);
        bee::UndoRedoManager::Execute(std::move(command));
    }

    return updated;
}

bool ImGuiHelper::Checkbox(const char* label, bool* value)
{
    bool updated = false;

    RightClickOption option = ItemLabel(label, ItemLabelFlag::Left);

    DefaultOptionCode(option, *value);

    bool before = *value;
    updated = ImGui::Checkbox(("##" + std::string(label)).c_str(), value);

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        auto command = CreateScope<TypeCommand<bool>>(*value, before, *value);
        bee::UndoRedoManager::Execute(std::move(command));
    }
    return updated;
}

bool ImGuiHelper::Color(const char* label, glm::vec4& color)
{
    bool updated = false;

    RightClickOption option = ItemLabel(label, ItemLabelFlag::Left);

    DefaultOptionCode(option, color);

    static auto initialColor = glm::vec4(-1.0f);  // To track the initial state

    updated = ImGui::ColorEdit4(("##" + std::string(label)).c_str(), glm::value_ptr(color));

    if (ImGui::IsItemActivated() && initialColor == glm::vec4(-1.0f))  // Detect when editing starts
    {
        initialColor = color;  // Store the initial color before editing
    }

    if (ImGui::IsItemDeactivatedAfterEdit())  // When the user stops editing
    {
        if (initialColor != color)  // Only execute if there's an actual change
        {
            auto command = CreateScope<TypeCommand<glm::vec4>>(color, initialColor, color);
            bee::UndoRedoManager::Execute(std::move(command));
        }
        initialColor = glm::vec4(-1.0f);  // Reset initialColor after edit completes
    }

    return updated;
}

bool ImGuiHelper::InputText(const char* label, std::string& value)
{
    bool updated = false;

    RightClickOption option = ItemLabel(label, ItemLabelFlag::Left);

    DefaultOptionCode(option, value);

    std::string before = value;
    std::string temp = value;
    updated = ImGui::InputText(("##" + std::string(label)).c_str(), &temp);

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        value = temp;
        auto command = CreateScope<TypeCommand<std::string>>(value, before, value);
        bee::UndoRedoManager::Execute(std::move(command));
    }

    return updated;
}

bool ImGuiHelper::Vec3(const char* label, glm::vec3& values)
{
    bool updated = false;

    glm::vec3 before = values;
    RightClickOption option = ItemLabel(label, ItemLabelFlag::Left);

    DefaultOptionCode(option, values);

    updated |= ImGui::DragFloat3(("##" + std::string(label)).c_str(), glm::value_ptr(values), 0.1f);

    bool isUsing = ImGui::IsItemActive();
    static auto initial = glm::vec3(-1.0f);

    if (isUsing && initial == glm::vec3(-1.0f))
    {
        initial = before;
    }

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        auto command = CreateScope<TypeCommand<glm::vec3>>(values, initial, values);
        bee::UndoRedoManager::Execute(std::move(command));
        initial = glm::vec3(-1.0f);
    }

    return updated;
}

bool ImGuiHelper::PlayButton(const char* label, ImVec2 size)
{
    ImGui::InvisibleButton(label, size);
    bool isClicked = ImGui::IsItemClicked();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    bool isHovered = ImGui::IsItemHovered();

    ImVec2 pos = ImGui::GetItemRectMin();
    ImVec2 end_pos = ImGui::GetItemRectMax();
    ImVec2 center = ImVec2((pos.x + end_pos.x) * 0.5f, (pos.y + end_pos.y) * 0.5f);

    float triangle_size = ImMin(size.x, size.y) * 0.4f;
    ImVec2 p1 = ImVec2(center.x - triangle_size * 0.5f, center.y - triangle_size);
    ImVec2 p2 = ImVec2(center.x - triangle_size * 0.5f, center.y + triangle_size);
    ImVec2 p3 = ImVec2(center.x + triangle_size, center.y);

    drawList->AddTriangleFilled(p1, p2, p3, isHovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(150, 150, 150, 255));

    return isClicked;
}

bool ImGuiHelper::SquareButton(const char* label, ImVec2 size)
{
    ImGui::InvisibleButton(label, size);
    bool is_clicked = ImGui::IsItemClicked();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    bool isHovered = ImGui::IsItemHovered();

    ImVec2 pos = ImGui::GetItemRectMin();
    ImVec2 end_pos = ImGui::GetItemRectMax();

    draw_list->AddRectFilled(pos, end_pos, isHovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(150, 150, 150, 255));

    return is_clicked;
}

ImVec2 bee::ImGuiHelper::GetMaxImageSize(const float imageWidth, const float imageHeight)
{
    ImVec2 final_size;
    // Get the size of the ImGui window
    ImVec2 window_size = ImGui::GetContentRegionAvail();

    float texture_aspect = imageWidth / imageHeight;
    float window_aspect = window_size.x / window_size.y;

    if (window_aspect > texture_aspect)
    {
        final_size.y = window_size.y;
        final_size.x = final_size.y * texture_aspect;
    }
    else
    {
        final_size.x = window_size.x;
        final_size.y = final_size.x / texture_aspect;
    }

    return final_size;
}



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/