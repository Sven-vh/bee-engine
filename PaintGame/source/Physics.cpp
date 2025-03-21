#include "Physics.hpp"
#include "Components.hpp"

// 95% of this code has been written by ChatGPT.
// Full conversation can be found: https://chatgpt.com/share/6712479f-b758-8011-98fe-bb8753649053

void Physics::OnAttach() {}

void Physics::OnDetach() {}

void Physics::OnRender()
{
    // Ensure that entityOBBs is up-to-date
    if (entityOBBs.empty())
    {
        // If not computed yet, you can call CheckCollisions or recompute OBBs here
        // CheckCollisions();
    }

    // Loop through all OBBs and draw them
    for (const auto& pair : entityOBBs)
    {
        const OBB& obb = pair.second;

        // Compute the 8 corners of the OBB
        glm::vec3 corners[8];
        int index = 0;
        for (int x = -1; x <= 1; x += 2)
        {
            for (int y = -1; y <= 1; y += 2)
            {
                for (int z = -1; z <= 1; z += 2)
                {
                    glm::vec3 corner = obb.center;
                    for (int i = 0; i < 3; ++i)
                    {
                        float sign = (float)(i == 0 ? x : (i == 1 ? y : z));
                        corner += obb.axes[i] * (sign * obb.halfSizes[i] / glm::length(obb.axes[i]));
                    }
                    corners[index++] = corner;
                }
            }
        }

        // Define the edges of the OBB (pairs of corner indices)
        int edges[12][2] = {
            {0, 1},
            {1, 3},
            {3, 2},
            {2, 0},  // Bottom face
            {4, 5},
            {5, 7},
            {7, 6},
            {6, 4},  // Top face
            {0, 4},
            {1, 5},
            {2, 6},
            {3, 7}  // Side edges
        };

        // Draw the edges
        glm::vec4 color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);  // Green color for OBBs
        for (int i = 0; i < 12; ++i)
        {
            glm::vec3 start = corners[edges[i][0]];
            glm::vec3 end = corners[edges[i][1]];
            bee::DebugRenderer::DrawLine(start, end, color);
        }
    }
}

void Physics::OnEngineInit() {
    bee::ComponentManager::RegisterComponentNoDraw<Rigidbody>("Rigidbody", true, true);
    bee::ComponentManager::RegisterComponent<BoxCollider, DrawBoxCollider>("BoxCollider", true, true);
}

void Physics::OnImGuiRender()
{
    ImGui::Begin("Physics");
    ImGui::SliderFloat("Gravity", &m_gravity, -20.0f, 20.0f);
    ImGui::SliderFloat("Reflection Coefficient", &m_reflectionCoefficient, 0.0f, 1.0f);
    ImGui::End();
}

void Physics::OnUpdate(float) {}

void Physics::OnFixedUpdate(float deltaTime)
{
    m_collisions.clear();
    UpdatePositions(deltaTime);
    CheckCollisions();
}

bool Physics::HasCollided(entt::entity entityA, entt::entity entityB, CollisionInfo& info) const
{
    auto it = m_collisions.find(std::make_pair(entityA, entityB));
    if (it != m_collisions.end())
    {
        info = it->second;
        return true;
    }
    return false;
}

void Physics::UpdatePositions(float deltaTime)
{
    auto& registry = bee::Engine.Registry();

    auto view = bee::ecs::GetView<Rigidbody, bee::Transform>(registry);
    for (auto entity : view)
    {
        auto& rb = view.get<Rigidbody>(entity);
        auto& transform = view.get<bee::Transform>(entity);

        rb.previousPosition = transform.GetPosition();
        rb.acceleration.y = m_gravity;
        rb.velocity += rb.acceleration * deltaTime;
        transform.SetPosition(transform.GetPosition() + rb.velocity * deltaTime);
    }
}

