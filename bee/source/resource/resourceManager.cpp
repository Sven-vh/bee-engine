#include "resource/resourceManager.hpp"
#include "resource/resource.hpp"
#include "resource/mesh.hpp"
#include "resource/texture.hpp"
#include "resource/gltfModel.hpp"

#if defined(EDITOR_MODE)
#define USE_WEAK_PTR 0
#else
#define USE_WEAK_PTR 1
#endif

namespace bee::resource::internal
{
#if USE_WEAK_PTR
std::unordered_map<std::string, WeakRef<Resource>> resourceMap;
#else
std::unordered_map<std::string, Ref<Resource>> resourceMap;
#endif

std::string selectedResource;

}  // namespace bee::resource::internal

using namespace bee::resource;
using namespace bee::resource::internal;

// Load a resource from disk
template <typename ResourceType>
Ref<ResourceType> bee::resource::LoadResource(const fs::path& path)
{
    // Check if resource already exists
    auto it = resourceMap.find(path.string());
    if (it != resourceMap.end())
    {
#if USE_WEAK_PTR
        if (it->second.expired())
        {
            // Resource has been deleted, remove it from the map and return nothing, let it create a new one
            resourceMap.erase(it);
        }
#else
        return std::static_pointer_cast<ResourceType>(it->second);
#endif
    }

    // Resource does not exist, create it
    Ref<ResourceType> resource = CreateRef<ResourceType>();
    if (!resource->Load(path))
    {
        // Failed to load resource
        return nullptr;
    }
    resourceMap[path.string()] = resource;

    return resource;
}

// Create a resource without loading it from disk
template <typename ResourceType>
Ref<ResourceType> bee::resource::CreateResource(const std::string& name)
{
    // Check if resource already exists
    auto it = resourceMap.find(name);
    if (it != resourceMap.end())
    {
#if USE_WEAK_PTR
        if (it->second.expired())
        {
            // Resource has been deleted, remove it from the map and return nothing, let it create a new one
            resourceMap.erase(it);
        }
#else
        return std::static_pointer_cast<ResourceType>(it->second);
#endif
    }

    // Resource does not exist, create it
    Ref<ResourceType> resource = CreateRef<ResourceType>();
    resourceMap[name] = resource;

    return resource;
}

void bee::resource::OnImGuiRender()
{
    ImGui::Begin(ICON_FA_FILE_IMAGE TAB_FA "Resource Manager");

    // Show total resources loaded
    ImGui::Text("Total Resources: %zu", resourceMap.size());
    ImGui::SameLine();
    // some space between the button and the text
    ImGui::SameLine(ImGui::GetWindowWidth() - 150);
    if (ImGui::Button("Unload unused"))
    {
        UnloadResources();
    }

    // If no resources, display a message
    if (resourceMap.empty())
    {
        ImGui::Text("No resources loaded.");
    }
    else
    {
        if (ImGui::BeginTable("ResourceTable",
                              4,
                              ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingStretchProp))
        {
            // Set up columns with weights to auto-scale
            ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch, 0.5f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.2f);
            ImGui::TableSetupColumn("Reference Count", ImGuiTableColumnFlags_WidthFixed, 0.15f);
            // remove button table
            ImGui::TableSetupColumn("Remove", ImGuiTableColumnFlags_WidthFixed, 0.15f);

            ImGui::TableHeadersRow();

            // Display information about each resource
            for (auto it = resourceMap.begin(); it != resourceMap.end();)
            {
#if USE_WEAK_PTR
                Ref<Resource> resource = it->second.lock();
#else
                auto* resource = it->second.get();
#endif
                if (!resource)
                {
                    // Resource has been deleted, continue to next item
                    ++it;
                    continue;
                }

                if (selectedResource == it->first)
                {
                    ImGui::TableNextRow();
                    // set background color to default selected color
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_ButtonActive));
                }
                else
                {
                    ImGui::TableNextRow();
                }
                ImGui::TableNextColumn();

                ImGui::Text("%s", it->first.c_str());
                // On click , set the selected resource
                if (ImGui::IsItemClicked())
                {
                    if (selectedResource == it->first)
                        selectedResource.clear();
                    else
                        selectedResource = it->first;
                }

                ImGui::TableNextColumn();
                ImGui::Text("%s", resource->ToString());

                ImGui::TableNextColumn();
                // -1 because the manager holds a reference
                ImGui::Text("%ld", it->second.use_count());

                // if the use count is 1, it means that only the manager holds a reference so make the button available
                if (it->second.use_count() == 1)
                {
                    ImGui::TableNextColumn();
                    if (ImGui::Button("Remove"))
                    {
                        it = resourceMap.erase(it);
                        continue;  // Skip the increment to avoid invalid iterator access
                    }
                }

                ++it;  // Increment iterator normally
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();

    // Show the selected resource
    if (!selectedResource.empty())
    {
        ImGui::Begin(ICON_FA_IMAGE TAB_FA "Selected Resource");
        auto it = resourceMap.find(selectedResource);
        if (it != resourceMap.end())
        {
#if USE_WEAK_PTR
            Ref<Resource> resource = it->second.lock();
#else
            auto* resource = it->second.get();
#endif
            if (resource)
            {
                resource->OnImGuiRender();
            }
            else
            {
                ImGui::Text("Resource not found.");
            }
        }
        else
        {
            ImGui::Text("Resource not found.");
        }
        ImGui::End();
    }
}

void bee::resource::UnloadResources()
{
    for (auto it = resourceMap.begin(); it != resourceMap.end();)
    {
        if (it->second.use_count() == 1)
        {
            it = resourceMap.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// Explicit instantiation for Mesh and Texture types
template Ref<bee::resource::Mesh> bee::resource::LoadResource<bee::resource::Mesh>(const fs::path& path);
template Ref<bee::resource::Texture> bee::resource::LoadResource<bee::resource::Texture>(const fs::path& path);
template Ref<bee::resource::GltfModel> bee::resource::LoadResource<bee::resource::GltfModel>(const fs::path& path);

template Ref<bee::resource::Mesh> bee::resource::CreateResource<bee::resource::Mesh>(const std::string& name);
template Ref<bee::resource::Texture> bee::resource::CreateResource<bee::resource::Texture>(const std::string& name);
template Ref<bee::resource::GltfModel> bee::resource::CreateResource<bee::resource::GltfModel>(const std::string& name);

// 21.31s to load the map
// 24.38s second time
// 15.65
// 15.5