#include "public/orientation_gizmo.h"

#include <algorithm>
#include "glm/glm/gtx/transform.hpp"
#include "vk/render_modules/orientation_gizmo_module.h"

namespace rra
{
    namespace renderer
    {
        struct PositionTypePair
        {
            glm::vec3               position;
            OrientationGizmoHitType type;
        };

        OrientationGizmoTransformInfo GetOrientationGizmoTransformInfo(glm::mat4 rotation, float window_ratio)
        {
            OrientationGizmoTransformInfo transform_info{};

            const float line_length = 0.05f;
            const float line_radius = 0.001f;
            const float margin      = 0.01f;

            transform_info.circle_radius     = 0.0125f;
            transform_info.background_radius = line_length + transform_info.circle_radius;

            // We flip across z-axis since world space and NDC use the same coordinate system except with opposite z-axes.
            // (Normally the y-axis would be flipped too but someone entered a negative height for the viewport)
            // This only makes a difference when depth is enabled.
            glm::mat4 screen_transform = glm::translate(glm::vec3(1.0f - (line_length + transform_info.circle_radius + margin),
                                                                  1.0f - (line_length + transform_info.circle_radius + margin) * window_ratio,
                                                                  0.0f)) *
                                         glm::scale(glm::vec3(1.0, window_ratio, -1.0)) * glm::translate(glm::vec3(0.0f, 0.0f, -0.5f));

            transform_info.screen_transform = screen_transform;

            /// Lines.
            rotation                 = glm::transpose(rotation);
            glm::mat4 base_transform = glm::scale(glm::vec3(line_radius, line_length, line_radius));

            transform_info.x_axis_transform = rotation * glm::rotate(glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * base_transform;
            transform_info.y_axis_transform = rotation * base_transform;
            transform_info.z_axis_transform = rotation * glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * base_transform;

            /// Circles.
            std::vector<glm::vec3> endpoints = {
                glm::mat3(rotation) * glm::vec3(line_length, 0.0f, 0.0f),
                glm::mat3(rotation) * glm::vec3(0.0f, line_length, 0.0f),
                glm::mat3(rotation) * glm::vec3(0.0f, 0.0f, line_length),
                glm::mat3(rotation) * glm::vec3(-line_length, 0.0f, 0.0f),
                glm::mat3(rotation) * glm::vec3(0.0f, -line_length, 0.0f),
                glm::mat3(rotation) * glm::vec3(0.0f, 0.0f, -line_length),
            };

            transform_info.x_circle_transform       = glm::translate(endpoints[0]) * glm::scale(glm::vec3(transform_info.circle_radius));
            transform_info.y_circle_transform       = glm::translate(endpoints[1]) * glm::scale(glm::vec3(transform_info.circle_radius));
            transform_info.z_circle_transform       = glm::translate(endpoints[2]) * glm::scale(glm::vec3(transform_info.circle_radius));
            transform_info.minus_x_circle_transform = glm::translate(endpoints[3]) * glm::scale(glm::vec3(transform_info.circle_radius));
            transform_info.minus_y_circle_transform = glm::translate(endpoints[4]) * glm::scale(glm::vec3(transform_info.circle_radius));
            transform_info.minus_z_circle_transform = glm::translate(endpoints[5]) * glm::scale(glm::vec3(transform_info.circle_radius));

            // We translate the background transform to be even further in the back, so when we test mouse clicks it will be behind the other circles.
            transform_info.background_transform = glm::translate(glm::vec3(0.0f, 0.0f, -0.49f)) * glm::scale(glm::vec3(transform_info.background_radius));

            transform_info.order = {0, 1, 2, 3, 4, 5};
            std::sort(
                transform_info.order.begin(), transform_info.order.end(), [=](const int a, const int b) -> bool { return endpoints[a].z < endpoints[b].z; });

            return transform_info;
        }

        glm::vec3 PositionFromMat4(const glm::mat4& mat)
        {
            return glm::vec3((mat)[3]);
        }

        OrientationGizmoHitType CheckOrientationGizmoHit(glm::mat4 rotation, float window_ratio, glm::vec2 hit_coords)
        {
            auto      info       = GetOrientationGizmoTransformInfo(rotation, window_ratio);
            glm::mat4 inv_screen = glm::inverse(info.screen_transform);

            // Since Vulkan viewport is set to invert y, so 1.0 is top of screen.
            hit_coords.y = 1.0f - hit_coords.y;

            // Convert from screen space (in the range [0, 1]) to NDC (in the range [-1, 1]).
            hit_coords = 2.0f * hit_coords - 1.0f;

            // Each of the circle transforms are untransformed by the screen_transform. So we apply
            // the inverse screen_transform so when mouse is in top right, it maps it to center of screen
            // where all the unstretched circle transforms are, which makes it easy to determine if we're
            // in circle since we can test distance.
            hit_coords = glm::vec2(inv_screen * glm::vec4(hit_coords, 0.0f, 1.0f));

            std::vector<PositionTypePair> circle_pairs = {
                {PositionFromMat4(info.x_circle_transform), OrientationGizmoHitType::kX},
                {PositionFromMat4(info.y_circle_transform), OrientationGizmoHitType::kY},
                {PositionFromMat4(info.z_circle_transform), OrientationGizmoHitType::kZ},
                {PositionFromMat4(info.minus_x_circle_transform), OrientationGizmoHitType::kMinusX},
                {PositionFromMat4(info.minus_y_circle_transform), OrientationGizmoHitType::kMinusY},
                {PositionFromMat4(info.minus_z_circle_transform), OrientationGizmoHitType::kMinusZ},
                {PositionFromMat4(info.background_transform), OrientationGizmoHitType::kBackground},
            };

            std::vector<PositionTypePair> hits;

            for (const auto& pair : circle_pairs)
            {
                float radius = (pair.type == OrientationGizmoHitType::kBackground) ? info.background_radius : info.circle_radius;

                if (glm::distance(hit_coords, glm::vec2(pair.position)) < radius)
                {
                    hits.push_back(pair);
                }
            }

            if (hits.empty())
            {
                return OrientationGizmoHitType::kNone;
            }

            // Worldspace has -z furthest from camera, and this is before transforming to screen space.
            auto closest_hit = std::max_element(
                std::begin(hits), std::end(hits), [=](const PositionTypePair& a, const PositionTypePair& b) { return a.position.z < b.position.z; });
            return closest_hit->type;
        }

        glm::vec3 GetForwardFromGizmoHit(OrientationGizmoHitType gizmo_hit)
        {
            switch (gizmo_hit)
            {
            case OrientationGizmoHitType::kX:
                return glm::vec3(-1.0f, 0.0f, 0.0f);
            case OrientationGizmoHitType::kY:
                return glm::vec3(0.0f, -1.0f, 0.0f);
            case OrientationGizmoHitType::kZ:
                return glm::vec3(0.0f, 0.0f, -1.0f);
            case OrientationGizmoHitType::kMinusX:
                return glm::vec3(1.0f, 0.0f, 0.0f);
            case OrientationGizmoHitType::kMinusY:
                return glm::vec3(0.0f, 1.0f, 0.0f);
            case OrientationGizmoHitType::kMinusZ:
                return glm::vec3(0.0f, 0.0f, 1.0f);
            }

            // Unknown hit type.
            return glm::vec3(0.0f, 0.0f, 0.0f);
        }

        void SetOrientationGizmoSelected(OrientationGizmoHitType gizmo_hit)
        {
            OrientationGizmoRenderModule::selected = gizmo_hit;
        }

    }  // namespace renderer
}  // namespace rra