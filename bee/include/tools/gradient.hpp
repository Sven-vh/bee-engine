#pragma once
#include "common.hpp"

namespace bee
{
struct ColorGradient
{
    using color = glm::vec4;
    std::vector<float> positions;
    std::vector<color> colors;

    void addColor(const float position, const color& color);
    void clear();

    color getColor(const float position) const;

    bool OnImGuiRender();

    template <class Archive>
    void save(Archive& archive) const
    {
        archive(positions, colors);
    }

    template <class Archive>
    void load(Archive& archive)
    {
        archive(positions, colors);
    }
};
}  // namespace bee



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/