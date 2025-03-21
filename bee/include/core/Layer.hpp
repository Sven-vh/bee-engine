// implemented from my own small engine project, which is inspired by the Hazel engine
#pragma once
#include <string>
#include "events/Event.hpp"

// ignore warning C4100
#ifdef BEE_PLATFORM_PC
#pragma warning(push)
#pragma warning(disable : 4100)
#endif

namespace bee
{
class Layer
{
public:
    Layer(const std::string& name = "Layer") : m_name(name) {}
    virtual ~Layer() = default;

    virtual void OnAttach() = 0;
    virtual void OnDetach() = 0;
    virtual void OnRender() {}
    virtual void OnEngineInit() {}
    virtual void OnImGuiRender() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnFixedUpdate(float deltaTime) {}

    virtual void OnEvent(Event& e) {}

    inline const std::string& GetName() const { return m_name; }

    bool loaded = false;

protected:
    std::string m_name;
};
}  // namespace bee

#ifdef BEE_PLATFORM_PC
#pragma warning(pop)
#endif