#pragma once
#include "common.hpp"
#include "managers/undo_redo_manager.hpp"

namespace bee
{

class ImGuiHelper
{
public:
    // Enumerations
    enum class RightClickOption
    {
        None,
        Copy,
        Paste,
        Reset,
    };

    enum ItemLabelFlag
    {
        Left = 1u << 0u,
        Right = 1u << 1u,
        Default = Left,
    };

    // Template functions
    template <typename Type>
    static void CopyToClipboard(const Type& value);

    template <typename Type>
    static void PasteFromClipboard(Type& value);

    // Enum to string conversion
    template <typename T>
    static const char* EnumToString(T value);

    // Enum combo box
    template <typename T>
    static bool EnumCombo(const char* label, T& currentEnum);

    // Item label with right-click options
    static RightClickOption ItemLabel(const std::string& title, ItemLabelFlag flags);

    // Drawing controls
    static bool DragControlWithPadding(const char* label, float* value, float min = 0.0f, float max = 100.0f);

    template <typename T>
    static bool DragControl(const char* label, T* value, float speed = 0.1f, T min = 0, T max = 0);
    static bool Checkbox(const char* label, bool* value);
    static bool Color(const char* label, glm::vec4& color);
    static bool InputText(const char* label, std::string& value);
    static bool Vec3(const char* label, glm::vec3& values);

    static bool PlayButton(const char* label, ImVec2 size);
    static bool SquareButton(const char* label, ImVec2 size);

    static ImVec2 GetMaxImageSize(const float imageWidth, const float imageHeight);

private:
    // Macro converted to a private static function
    template <typename Type>
    static void DefaultOptionCode(RightClickOption option, Type& value);
};

template <typename Type>
inline void ImGuiHelper::CopyToClipboard(const Type& value)
{
    try
    {
        std::stringstream ss;
        {
            cereal::JSONOutputArchive archive(ss);
            archive(value);
        }
        ImGui::SetClipboardText(ss.str().c_str());
    }
    catch (const std::exception& e)
    {
        bee::Log::Error("Failed to copy to clipboard: ", e.what());
    }
}

template <typename Type>
inline void ImGuiHelper::PasteFromClipboard(Type& value)
{
    try
    {
        const char* clipboardText = ImGui::GetClipboardText();
        if (clipboardText != nullptr)
        {
            std::stringstream ss(clipboardText);
            {
                cereal::JSONInputArchive archive(ss);
                archive(value);
            }
        }
    }
    catch (const std::exception& e)
    {
        bee::Log::Error("Failed to paste from clipboard: ", e.what());
    }
}

template <typename T>
inline const char* ImGuiHelper::EnumToString(T value)
{
    // This should be specialized for each enum type
    static_assert(sizeof(T) == 0, "EnumToString not specialized for this type.");
    return "";
}

template <typename T>
inline bool ImGuiHelper::EnumCombo(const char* label, T& currentEnum)
{
    // Get the count of enum values
    const int enumCount = static_cast<int>(T::Count);

    // Create a list of strings for display
    std::vector<const char*> items;
    items.reserve(enumCount);

    for (int i = 0; i < enumCount; ++i)
    {
        items.push_back(EnumToString(static_cast<T>(i)));
    }

    // Current index
    int currentIndex = static_cast<int>(currentEnum);

    // Render the combo box
    if (ImGui::Combo(label, &currentIndex, items.data(), enumCount))
    {
        // Update enum value based on user selection
        currentEnum = static_cast<T>(currentIndex);
        return true;  // Return true if the value changed
    }

    return false;  // Return false if the value did not change
}

template <typename T>
inline bool ImGuiHelper::DragControl(const char* label, T* value, float speed, T min, T max)
{
    bool updated = false;

    RightClickOption option = ItemLabel(label, ItemLabelFlag::Left);

    DefaultOptionCode(option, *value);

    T before = *value;
    updated = ImGui::DragScalar(("##" + std::string(label)).c_str(),
                                ImGuiDataType_((sizeof(T) == sizeof(float)) ? ImGuiDataType_Float : ImGuiDataType_S32),
                                value,
                                speed,
                                &min,
                                &max);

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        auto command = CreateScope<TypeCommand<T>>(*value, before, *value);
        bee::UndoRedoManager::Execute(std::move(command));
        return true;
    }

