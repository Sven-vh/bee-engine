#pragma once
#include "common.hpp"

#define M_PI 3.14159265358979323846

namespace bee
{

/// <summary>
/// Linearly interpolates between two values.
/// </summary>
template <class T>
T Lerp(T a, T b, float t)
{
    t = std::max(0.0f, std::min(t, 1.0f));
    return a * (1.0f - t) + b * t;
}

/// <summary>
/// Returns the parameter t that was used to linearly interpolate between a and b.
/// </summary>
template <class T>
float InvLerp(T a, T b, T v)
{
    float t = (v - a) / (b - a);
    t = std::max(0.0f, std::min(t, 1.0f));
    return t;
}

/// <summary>
/// Remaps a value from one range to another.
/// </summary>
/// <param name="iF">Input range from</param>
/// <param name="iT">Input range to</param>
/// <param name="oF">Output range from</param>
/// <param name="oT">Output range to</param>
/// <param name="v">Value to remap</param>
template <class T>
T Remap(T iF, T iT, T oF, T oT, T v)
{
    float t = InvLerp(iF, iT, v);
    return Lerp(oF, oT, t);
}

/// <summary>
/// Framerate-independent exponential dampening.
/// </summary>
/// <typeparam name="T">Any type on which Lerp and InvLerp are defined.</typeparam>
/// <param name="a">From parameter</param>
/// <param name="b">To parameter</param>
/// <param name="lambda">Speed of dampening</param>
/// <param name="dt">Time since last frame</param>
template <class T>
T Damp(T a, T b, float lambda, float dt)
{
    return Lerp(a, b, 1 - exp(-lambda * dt));
}

// random direction in a sphere
static inline glm::vec3 RandomDirectionInSphere()
{
    float theta = glm::linearRand(0.0f, 2.0f * glm::pi<float>());
    float phi = glm::linearRand(0.0f, glm::pi<float>());
    return glm::vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
}

static inline float RandomFloat(float min, float max) { return glm::linearRand(min, max); }

static inline glm::vec3 RandomDirectionInCone(const glm::vec3& direction, const float angle)
{
    // Convert angle from degrees to radians
    float radianAngle = glm::radians(angle);

    // Generate a random angle within the cone
    float theta = glm::linearRand(0.0f, 2.0f * glm::pi<float>());
    float phi = glm::linearRand(0.0f, radianAngle);

    // Spherical coordinates to Cartesian coordinates conversion
    float x = sin(phi) * cos(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(phi);

    // Generate a random vector within the unit sphere in the cone's direction
    glm::vec3 randomDir = glm::vec3(x, y, z);

    // Align the cone direction with the z-axis and rotate the random direction to match the original direction
    glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::quat rotation = glm::rotation(up, glm::normalize(direction));
    randomDir = rotation * randomDir;

    return glm::normalize(randomDir);
}
}  // namespace bee
