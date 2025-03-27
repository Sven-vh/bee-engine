#include "rendering/RenderingHelper.hpp"

glm::mat4 bee::helper::LookToPosition(const glm::mat4& original,
                                      const glm::vec3& cameraPosition,
                                      const bool addRotation,
                                      const glm::vec3& customScale)
{
    glm::vec3 position = original[3];
    glm::vec3 rotation = glm::eulerAngles(glm::quat_cast(original));
    glm::vec3 scale = glm::vec3(glm::length(original[0]), glm::length(original[1]), glm::length(original[2]));

    glm::vec3 directionToCamera = glm::normalize(cameraPosition - position);

    glm::quat rotationToCamera = glm::quatLookAt(directionToCamera, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat finalRotation = addRotation ? rotation * rotationToCamera : rotationToCamera;

    scale = customScale == glm::vec3(-1.0f) ? scale : customScale;

    glm::mat4 model =
        glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(finalRotation) * glm::scale(glm::mat4(1.0f), scale);
    return model;
}

// return the rotation of the object to look at the camera
glm::mat4 bee::helper::LookToPosition(const glm::vec3& original, const glm::vec3& lookAtPos)
{
    // glm::vec3 directionToCamera = glm::normalize(lookAtPos - original);
    glm::mat4 rotationToCamera = glm::lookAt(original, lookAtPos, glm::vec3(0.0f, 1.0f, 0.0f));
    return rotationToCamera;
}



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/