//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the ray inspector model.
//=============================================================================

#include "models/ray/ray_inspector_model.h"

#include <QTableView>
#include <QScrollBar>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

#include "models/ray/ray_list_item_model.h"
#include "util/string_util.h"
#include "views/widget_util.h"
#include "settings/settings.h"
#include "public/rra_tlas.h"
#include "managers/message_manager.h"

// We can't use std::max or glm::max since the windows macro ends up overriding the max keyword.
// So we underfine max for this file only.
#undef max
// Same for min
#undef min

namespace rra
{
    RayInspectorModel::RayInspectorModel(int32_t num_model_widgets)
        : ModelViewMapper(num_model_widgets)
        , tree_model_(nullptr)
        , proxy_model_(nullptr)
    {
        scene_collection_model_ = new RayInspectorSceneCollectionModel();
    }

    RayInspectorModel::~RayInspectorModel()
    {
        delete tree_model_;
        delete proxy_model_;
        delete scene_collection_model_;
    }

    void RayInspectorModel::ResetModelValues()
    {
        tree_model_->removeRows(0, tree_model_->rowCount());
        scene_collection_model_->ResetModelValues();
    }

    void RayInspectorModel::InitializeTreeModel(ScaledTreeView* tree_view)
    {
        if (proxy_model_ != nullptr)
        {
            delete proxy_model_;
            proxy_model_ = nullptr;
        }

        proxy_model_ = new RayInspectorRayTreeProxyModel();
        tree_model_  = proxy_model_->InitializeRayTreeModels(tree_view);
        tree_model_->Initialize(tree_view);
    }

    void RayInspectorModel::SetKey(RayInspectorKey key)
    {
        key_ = key;

        ClearKey();

        uint32_t ray_count{};
        RraRayGetRayCount(key.dispatch_id, key.invocation_id, &ray_count);

        rays_.resize(ray_count);
        RraRayGetRays(key.dispatch_id, key.invocation_id, rays_.data());

        std::unordered_map<uint32_t, std::shared_ptr<RayInspectorRayTreeItemData>> dynamic_id_to_ray_data;

        for (uint32_t i = 0; i < ray_count; ++i)
        {
            IntersectionResult intersection_result{};
            RraRayGetIntersectionResult(key.dispatch_id, key.invocation_id, i, &intersection_result);

            uint32_t        any_hit_count{};
            AnyHitRayResult any_hit_result{};
            RraRayGetAnyHitInvocationData(key.dispatch_id, key.invocation_id, i, &any_hit_count, &any_hit_result);

            auto item_data                        = std::make_shared<RayInspectorRayTreeItemData>();
            item_data->row_index                  = i;
            item_data->ray_event_count            = intersection_result.num_iterations;
            item_data->ray_instance_intersections = intersection_result.num_instance_intersections;
            item_data->ray_any_hit_invocations    = any_hit_count;
            item_data->hit                        = intersection_result.hit_t >= 0.0;
            intersection_result.any_hit_result    = any_hit_result;

            item_data->dynamic_id = rays_[i].dynamic_id;
            item_data->parent_id  = rays_[i].parent_id;

            results_.push_back(intersection_result);

            dynamic_id_to_ray_data[item_data->dynamic_id] = item_data;
        }

        using data_handle = std::shared_ptr<RayInspectorRayTreeItemData>;

        // Create ray hierarchies.
        for (auto& [dynamic_id, item_data] : dynamic_id_to_ray_data)
        {
            for (auto& [other_dynamic_id, other_item_data] : dynamic_id_to_ray_data)
            {
                // Am I the parent of the other item ?
                if (dynamic_id == other_item_data->parent_id)
                {
                    item_data->child_rays.push_back(other_item_data);
                }
            }

            // Sort child rays.
            std::sort(item_data->child_rays.begin(), item_data->child_rays.end(), [](const data_handle& a, const data_handle& b) {
                return a->row_index < b->row_index;
            });
        }

        std::vector<std::shared_ptr<RayInspectorRayTreeItemData>> root_rays;
        for (auto& [dynamic_id, item_data] : dynamic_id_to_ray_data)
        {
            if (item_data->parent_id == 0xFFFFFFFF || item_data->parent_id == 0)
            {
                root_rays.push_back(item_data);
            }
        }

        // Sort root rays
        std::sort(root_rays.begin(), root_rays.end(), [](const data_handle& a, const data_handle& b) { return a->row_index < b->row_index; });

        tree_model_->ClearRays();
        tree_model_->AddNewRays(root_rays);

        SelectRayIndex(0);
    }

