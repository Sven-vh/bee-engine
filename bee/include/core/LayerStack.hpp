// implemented from my own small engine project, which is inspired ny the Hazel engine
#pragma once
#include <vector>
#include "Layer.hpp"

namespace bee
{
class LayerStack
{
public:
    LayerStack();
    ~LayerStack();

    void PushLayer(Layer* layer);
    void PopLayer(Layer* layer);
    void PushOverlay(Layer* overlay);
    void PopOverlay(Layer* overlay);

    std::vector<Layer*>::iterator begin() { return layers.begin(); }
    std::vector<Layer*>::iterator end() { return layers.end(); }
    std::vector<Layer*>::reverse_iterator rbegin() { return layers.rbegin(); }
    std::vector<Layer*>::reverse_iterator rend() { return layers.rend(); }

    std::vector<Layer*>::const_iterator begin() const { return layers.begin(); }
    std::vector<Layer*>::const_iterator end() const { return layers.end(); }
    std::vector<Layer*>::const_reverse_iterator rbegin() const { return layers.rbegin(); }
    std::vector<Layer*>::const_reverse_iterator rend() const { return layers.rend(); }

    template <class derived>
    derived* GetLayer()
    {
        for (Layer* layer : layers)
        {
            if (dynamic_cast<derived*>(layer))
            {
                return static_cast<derived*>(layer);
            }
        }
        return nullptr;
    }

private:
    std::vector<Layer*> layers;
    unsigned int layerInsertIndex = 0;
};
}  // namespace bee



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/