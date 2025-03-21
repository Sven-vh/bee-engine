#pragma once
#include "common.hpp"
#include "xsr/include/xsr.hpp"

namespace bee
{
class RenderManager
{
public:
    static void Initialize();

    static void SubmitRenderables(entt::registry& registry);
    static void SubmitUIRenderables(entt::registry& registry);
    static void SubmitBillboards(const entt::entity cameraEntity, entt::registry& registry);
    static void SubmitLights(entt::registry& registry);
    static void SubmitSceneData(entt::registry& registry);

    static void ClearEntries();
    static void Render(const entt::entity cameraEntity, entt::registry& registry);
    static void RenderUI(const entt::entity cameraEntity, entt::registry& registry);

private:
    static const glm::mat4& GetWorldModelCache(const entt::entity entity, entt::registry& registry);

    static xsr::shader_handle CreateDefaultShader();
    static xsr::shader_handle CreateNormalShader();
    static xsr::shader_handle CreateUVShader();
    static xsr::shader_handle CreateTextureShader();
    static xsr::shader_handle CreateVertexColorShader();

private:
    static std::unordered_map<entt::entity, glm::mat4> m_worldModelCache;
    static std::unordered_map<RenderMode, xsr::shader_handle> m_shaders;
};
}  // namespace bee