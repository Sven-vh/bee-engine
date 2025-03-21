#pragma once
#include "core/Layer.hpp"

namespace bee
{
class ImGuiLayer : public bee::Layer
{
public:
    ImGuiLayer();
    ~ImGuiLayer();

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnEngineInit() override;
    virtual void Begin();
    virtual void End();

    // virtual void OnEvent(events::Event& e) override;

private:
#ifdef EDITOR_MODE
    bool m_pushedStyle = false;
#endif
};
}  // namespace bee
