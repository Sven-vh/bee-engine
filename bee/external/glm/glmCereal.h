#pragma once
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <glm/glm.hpp>

namespace glm
{

template <class Archive>
void serialize(Archive& archive, glm::vec3& vec)
{
    archive(vec.x, vec.y, vec.z);  // Serialize each component of glm::vec3
}

template <class Archive>
void serialize(Archive& archive, glm::vec4& vec)
{
    archive(vec.x, vec.y, vec.z, vec.w);  // Serialize each component of glm::vec4
}

template <class Archive>
void serialize(Archive& archive, glm::quat& quat)
{
    archive(quat.x, quat.y, quat.z, quat.w);  // Serialize each component of glm::quat
}

template <class Archive>
void serialize(Archive& archive, glm::mat4& mat)
{
    for (int i = 0; i < 4; i++)
    {
        archive(mat[i][0], mat[i][1], mat[i][2], mat[i][3]);  // Serialize each component of glm::mat4
    }
}

template <class Archive>
void serialize(Archive& archive, glm::ivec2& vec)
{
    archive(vec.x, vec.y);  // Serialize each component of glm::ivec2
}

}  // namespace glm