    void RayInspectorModel::ClearKey()
    {
        ResetModelValues();
        rays_.clear();
        results_.clear();
        SelectRayIndex(0);
    }

    RayInspectorKey RayInspectorModel::GetKey()
    {
        return key_;
    }

    void RayInspectorModel::SelectRayIndex(uint32_t ray_index)
    {
        ray_index_ = ray_index;
        PopulateFlagsTable(flags_table_model_);
    }

    uint32_t RayInspectorModel::GetSelectedRayIndex()
    {
        return ray_index_;
    }

    std::optional<uint64_t> RayInspectorModel::GetSelectedTlasIndex()
    {
        std::optional<uint64_t> tlas_index = std::nullopt;

        auto ray_index = GetSelectedRayIndex();
        auto opt_ray   = GetRay(ray_index);
        if (!opt_ray)
        {
            return tlas_index;
        }

        const auto& ray = opt_ray.value();

        uint64_t difference = 0xFFFFFFFF;
        for (auto& [address, index] : tlas_address_to_index_)
        {
            if (address > ray.tlas_address)
            {
                continue;
            }

            uint64_t local_difference = ray.tlas_address - address;
            if (local_difference < difference)
            {
                difference = local_difference;
                tlas_index = index;
            }
        }

        return tlas_index;
    }

    RayInspectorRayTreeProxyModel* RayInspectorModel::GetProxyModel() const
    {
        return proxy_model_;
    }

    void RayInspectorModel::ToggleInstanceTransformWireframe()
    {
        if (render_state_adapter_ != nullptr)
        {
            if (render_state_adapter_->GetRenderInstancePretransform())
            {
                render_state_adapter_->SetRenderInstancePretransform(false);
            }
            else
            {
                render_state_adapter_->SetRenderInstancePretransform(true);
            }
            emit MessageManager::Get().RenderStateChanged();
        }
    }

    void RayInspectorModel::ToggleBVHWireframe()
    {
        if (render_state_adapter_ != nullptr)
        {
            if (render_state_adapter_->GetRenderBoundingVolumes())
            {
                render_state_adapter_->SetRenderBoundingVolumes(false);
            }
            else
            {
                render_state_adapter_->SetRenderBoundingVolumes(true);
            }
            emit MessageManager::Get().RenderStateChanged();
        }
    }

    void RayInspectorModel::ToggleMeshWireframe()
    {
        if (render_state_adapter_ != nullptr)
        {
            if (render_state_adapter_->GetRenderWireframe())
            {
                render_state_adapter_->SetRenderWireframe(false);
            }
            else
            {
                render_state_adapter_->SetRenderWireframe(true);
            }
            emit MessageManager::Get().RenderStateChanged();
        }
    }

    void RayInspectorModel::ToggleRenderGeometry()
    {
        if (render_state_adapter_ != nullptr && !render_state_adapter_->GetRenderTraversal())
        {
            if (render_state_adapter_->GetRenderGeometry())
            {
                render_state_adapter_->SetRenderGeometry(false);
            }
            else
            {
                render_state_adapter_->SetRenderGeometry(true);
            }
            emit MessageManager::Get().RenderStateChanged();
        }
    }

