//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Renderer Interface.
//=============================================================================

#include "public/renderer_interface.h"
#include "public/render_state_adapter.h"
#include "public/view_state_adapter.h"

#include "vk/renderer_vulkan.h"

#include "vk/render_modules/bounding_volume.h"
#include "vk/render_modules/mesh_render_module.h"
#include "vk/render_modules/checker_clear.h"
#include "vk/render_modules/traversal.h"
#include "vk/render_modules/orientation_gizmo_module.h"
#include "vk/render_modules/ray_inspector_overlay.h"
#include "vk/render_modules/selection_module.h"

namespace rra
{
    namespace renderer
    {
        RendererInterface::RendererInterface()
            : width_(0)
            , height_(0)
        {
            // Initialize the camera with a good default settings.
            camera_.SetPerspective(kDefaultCameraFieldOfView, kDefaultCameraNearPlaneDistance, kDefaultCameraFarPlaneDistance);

            heatmap_               = new Heatmap(Heatmap::GetClassicHeatmapData());
            should_update_heatmap_ = true;
        }

        RendererInterface::~RendererInterface()
        {
            if (heatmap_)
            {
                delete heatmap_;
            }
        }

        void RendererInterface::SetWindowInfo(const WindowInfo* window_info)
        {
            window_info_ = window_info;
        }

        void RendererInterface::SetDimensions(int width, int height)
        {
            width_  = width;
            height_ = height;

            HandleDimensionsUpdated();
        }

        Camera& RendererInterface::GetCamera()
        {
            return camera_;
        }

        void RendererInterface::SetSceneInfoCallback(
            std::function<void(RendererSceneInfo&, Camera* camera, bool frustum_culling, bool force_camera_update)> callback)
        {
            update_scene_info_ = callback;
        }

        void RendererInterface::SetHeatmapData(const HeatmapData& heatmap_data)
        {
            if (heatmap_)
            {
                delete heatmap_;
            }
            heatmap_               = new Heatmap(heatmap_data);
            should_update_heatmap_ = true;
            MarkAsDirty();
        }

        RendererInterface* RendererFactory::CreateRenderer(RendererAdapterMap& renderer_adapter_map)
        {
            // Create a list of modules to be drawn in order.
            // RendererVulkan will take ownership of these modules.
            std::vector<RenderModule*> render_modules;

            // Add a clear checkerboard background module.
            render_modules.push_back(new CheckerClearRenderModule());

            // Add a traversal counter module.
            TraversalRenderModule* traversal_module = new TraversalRenderModule();
            traversal_module->Disable();
            render_modules.push_back(traversal_module);

            // Add a mesh render module.
            MeshRenderModule* blas_render_module = new MeshRenderModule();
            render_modules.push_back(blas_render_module);

            // Add a bounding box render module.
            BoundingVolumeRenderModule* bounding_volumes_module = new BoundingVolumeRenderModule();
            render_modules.push_back(bounding_volumes_module);

            // Add a selection render module.
            SelectionRenderModule* selection_module = new SelectionRenderModule();
            render_modules.push_back(selection_module);

            // Add the ray inspector render module.
            RayInspectorOverlayRenderModule* ray_inspector_module = new RayInspectorOverlayRenderModule();
            render_modules.push_back(ray_inspector_module);

            // Add orientation gizmo render module.
            OrientationGizmoRenderModule* orientation_gizmo_module = new OrientationGizmoRenderModule();
            render_modules.push_back(orientation_gizmo_module);

            // Create a new instance of a Vulkan renderer.
            RendererInterface* renderer = new RendererVulkan(render_modules);

            // Create an adapter to alter the render state.
            renderer_adapter_map[RendererAdapterType::kRendererAdapterTypeRenderState] =
                new RenderStateAdapter(renderer, blas_render_module, bounding_volumes_module, traversal_module, selection_module, ray_inspector_module);

            // Create an adapter to alter the camera state.
            renderer_adapter_map[RendererAdapterType::kRendererAdapterTypeView] = new ViewStateAdapter(&renderer->GetCamera());

            return renderer;
        }
    }  // namespace renderer
}  // namespace rra
