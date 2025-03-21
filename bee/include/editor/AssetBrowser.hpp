#pragma once
#include "core.hpp"

class AssetBrowser
{
public:
    AssetBrowser();
    ~AssetBrowser() = default;

    void OnImGuiRender();

    entt::entity GetSelected() const { return m_selectedAssetEntity; }

private:
    int m_iconSize;
    entt::entity m_selectedAssetEntity;

private:
    void RenderSettings();
    void RenderAssets();
    void ClearSelection();

    void LoadAsset(const std::filesystem::path& path);
    void LoadAssets(const std::vector<std::filesystem::path>& paths);
};