// Function written by ChatGPT
void Physics::CheckCollisions()
{
    auto meshView = bee::ecs::GetView<BoxCollider, bee::Renderable>(bee::Engine.Registry());

    // Update box collider sizes based on mesh sizes
    for (auto entity : meshView)
    {
        auto& boxCollider = meshView.get<BoxCollider>(entity);
        auto& renderable = meshView.get<bee::Renderable>(entity);

        // Ensure boxCollider.size is in local space (without scaling)
        // If meshSize is already scaled, divide by the scaling factors
        glm::vec3 meshSize = renderable.mesh->GetHandle().meshSize;

        boxCollider.size = meshSize;
    }

    // Get all entities with box colliders and transforms
    auto view = bee::ecs::GetView<BoxCollider, bee::Transform>(bee::Engine.Registry());
    // Clear and populate the entityOBBs map
    entityOBBs.clear();

    // Extract OBB data for all entities
    for (auto entity : view)
    {
        //if (bee::Engine.Registry().all_of<Rigidbody>(entity)) continue;
        auto& boxCollider = view.get<BoxCollider>(entity);
        auto& transform = view.get<bee::Transform>(entity);

        glm::mat4 model = bee::GetWorldModel(entity, bee::Engine.Registry());

        OBB obb;
        obb.center = glm::vec3(model[3]);  // Extract translation

        // Extract axes and scaling factors
        glm::vec3 right = glm::vec3(model[0]);
        glm::vec3 up = glm::vec3(model[1]);
        glm::vec3 forward = glm::vec3(model[2]);

        glm::vec3 scale = transform.GetScale();

        // Normalize the axes to get rotation
        obb.axes[0] = glm::normalize(right);
        obb.axes[1] = glm::normalize(up);
        obb.axes[2] = glm::normalize(forward);

        // Multiply half-sizes by the scaling factors
        // Since boxCollider.size is now in local space, scaling factors are applied here
        obb.halfSizes = (boxCollider.size * 0.5f) * scale;

        entityOBBs[entity] = obb;
    }

    // Structure to hold collision data
    struct CollisionResult
    {
        bool isColliding;
        float penetrationDepth;
        glm::vec3 collisionNormal;
    };

    // Collision detection function using normalized axes
    auto OBBOverlap = [](const OBB& A, const OBB& B, CollisionResult& result) -> bool
    {
        float EPSILON = 1e-6f;
        float ra, rb;
        glm::mat3 R, AbsR;

        float minPenetration = FLT_MAX;
        glm::vec3 axis;
        bool invertNormal = false;

        // Compute rotation matrix expressing B in A's coordinate frame
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                R[i][j] = glm::dot(A.axes[i], B.axes[j]);
            }
        }

        // Compute translation vector T
        glm::vec3 t = B.center - A.center;
        // Bring translation into A's coordinate frame
        t = glm::vec3(glm::dot(t, A.axes[0]), glm::dot(t, A.axes[1]), glm::dot(t, A.axes[2]));

        // Compute common subexpressions and add in an epsilon term
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                AbsR[i][j] = std::abs(R[i][j]) + EPSILON;
            }
        }

        float penetration;
        glm::vec3 currentAxis;

        // Test axes L = A0, A1, A2
        for (int i = 0; i < 3; ++i)
        {
            ra = A.halfSizes[i];
            rb = B.halfSizes[0] * AbsR[i][0] + B.halfSizes[1] * AbsR[i][1] + B.halfSizes[2] * AbsR[i][2];
            penetration = (ra + rb) - std::abs(t[i]);
            if (penetration < 0)
                return false;  // No collision
            else if (penetration < minPenetration)
            {
                minPenetration = penetration;
                axis = A.axes[i];
                invertNormal = t[i] < 0;
            }
        }

        // Now test the 9 cross products of the axes
        auto TestCrossAxis = [&](int i, int j)
        {
            currentAxis = glm::cross(A.axes[i], B.axes[j]);
            float len2 = glm::length2(currentAxis);
            if (len2 < EPSILON) return true;  // Skip near-zero axes

            currentAxis = glm::normalize(currentAxis);

            ra = A.halfSizes[(i + 1) % 3] * glm::length(A.axes[(i + 2) % 3]) * AbsR[(i + 2) % 3][j] +
                 A.halfSizes[(i + 2) % 3] * glm::length(A.axes[(i + 1) % 3]) * AbsR[(i + 1) % 3][j];
            rb = B.halfSizes[(j + 1) % 3] * glm::length(B.axes[(j + 2) % 3]) * AbsR[i][(j + 2) % 3] +
                 B.halfSizes[(j + 2) % 3] * glm::length(B.axes[(j + 1) % 3]) * AbsR[i][(j + 1) % 3];
            float tValue = t[(i + 2) % 3] * R[(i + 1) % 3][j] - t[(i + 1) % 3] * R[(i + 2) % 3][j];
            penetration = (ra + rb) - std::abs(tValue);
            if (penetration < 0)
                return false;  // No collision
            else if (penetration < minPenetration)
            {
                minPenetration = penetration;
                axis = currentAxis;
                invertNormal = tValue < 0;
            }
            return true;
        };

        // Loop over all cross products of A's and B's axes
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                if (!TestCrossAxis(i, j)) return false;
            }
        }

        // If no separating axis is found, the OBBs must be intersecting
        result.isColliding = true;
        result.penetrationDepth = minPenetration;
        result.collisionNormal = invertNormal ? -axis : axis;

        return true;
    };

    auto HalfSizeAlongNormal = [](const OBB& obb, const glm::vec3& normal) -> float
    {
        return obb.halfSizes.x * std::abs(glm::dot(normal, obb.axes[0])) +
               obb.halfSizes.y * std::abs(glm::dot(normal, obb.axes[1])) +
               obb.halfSizes.z * std::abs(glm::dot(normal, obb.axes[2]));
    };

    // Check collisions between all pairs
    for (auto it = view.begin(); it != view.end(); ++it)
    {
        entt::entity entityA = *it;
        if (bee::Engine.Registry().all_of<Rigidbody>(entityA)) continue;
        OBB& obbA = entityOBBs[entityA];

        for (auto otherIt = std::next(it); otherIt != view.end(); ++otherIt)
        {
            entt::entity entityB = *otherIt;
            if (bee::Engine.Registry().all_of<Rigidbody>(entityB)) continue;
            OBB& obbB = entityOBBs[entityB];

            // Collision result
            CollisionResult collisionResult;
            collisionResult.isColliding = false;

            // Check for collision
            if (OBBOverlap(obbA, obbB, collisionResult))
            {
                // Collision normal from SAT
                // After collision detection and determining collision normal
                glm::vec3 collisionNormal = collisionResult.collisionNormal;

                // Calculate half-size projections along the collision normal
                float distanceA = HalfSizeAlongNormal(obbA, collisionNormal);
                float distanceB = HalfSizeAlongNormal(obbB, collisionNormal);

                // Calculate the contact points on both OBBs
                glm::vec3 contactPointA = obbA.center + collisionNormal * (distanceA - collisionResult.penetrationDepth * 0.5f);
                glm::vec3 contactPointB = obbB.center - collisionNormal * (distanceB - collisionResult.penetrationDepth * 0.5f);

                // Decide which object is smaller
                float sizeA = obbA.halfSizes.x * obbA.halfSizes.y * obbA.halfSizes.z;
                float sizeB = obbB.halfSizes.x * obbB.halfSizes.y * obbB.halfSizes.z;

                glm::vec3 hitPosition;
                auto adjustedNormal = collisionNormal;

                if (sizeA < sizeB)
                {
                    // Object A is smaller
                    hitPosition = contactPointA;

                    // Adjust the collision normal to be the surface normal of object A at the contact point
                    // Find the axis of obbA that is most aligned with the collision normal
                    float maxDot = -FLT_MAX;
                    for (int i = 0; i < 3; ++i)
                    {
                        float dotProduct = glm::dot(collisionNormal, obbA.axes[i]);
                        if (std::abs(dotProduct) > maxDot)
                        {
                            maxDot = std::abs(dotProduct);
                            adjustedNormal = dotProduct > 0 ? obbA.axes[i] : -obbA.axes[i];
                        }
                    }
                }
                else
                {
                    // Object B is smaller
                    hitPosition = contactPointB;

                    // Adjust the collision normal to be the surface normal of object B at the contact point
                    float maxDot = -FLT_MAX;
                    for (int i = 0; i < 3; ++i)
                    {
                        float dotProduct = glm::dot(collisionNormal, obbB.axes[i]);
                        if (std::abs(dotProduct) > maxDot)
                        {
                            maxDot = std::abs(dotProduct);
                            adjustedNormal = dotProduct > 0 ? obbB.axes[i] : -obbB.axes[i];
                        }
                    }
                }

                // convert adjustedNormal from localspace to worldspace
                adjustedNormal = glm::normalize(obbA.axes[0] * adjustedNormal.x + obbA.axes[1] * adjustedNormal.y +
                                                obbA.axes[2] * adjustedNormal.z);

                // Store collision info with the adjusted normal
                CollisionInfo info;
                info.worldPosition = hitPosition;
                info.normal = adjustedNormal;  // Use the adjusted normal
                info.impactVelocity = 0.0f;    // Initialize impact velocity to 0
                m_collisions[std::make_pair(entityA, entityB)] = info;
            }
        }
    }

    // For each entity with a rigidbody
    auto rigidbodyView = bee::ecs::GetView<Rigidbody, bee::Transform>(bee::Engine.Registry());

    for (auto entityA : rigidbodyView)
    {
        Rigidbody& rigidbody = rigidbodyView.get<Rigidbody>(entityA);
        bee::Transform& transform = rigidbodyView.get<bee::Transform>(entityA);
        OBB& obbA = entityOBBs[entityA];

        // Initialize remaining movement
        glm::vec3 start = rigidbody.previousPosition;
        glm::vec3 end = transform.GetPosition();
        glm::vec3 remainingMovement = end - start;

        // Maximum number of iterations to prevent infinite loops
        const int maxIterations = 5;
        int iterations = 0;

        while (glm::length2(remainingMovement) > 1e-6f && iterations < maxIterations)
        {
            iterations++;

            // Update OBB position for current movement
            obbA.center = start;

            float nearestT = 1.0f;
            auto collisionNormal = glm::vec3(0.0f);
            entt::entity collidedEntity = entt::null;

            // Check collisions with other entities
            for (auto& entityB : entityOBBs)
            {
                if (entityB.first == entityA) continue;  // Skip self

                OBB& obbB = entityB.second;

                if (bee::Engine.Registry().all_of<Rigidbody>(entityB.first)) continue;

                float t;
                glm::vec3 intersectionPoint;
                glm::vec3 normal;

                if (LineSegmentOBBIntersection(start, start + remainingMovement, obbB, t, intersectionPoint, normal))
                {
                    if (t < nearestT)
                    {
                        nearestT = t;
                        collisionNormal = normal;
                        collidedEntity = entityB.first;
                    }
                }
            }

            if (collidedEntity != entt::null)
            {
                // Collision detected at time t = nearestT
                // Update position to collision point
                glm::vec3 movementToCollision = remainingMovement * nearestT;
                start += movementToCollision;
                obbA.center = start;

                // Store collision info
                CollisionInfo info;
                info.worldPosition = start;
                info.normal = collisionNormal;
                // set the impact velocity to the velocity of the rigidbody in the inverse direction of the collision normal
                info.impactVelocity = glm::dot(rigidbody.velocity, -collisionNormal);
                m_collisions[std::make_pair(entityA, collidedEntity)] = info;

                // Adjust velocity (simple reflection)
                glm::vec3 velocity = rigidbody.velocity;
                rigidbody.velocity =
                    velocity - 2.0f * glm::dot(velocity, collisionNormal) * collisionNormal * m_reflectionCoefficient;
                // also set the other entity's velocity
                if (bee::Engine.Registry().all_of<Rigidbody>(collidedEntity))
                {
                    auto& otherRigidbody = bee::Engine.Registry().get<Rigidbody>(collidedEntity);
                    glm::vec3 otherVelocity = otherRigidbody.velocity;
                    otherRigidbody.velocity = otherVelocity + 2.0f * glm::dot(otherVelocity, -collisionNormal) *
                                                                  -collisionNormal * m_reflectionCoefficient;
                }

                // place them back to the point of collision plus the size of the bounding box
                glm::vec3 boundingVolume = obbA.halfSizes;
                start += boundingVolume * collisionNormal;

                // Update remaining movement
                remainingMovement = remainingMovement * (1.0f - nearestT);

                // Adjust remaining movement based on new velocity
                remainingMovement = rigidbody.velocity * (glm::length(remainingMovement) / glm::length(velocity));
            }
            else
            {
                // No collision, move to end position
                start += remainingMovement;
                remainingMovement = glm::vec3(0.0f);
            }
        }

        // Update transform component
        bee::Engine.Registry().get<bee::Transform>(entityA).SetPosition(start);
    }
}

