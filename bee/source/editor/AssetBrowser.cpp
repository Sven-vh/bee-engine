// AssetBrowser.cpp
#include "editor/AssetBrowser.hpp"
#include "core.hpp"

AssetBrowser::AssetBrowser() : m_iconSize(128), m_selectedAssetEntity(entt::null) {}

void AssetBrowser::OnImGuiRender()
{
    ImGui::Begin(ICON_FA_CHESS_ROOK TAB_FA "Asset Browser");

    RenderSettings();
    RenderAssets();

    ImGui::End();
}

void AssetBrowser::ClearSelection() { m_selectedAssetEntity = entt::null; }

void AssetBrowser::RenderSettings()
{
    ImGui::Text("Settings:");
    ImGui::SameLine();
    ImGui::PushItemWidth(100);
    ImGui::SliderInt("Icon Width", &m_iconSize, 32, 512);
    ImGui::PopItemWidth();  // Ensure to pop the item width
    ImGui::Separator();

    if (ImGui::Button("Load Assets"))
    {
        std::vector<fs::path> paths = bee::FileDialog::OpenFiles(FILE_MULTIPLE_FILTER("GLTF Files", ".gltf", ".glb"));
        if (!paths.empty())
        {
            LoadAssets(paths);
        }
    }
}

void AssetBrowser::RenderAssets()
{
    auto& registry = bee::Engine.Registry();
    auto view = registry.view<bee::AssetItem>();

    float windowWidth = ImGui::GetContentRegionAvail().x;
    int itemsPerRow = std::max(1, static_cast<int>(std::floor(windowWidth / static_cast<float>(m_iconSize))));

    int currentItem = 0;

    for (auto entity : view)
    {
        auto& assetItem = view.get<bee::AssetItem>(entity);
        ImGui::PushID(static_cast<int>(entity));

        if (currentItem % itemsPerRow != 0) ImGui::SameLine();

        bool isSelected = (m_selectedAssetEntity == entity);

        // Get the current cursor position for background drawing
        ImVec2 itemPos = ImGui::GetCursorScreenPos();
        ImVec2 itemSize(static_cast<float>(m_iconSize), static_cast<float>(m_iconSize) + ImGui::GetTextLineHeightWithSpacing());

        // If the item is selected, draw a background rectangle
        if (isSelected)
        {
            float padding = 7.5f;
            ImVec4 defaultSelectedColor = ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered);
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(itemPos.x - padding, itemPos.y - padding),
                ImVec2(itemPos.x + itemSize.x + padding, itemPos.y + itemSize.y + padding),
                ImGui::GetColorU32(defaultSelectedColor));
        }

        ImGui::BeginGroup();

        ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(assetItem.frameBuffer->GetColorAttachmentRendererID())),
                     ImVec2(static_cast<float>(m_iconSize), static_cast<float>(m_iconSize)),
                     ImVec2(0, 1),
                     ImVec2(1, 0));

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_selectedAssetEntity = entity;
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            bee::Log::Info("Asset clicked: {0}", assetItem.path.string());

            // Load the GLTF file
            auto gltfPath = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::None, assetItem.path);
            entt::entity newEntity = bee::resource::LoadGLTF(gltfPath, bee::Engine.Registry());

            // Get the transform of the entity and set the position in front of the camera
            auto& transform = bee::Engine.Registry().get<bee::Transform>(newEntity);
            auto& cameraTransform = bee::Engine.Registry().get<bee::Transform>(bee::Engine.EditorCamera());

            glm::vec3 cameraPosition = cameraTransform.GetPosition();
            glm::vec3 cameraDirection = cameraTransform.GetDirection();

            glm::vec3 newPosition =
                cameraPosition + glm::normalize(glm::vec3(cameraDirection.x, 0.0f, cameraDirection.z)) * 5.0f;
            newPosition.y = 0.0f;  // Set y to 0

            transform.SetPosition(glm::round(newPosition));

            // Select the new entity
            ClearSelection();
        }

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + static_cast<float>(m_iconSize));
        ImGui::TextWrapped("%s", bee::FileIO::GetFileName(assetItem.path).c_str());
        ImGui::PopTextWrapPos();

        ImGui::EndGroup();
        ImGui::PopID();

        ++currentItem;
    }
}

// maybe add a temp component
void AssetBrowser::LoadAsset(const std::filesystem::path& path)
{
    // Load the GLTF entity
    auto gltfPath = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::None, path.string());
    gltfPath = bee::Engine.FileIO().GetRelativePath(bee::FileIO::Directory::Root, gltfPath);

    entt::entity newEntity = bee::resource::LoadGLTF(gltfPath, bee::Engine.Registry());

    // Create and initialize the asset entity
    entt::entity assetEntity = bee::ecs::CreateEmpty();
    bee::Engine.Registry().emplace<bee::AssetItem>(assetEntity, m_iconSize, m_iconSize, gltfPath);
    bee::Engine.Registry().emplace<bee::EditorComponent>(assetEntity);

    bee::AssetItem& assetItem = bee::Engine.Registry().get<bee::AssetItem>(assetEntity);
    bee::ecs::RenderEntity(bee::Engine.Registry(), newEntity, assetItem.frameBuffer);

    auto& hierarchy = bee::Engine.Registry().get<bee::HierarchyNode>(newEntity);
    hierarchy.DestroyChildren(bee::Engine.Registry());
    bee::Engine.Registry().destroy(newEntity);
}

void AssetBrowser::LoadAssets(const std::vector<std::filesystem::path>& paths)
{
    for (const auto& path : paths)
    {
        LoadAsset(path);
    }
}


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/