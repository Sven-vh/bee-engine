// implemented from my own small engine project, which is inspired ny the Hazel engine
#include "core/LayerStack.hpp"

bee::LayerStack::LayerStack() { layerInsertIndex = 0; }

bee::LayerStack::~LayerStack()
{
    for (Layer* layer : layers)
    {
        delete layer;
    }
}

void bee::LayerStack::PushLayer(Layer* layer)
{
    layers.emplace(layers.begin() + layerInsertIndex, layer);
    layerInsertIndex++;
}

void bee::LayerStack::PopLayer(Layer* layer)
{
    auto it = std::find(layers.begin(), layers.begin() + layerInsertIndex, layer);
    if (it != layers.begin() + layerInsertIndex)
    {
        layers.erase(it);
        layerInsertIndex--;
    }
}

void bee::LayerStack::PushOverlay(Layer* overlay) { layers.emplace_back(overlay); }

void bee::LayerStack::PopOverlay(Layer* overlay)
{
    auto it = std::find(layers.begin() + layerInsertIndex, layers.end(), overlay);
    if (it != layers.end())
    {
        layers.erase(it);
    }
}