    void RayInspectorModel::AdaptTraversalCounterRangeToView()
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->AdaptTraversalCounterRangeToView(
                [=](uint32_t min, uint32_t max) { emit rra::MessageManager::Get().TraversalSliderChanged(min, max); });
        }
    }

    std::optional<Ray> RayInspectorModel::GetRay(uint32_t index) const
    {
        if (index >= rays_.size())
        {
            return std::nullopt;
        }
        return rays_[index];
    }

    float RayInspectorGetNearDistance(BoundingVolumeExtents scene_volume)
    {
        float x               = scene_volume.max_x - scene_volume.min_x;
        float y               = scene_volume.max_y - scene_volume.min_y;
        float z               = scene_volume.max_z - scene_volume.min_z;
        float diagonal_length = std::sqrt(x * x + y * y + z * z);

        return 0.01f * diagonal_length;
    }

    void RayInspectorModel::UpdateTlasMap()
    {
        tlas_address_to_index_.clear();

        uint64_t tlas_count = 0;
        RraBvhGetTlasCount(&tlas_count);

        for (uint64_t i = 0; i < tlas_count; i++)
        {
            uint64_t address;
            RraTlasGetBaseAddress(i, &address);
            tlas_address_to_index_[address] = i;
        }
    }

    ViewerIOCallbacks RayInspectorModel::GetViewerCallbacks()
    {
        ViewerIOCallbacks viewer_callbacks = {};

        auto opt_tlas_index = GetSelectedTlasIndex();
        if (!opt_tlas_index)
        {
            return viewer_callbacks;
        }

        // Use the same selection extents for the scene.
        viewer_callbacks.get_scene_extents = [=]() -> BoundingVolumeExtents {
            BoundingVolumeExtents extents = {};
            extents.max_x                 = -std::numeric_limits<float>::infinity();
            extents.max_y                 = -std::numeric_limits<float>::infinity();
            extents.max_z                 = -std::numeric_limits<float>::infinity();
            extents.min_x                 = std::numeric_limits<float>::infinity();
            extents.min_y                 = std::numeric_limits<float>::infinity();
            extents.min_z                 = std::numeric_limits<float>::infinity();

            auto ray_count = GetRayCount();

            auto opt_selected_ray = GetRay(GetSelectedRayIndex());
            if (!opt_selected_ray)
            {
                return extents;
            }
            auto& selected_ray = opt_selected_ray.value();

            for (uint32_t i = 0; i < ray_count; i++)
            {
                auto opt_ray = GetRay(i);
                if (!opt_ray)
                {
                    continue;
                }

                auto& ray = opt_ray.value();
                if (ray.tlas_address != selected_ray.tlas_address)
                {
                    continue;
                }

                extents.max_x = glm::max(extents.max_x, ray.origin[0]);
                extents.max_y = glm::max(extents.max_y, ray.origin[1]);
                extents.max_z = glm::max(extents.max_z, ray.origin[2]);
                extents.min_x = glm::min(extents.min_x, ray.origin[0]);
                extents.min_y = glm::min(extents.min_y, ray.origin[1]);
                extents.min_z = glm::min(extents.min_z, ray.origin[2]);
            }

            extents.max_x += 1.0f;
            extents.max_y += 1.0f;
            extents.max_z += 1.0f;
            extents.min_x -= 1.0f;
            extents.min_y -= 1.0f;
            extents.min_z -= 1.0f;

            return extents;
        };

        viewer_callbacks.get_context_options = [=](rra::SceneContextMenuRequest request) -> rra::SceneContextMenuOptions {
            RRA_UNUSED(request);
            rra::SceneContextMenuOptions options{};
            options[kFocusOnSelectionName] = [&]() { BurstResetCamera(); };
            return options;
        };

        viewer_callbacks.select_from_scene = [=](const rra::renderer::Camera* camera, glm::vec2 coords) -> rra::SceneCollectionModelClosestHit {
            RRA_UNUSED(camera);
            RRA_UNUSED(coords);

            return {};
        };

        return viewer_callbacks;
    }

    void RayInspectorModel::InitializeFlagsTableModel(ScaledTableView* table_view)
    {
        flags_table_model_ = new rra::FlagsTableItemModel();
        flags_table_model_->SetRowCount(8);
        flags_table_model_->SetColumnCount(2);

        table_view->setModel(flags_table_model_);

        flags_table_model_->SetRowFlagName(0, "Opaque");
        flags_table_model_->SetRowFlagName(1, "No opaque");
        flags_table_model_->SetRowFlagName(2, "Terminate on first hit");
        flags_table_model_->SetRowFlagName(3, "Skip closest hit shader");
        flags_table_model_->SetRowFlagName(4, "Cull back facing triangles");
        flags_table_model_->SetRowFlagName(5, "Cull front facing triangles");
        flags_table_model_->SetRowFlagName(6, "Cull opaque");
        flags_table_model_->SetRowFlagName(7, "Cull no opaque");

        flags_table_model_->Initialize(table_view);

        table_view->GetHeaderView()->setVisible(false);
    }

    void RayInspectorModel::PopulateFlagsTable(FlagsTableItemModel* flags_table)
    {
        auto opt_ray = GetRay(GetSelectedRayIndex());
        if (!opt_ray)
        {
            return;
        }

        auto flags = opt_ray->ray_flags;

        const uint gl_RayFlagsOpaqueEXT                   = 1U;
        const uint gl_RayFlagsNoOpaqueEXT                 = 2U;
        const uint gl_RayFlagsTerminateOnFirstHitEXT      = 4U;
        const uint gl_RayFlagsSkipClosestHitShaderEXT     = 8U;
        const uint gl_RayFlagsCullBackFacingTrianglesEXT  = 16U;
        const uint gl_RayFlagsCullFrontFacingTrianglesEXT = 32U;
        const uint gl_RayFlagsCullOpaqueEXT               = 64U;
        const uint gl_RayFlagsCullNoOpaqueEXT             = 128U;

        flags_table->SetRowChecked(0, flags & gl_RayFlagsOpaqueEXT);
        flags_table->SetRowChecked(1, flags & gl_RayFlagsNoOpaqueEXT);
        flags_table->SetRowChecked(2, flags & gl_RayFlagsTerminateOnFirstHitEXT);
        flags_table->SetRowChecked(3, flags & gl_RayFlagsSkipClosestHitShaderEXT);
        flags_table->SetRowChecked(4, flags & gl_RayFlagsCullBackFacingTrianglesEXT);
        flags_table->SetRowChecked(5, flags & gl_RayFlagsCullFrontFacingTrianglesEXT);
        flags_table->SetRowChecked(6, flags & gl_RayFlagsCullOpaqueEXT);
        flags_table->SetRowChecked(7, flags & gl_RayFlagsCullNoOpaqueEXT);

        // The table will not be updated without this.
        flags_table->dataChanged(QModelIndex(), QModelIndex());
    }

    void RayInspectorModel::BurstResetCamera()
    {
        camera_reset_countdown_ = 1;  // Used as a trigger to update the frame and reset camera, can be increased to update more times for debugging.
    }

    void RayInspectorModel::BurstUpdateCamera()
    {
        camera_update_countdown_ = 1;  // Used as a trigger to update the frame, can be increased to update more times for debugging.
    }

    std::function<ViewerFitParams(rra::renderer::Camera*)> RayInspectorModel::GetCameraFitFunction()
    {
        return [=](rra::renderer::Camera* camera) -> ViewerFitParams {
            ViewerFitParams params;

            auto camera_controller = static_cast<rra::ViewerIO*>(camera->GetCameraController());
            if (camera_controller)
            {
                camera_controller->ResetArcRadius();
            }

            params.position = camera->GetPosition();
            auto opt_ray    = GetRay(GetSelectedRayIndex());

            if (opt_ray)
            {
                params.position = glm::vec3(opt_ray->origin[0], opt_ray->origin[1], opt_ray->origin[2]);
                params.forward  = glm::vec3(opt_ray->direction[0], opt_ray->direction[1], opt_ray->direction[2]);

                rra::ViewerIOOrientation orientation = static_cast<rra::ViewerIO*>(camera->GetCameraController())->GetCameraOrientation();
                switch (orientation.up_axis)
                {
                case ViewerIOUpAxis::kUpAxisX:
                    params.up = glm::vec3{1.0f, 0.0f, 0.0f};
                    break;
                case ViewerIOUpAxis::kUpAxisY:
                    params.up = glm::vec3{0.0f, 1.0f, 0.0f};
                    break;
                case ViewerIOUpAxis::kUpAxisZ:
                    params.up = glm::vec3{0.0f, 0.0f, 1.0f};
                    break;
                }

                auto      desired_rotation  = glm::lookAt({}, params.forward, params.up);
                glm::mat4 bound_perspective = glm::perspective(glm::radians(camera->GetFieldOfView()), camera->GetAspectRatio(), 0.1f, 1.0f);

                glm::vec3 screen_space_coords = glm::vec3(-0.5f, -0.5f, 1.0f);

                glm::vec4 perspective_direction = glm::inverse(bound_perspective) * glm::vec4(screen_space_coords, 1.0f);
                perspective_direction           = perspective_direction / perspective_direction.w;
                glm::vec3 push_direction        = glm::transpose(glm::mat3(desired_rotation)) * glm::vec3(perspective_direction);

                float near_plane_distance = glm::length(screen_space_coords);
                params.position -= glm::normalize(push_direction) * camera->GetNearClip() * near_plane_distance * 2.0f;
                camera->SetArcCenterPosition(params.position);
            }
            camera->SetMovementSpeed(1.0f);

            return params;
        };
    }

    void RayInspectorModel::SetRendererAdapters(const rra::renderer::RendererAdapterMap& adapters)
    {
        using namespace rra::renderer;

        render_state_adapter_ = GetAdapter<RenderStateAdapter*>(adapters, RendererAdapterType::kRendererAdapterTypeRenderState);
    }

    std::vector<renderer::RayInspectorRay> RayInspectorModel::GetRenderableRays(uint32_t* out_first_ray_outline, uint32_t* out_outline_count)
    {
        std::vector<renderer::RayInspectorRay> renderable_rays;
        std::vector<renderer::RayInspectorRay> mid_renderable_rays;      // Outline of selected rays.
        std::vector<renderer::RayInspectorRay> delayed_renderable_rays;  // Selected ray itself.

        renderable_rays.reserve(rays_.size());

        // Reverse order that i is iterated so rays shot first appear in front.
        for (int32_t i = (int32_t)rays_.size() - 1; i >= 0; i--)
        {
            const auto&        ray                 = rays_[i];
            IntersectionResult intersection_result = {};
            RraRayGetIntersectionResult(key_.dispatch_id, key_.invocation_id, static_cast<uint32_t>(i), &intersection_result);

            renderer::RayInspectorRay iray = {};
            iray.tlas_address              = ray.tlas_address;
            iray.direction                 = glm::vec4(ray.direction[0], ray.direction[1], ray.direction[2], 1.0);
            iray.origin                    = glm::vec4(ray.origin[0], ray.origin[1], ray.origin[2], 1.0);
            iray.tmin                      = ray.t_min;
            iray.tmax                      = ray.t_max;
            iray.is_outline                = ray_index_ == (uint32_t)i;
            iray.ray_flags                 = ray.ray_flags;
            iray.cull_mask                 = ray.cull_mask;

            if (intersection_result.hit_t >= 0.0)
            {
                // There is a hit, so only render hit
                iray.tmax = intersection_result.hit_t;
            }

            iray.hit_distance = intersection_result.hit_t;

            if (iray.is_outline)
            {
                mid_renderable_rays.push_back(iray);  // Push back outline to be rendered.
                iray.is_outline = false;
                delayed_renderable_rays.push_back(iray);  // Push back ray to be rendered on top of outline.
            }
            else
            {
                renderable_rays.push_back(iray);
            }
        }

        *out_first_ray_outline = (uint32_t)renderable_rays.size();
        *out_outline_count     = (uint32_t)mid_renderable_rays.size();

        renderable_rays.insert(renderable_rays.end(), mid_renderable_rays.begin(), mid_renderable_rays.end());
        renderable_rays.insert(renderable_rays.end(), delayed_renderable_rays.begin(), delayed_renderable_rays.end());

        return renderable_rays;
    }

    std::optional<IntersectionResult> RayInspectorModel::GetRayResult(uint32_t index) const
    {
        if (index >= results_.size())
        {
            return std::nullopt;
        }
        return results_[index];
    }

    uint32_t RayInspectorModel::GetRayCount() const
    {
        return static_cast<uint32_t>(rays_.size());
    }

    float GetNearPlane(Scene* scene, rra::renderer::Camera* camera)
    {
        const float kNearMultiplier = 0.01f;

        BoundingVolumeExtents volume = {};
        scene->GetSceneBoundingVolume(volume);
        float diagonal = glm::length(glm::vec3{volume.max_x - volume.min_x, volume.max_y - volume.min_y, volume.max_z - volume.min_z});

        float near_value = diagonal * kNearMultiplier;

        size_t width  = 16;
        size_t height = 16;

        for (size_t i = 0; i < width; i++)
        {
            for (size_t k = 0; k < height; k++)
            {
                float x = i / float(width - 1);
                float y = k / float(height - 1);

                x = (x - 0.5f) * 2.0f;
                y = (y - 0.5f) * 2.0f;

                auto ray = camera->CastRay({x, y});

                auto closest_hit = scene->CastRayGetClosestHit(ray.origin, ray.direction);
                if (closest_hit.distance > 0.0 && closest_hit.distance < near_value)
                {
                    near_value = closest_hit.distance;
                }
            }
        }

        return glm::max(near_value * kNearMultiplier, 0.05f);
    }

    void RayInspectorModel::PopulateScene(renderer::RendererInterface* renderer)
    {
        if (renderer == nullptr)
        {
            return;
        }

        auto        ray_index = GetSelectedRayIndex();
        const auto& opt_ray   = GetRay(ray_index);
        if (!opt_ray)
        {
            return;
        }

        const auto& ray = opt_ray.value();

        auto opt_tlas_index = GetSelectedTlasIndex();
        if (!opt_tlas_index)
        {
            return;
        }

        auto tlas_index = opt_tlas_index.value();

        scene_collection_model_->PopulateScene(renderer, tlas_index);
        Scene* bvh_scene = scene_collection_model_->GetSceneByIndex(tlas_index);
        bvh_scene->ShowAllNodes();
        bvh_scene->FilterNodesByInstanceMask(ray.cull_mask);

        // Set the scene info callback in the renderer so the renderer can acces up-to-date info about the scene.
        auto&    node_colors             = GetSceneNodeColors();
        bool     fused_instances_enabled = scene_collection_model_->GetFusedInstancesEnabled(tlas_index);
        uint32_t first_ray_outline{};
        uint32_t ray_outline_count{};
        auto     rendering_rays = GetRenderableRays(&first_ray_outline, &ray_outline_count);

        renderer->SetSceneInfoCallback([=](renderer::RendererSceneInfo& info, renderer::Camera* camera, bool frustum_culling, bool force_camera_update) {
            info.scene_iteration                       = bvh_scene->GetSceneIteration();
            info.depth_range_lower_bound               = bvh_scene->GetDepthRangeLowerBound();
            info.depth_range_upper_bound               = bvh_scene->GetDepthRangeUpperBound();
            auto& stats                                = bvh_scene->GetSceneStatistics();
            info.max_instance_count                    = stats.max_instance_count;
            info.max_triangle_count                    = stats.max_triangle_count;
            info.max_tree_depth                        = stats.max_tree_depth;
            info.max_node_depth                        = stats.max_node_depth;
            info.box16_node_color                      = node_colors.box16_node_color;
            info.box32_node_color                      = node_colors.box32_node_color;
            info.instance_node_color                   = node_colors.instance_node_color;
            info.procedural_node_color                 = node_colors.procedural_node_color;
            info.triangle_node_color                   = node_colors.triangle_node_color;
            info.selected_node_color                   = node_colors.selected_node_color;
            info.wireframe_normal_color                = node_colors.wireframe_normal_color;
            info.wireframe_selected_color              = node_colors.wireframe_selected_color;
            info.selected_geometry_color               = node_colors.selected_geometry_color;
            info.background1_color                     = node_colors.background1_color;
            info.background2_color                     = node_colors.background2_color;
            info.transparent_color                     = node_colors.transparent_color;
            info.opaque_color                          = node_colors.opaque_color;
            info.positive_color                        = node_colors.positive_color;
            info.negative_color                        = node_colors.negative_color;
            info.build_algorithm_none_color            = node_colors.build_algorithm_none_color;
            info.build_algorithm_fast_build_color      = node_colors.build_algorithm_fast_build_color;
            info.build_algorithm_fast_trace_color      = node_colors.build_algorithm_fast_trace_color;
            info.build_algorithm_both_color            = node_colors.build_algorithm_both_color;
            info.instance_opaque_none_color            = node_colors.instance_opaque_none_color;
            info.instance_opaque_force_opaque_color    = node_colors.instance_opaque_force_opaque_color;
            info.instance_opaque_force_no_opaque_color = node_colors.instance_opaque_force_no_opaque_color;
            info.instance_opaque_force_both_color      = node_colors.instance_opaque_force_both_color;
            info.selected_ray_color                    = node_colors.selected_ray_color;
            info.ray_color                             = node_colors.ray_color;
            info.shadow_ray_color                      = node_colors.shadow_ray_color;
            info.zero_mask_ray_color                   = node_colors.zero_mask_ray_color;
            info.first_ray_outline                     = first_ray_outline;
            info.ray_outline_count                     = ray_outline_count;

            bool scene_updated  = info.scene_iteration != info.last_iteration;
            bool camera_changed = camera->GetViewProjection() != info.last_view_proj;

            if (scene_updated)
            {
                info.custom_triangles          = bvh_scene->GetCustomTriangles();
                info.bounding_volume_list      = bvh_scene->GetBoundingVolumeList();
                info.selected_volume_instances = bvh_scene->GetSelectedVolumeInstances();
                info.traversal_tree            = bvh_scene->GenerateTraversalTree();
                info.instance_counts           = bvh_scene->GetBlasInstanceCounts();
            }

            if (scene_updated || camera_changed || force_camera_update)
            {
                if (frustum_culling)
                {
                    camera->SetNearClipScale(0.0);
                    auto frustum_info                = camera->GetFrustumInfo();
                    frustum_info.fov_threshold_ratio = rra::Settings::Get().GetFrustumCullRatio();
                    info.instance_map                = bvh_scene->GetFrustumCulledInstanceMap(frustum_info);  ///< Writes to closest_point_to_camera.
                    info.closest_point_to_camera     = frustum_info.closest_point_to_camera;
                }
                else
                {
                    BoundingVolumeExtents volume;
                    bvh_scene->GetSceneBoundingVolume(volume);
                    float closest_point_distance = RayInspectorGetNearDistance(volume);
                    info.instance_map            = bvh_scene->GetInstanceMap();
                    info.closest_point_to_camera = camera->GetPosition() + glm::vec3(closest_point_distance, 0.0f, 0.0f);
                }

                auto near_plane = GetNearPlane(bvh_scene, camera);
                camera->SetNearClipMultiplier(1.0f);
                camera->SetNearClipScale(near_plane);

                info.camera = camera;
            }

            info.last_view_proj = camera->GetViewProjection();
            info.last_iteration = info.scene_iteration;

            info.fused_instances_enabled = fused_instances_enabled;

            info.ray_inspector_rays = rendering_rays;

            // Burst resetting camera is no longer necessary, since reset no longer needs to converge.
            // Leave the code for it for testing in case we see issues later.
            auto camera_controller = static_cast<rra::ViewerIO*>(camera->GetCameraController());
            camera_controller->SetViewerCallbacks(GetViewerCallbacks());
            if (camera_controller && camera_reset_countdown_ > 0)
            {
                camera_reset_countdown_--;

                auto fit_function = GetCameraFitFunction();

                // Since we are casting rays instead of using frustum culling near plane value we can just iterate independently of rendering.
                for (size_t i = 0; i < 3; i++)
                {
                    auto near_plane = GetNearPlane(bvh_scene, camera);
                    camera->SetNearClipMultiplier(1.0f);
                    camera->SetNearClipScale(near_plane);

                    auto fit_values = fit_function(camera);
                    camera_controller->FitCameraParams(fit_values.position, fit_values.forward, fit_values.up);
                }

                renderer->MarkAsDirty();
            }

            // Burst update camera.
            // Used when the camera need to be updated but not reset.
            if (camera_update_countdown_ > 0)
            {
                camera_update_countdown_--;
                renderer->MarkAsDirty();

                // Increment the scene to force an update on the near plane.
                bvh_scene->IncrementSceneIteration();
            }
        });
    }

}  // namespace rra