    return updated;
}

template <typename Type>
inline void ImGuiHelper::DefaultOptionCode(RightClickOption option, Type& value)
{
    switch (option)
    {
        case RightClickOption::Copy:
            CopyToClipboard<Type>(value);
            break;
        case RightClickOption::Paste:
            PasteFromClipboard<Type>(value);
            break;
        case RightClickOption::Reset:
            value = Type{};
            break;
        default:
            break;
    }
}

template <>
inline const char* ImGuiHelper::EnumToString<EaseType>(EaseType value)
{
    switch (value)
    {
        case EaseType::Linear:
            return "Linear";
        case EaseType::EaseInQuad:
            return "EaseInQuad";
        case EaseType::EaseOutQuad:
            return "EaseOutQuad";
        case EaseType::EaseInOutQuad:
            return "EaseInOutQuad";
        case EaseType::EaseInCubic:
            return "EaseInCubic";
        case EaseType::EaseOutCubic:
            return "EaseOutCubic";
        case EaseType::EaseInOutCubic:
            return "EaseInOutCubic";
        case EaseType::EaseInQuart:
            return "EaseInQuart";
        case EaseType::EaseOutQuart:
            return "EaseOutQuart";
        case EaseType::EaseInOutQuart:
            return "EaseInOutQuart";
        case EaseType::EaseInQuint:
            return "EaseInQuint";
        case EaseType::EaseOutQuint:
            return "EaseOutQuint";
        case EaseType::EaseInOutQuint:
            return "EaseInOutQuint";
        default:
            throw std::invalid_argument("Unsupported EaseType.");
    }
}

template <>
inline const char* ImGuiHelper::EnumToString<RenderMode>(RenderMode value)
{
    switch (value)
    {
        case RenderMode::Standard:
            return ICON_FA_PANORAMA TAB_FA "Standard";
        case RenderMode::Wireframe:
            return ICON_FA_EXPAND TAB_FA "Wireframe";
        case RenderMode::Normal:
            return ICON_FA_UPLOAD TAB_FA "Normal";
        case RenderMode::UV:
            return ICON_FA_CIRCLE_HALF_STROKE TAB_FA "UV";
        case RenderMode::Texture:
            return ICON_FA_FILE_IMAGE TAB_FA "Texture";
        case RenderMode::VertexColor:
            return ICON_FA_PAINT_BRUSH TAB_FA "Vertex Color";
        default:
            throw std::invalid_argument("Unsupported RenderMode.");
    }
}

template <>
inline const char* ImGuiHelper::EnumToString<Anchor>(Anchor value)
{
    switch (value)
    {
        case Anchor::TopLeft:
            return ICON_FA_ARROW_UP_LEFT TAB_FA "Top Left";
        case Anchor::TopCenter:
            return ICON_FA_ARROW_UP TAB_FA "Top Center";
        case Anchor::TopRight:
            return ICON_FA_ARROW_UP_RIGHT TAB_FA "Top Right";
        case Anchor::CenterLeft:
            return ICON_FA_ARROW_LEFT TAB_FA "Center Left";
        case Anchor::Center:
            return ICON_FA_CIRCLE TAB_FA "Center";
        case Anchor::CenterRight:
            return ICON_FA_ARROW_RIGHT TAB_FA "Center Right";
        case Anchor::BottomLeft:
            return ICON_FA_ARROW_DOWN_LEFT TAB_FA "Bottom Left";
        case Anchor::BottomCenter:
            return ICON_FA_ARROW_DOWN TAB_FA "Bottom Center";
        case Anchor::BottomRight:
            return ICON_FA_ARROW_DOWN_RIGHT TAB_FA "Bottom Right";
        default:
            throw std::invalid_argument("Unsupported Anchor.");
    }
}

}  // namespace bee



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/