bool Physics::LineSegmentOBBIntersection(const glm::vec3& start,
                                         const glm::vec3& end,
                                         const OBB& obb,
                                         float& t,
                                         glm::vec3& intersectionPoint,
                                         glm::vec3& normal)
{
    // Transform the line segment into the OBB's local space
    glm::vec3 dir = end - start;
    glm::vec3 localStart = start - obb.center;

    // Initialize variables for tracking the interval of intersection
    float tmin = 0.0f;
    float tmax = 1.0f;

    // For each OBB axis
    for (int i = 0; i < 3; ++i)
    {
        glm::vec3 axis = obb.axes[i];
        float e = glm::dot(axis, localStart);
        float f = glm::dot(axis, dir);
        float h = obb.halfSizes[i];

        if (std::abs(f) > 1e-6f)
        {
            float t1 = (-h - e) / f;
            float t2 = (h - e) / f;

            if (t1 > t2) std::swap(t1, t2);

            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;

            if (tmin > tmax) return false;
            if (tmax < 0.0f) return false;
        }
        else
        {
            if (-e - h > 0.0f || -e + h < 0.0f) return false;
        }
    }

    t = tmin;

    if (t < 0.0f) t = 0.0f;  // Clamp to segment start
    if (t > 1.0f) t = 1.0f;  // Clamp to segment end

    // Calculate intersection point
    intersectionPoint = start + dir * t;

    // Calculate normal at the point of intersection
    // Find the axis with the largest projection
    glm::vec3 localPoint = intersectionPoint - obb.center;
    for (int i = 0; i < 3; ++i)
    {
        float projection = glm::dot(localPoint, obb.axes[i]);
        float extent = obb.halfSizes[i];
        if (std::abs(projection) >= extent - 1e-3f)
        {
            normal = obb.axes[i] * (projection > 0 ? 1.0f : -1.0f);
            break;
        }
    }

    return true;
}
