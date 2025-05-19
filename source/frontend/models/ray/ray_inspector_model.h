//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the ray inspector model.
//=============================================================================

#ifndef RRA_MODELS_RAY_INSPECTOR_MODEL_H_
#define RRA_MODELS_RAY_INSPECTOR_MODEL_H_

#include <optional>

#include "qt_common/custom_widgets/scaled_table_view.h"
#include "qt_common/utils/model_view_mapper.h"

#include "public/renderer_interface.h"
#include "public/renderer_types.h"
#include "public/rra_ray_history.h"

#include "io/viewer_io.h"
#include "models/acceleration_structure_flags_table_item_model.h"
#include "models/ray/ray_inspector_ray_tree_model.h"
#include "models/ray/ray_inspector_ray_tree_proxy_model.h"
#include "ray_inspector_scene_collection_model.h"

namespace rra
{
    /// @brief Calculate the near plane by casting rays to detect closest triangle.
    ///
    /// @param scene  The BVH scene.
    /// @param camera The renderer camera.
    ///
    /// @return The optimal near plane distance.
    float GetNearPlane(Scene* scene, rra::renderer::Camera* camera);

    /// @brief The unique dispatch index to identify a "thread" that launches rays consecutively.
    struct RayInspectorKey
    {
        uint64_t           dispatch_id;
        GlobalInvocationID invocation_id;
    };

    /// @brief Enum containing indices for the widgets shared between the model and UI.
    enum RayInspectorRayListWidgets
    {
        kRayInspectorRayListNumWidgets,
    };

    /// @brief Container class that holds model data for the ray history pane.
    class RayInspectorModel : public ModelViewMapper
    {
    public:
        /// @brief Constructor.
        ///
        /// param [in] num_model_widgets the number of widgets that require updating by the model.
        explicit RayInspectorModel(int32_t num_model_widgets);

        /// @brief Destructor.
        virtual ~RayInspectorModel();

        /// @brief Initialize blank data for the model.
        void ResetModelValues();

        /// @brief Initialize the table model.
        ///
        /// @param [in] table_view  The view to the table.
        /// @param [in] num_rows    Total rows of the table.
        /// @param [in] num_columns Total columns of the table.
        void InitializeTreeModel(ScaledTreeView* tree_view);

        /// @brief Update the ray list table.
        ///
        /// @param [in] key The key to find a rays at a dispatch coordinate.
        void SetKey(RayInspectorKey key);

        /// @brief Clear key along with the cached ray data.
        void ClearKey();

        /// @brief  Returns the current key.
        ///
        /// @return The key to find rays at a dispatch coordinate.
        RayInspectorKey GetKey();

        /// @brief Select the ray index.
        ///
        /// @param [in] ray_index The ray index to select.
        void SelectRayIndex(uint32_t ray_index);

        /// @brief Get the selected ray index.
        ///
        /// @return the selected ray index.
        uint32_t GetSelectedRayIndex();

        /// @brief Get the selected tlas index.
        ///
        /// @return the selected tlas index if valid.
        std::optional<uint64_t> GetSelectedTlasIndex();

        /// @brief Get the proxy model. Used to set up a connection between the table being sorted and the UI update.
        ///
        /// @return the proxy model.
        RayInspectorRayTreeProxyModel* GetProxyModel() const;

        /// @brief Toggle the instance transform wireframe rendering.
        void ToggleInstanceTransformWireframe();

        /// @brief Toggle the BVH wireframe rendering.
        void ToggleBVHWireframe();

        /// @brief Toggle the Mesh wireframe rendering.
        void ToggleMeshWireframe();

        /// @brief Toggle the rendering og geometry.
        void ToggleRenderGeometry();

        /// @brief Adapts the counter range to the view.
        void AdaptTraversalCounterRangeToView();

        /// @brief Get a ray from the dispatch index.
        ///
        /// @param index The index into this dispatch index's rays.
        ///
        /// @return The ray if valid.
        std::optional<Ray> GetRay(uint32_t index) const;

        /// @brief Get a ray result from the dispatch index.
        ///
        /// @param index The index into this dispatch index's rays.
        ///
        /// @return The ray result if valid.
        std::optional<RraIntersectionResult> GetRayResult(uint32_t index) const;

        /// @brief Get current ray count.
        uint32_t GetRayCount() const;

        /// @brief Populate the scene with rendering data from the BVH at the selected ray.
        ///
        /// @param [in] renderer The renderer to use to display the BVH scene.
        void PopulateScene(renderer::RendererInterface* renderer);

        /// @brief Refreshes the tlas map.
        void UpdateTlasMap();

        /// @brief Get the viewer callbacks.
        ///
        /// @return the viewer callbacks.
        ViewerIOCallbacks GetViewerCallbacks();

        /// @brief Initialize the flags table model, used by the flags table in the viewer left-side pane.
        ///
        /// @param [in] table_view  The table view widget.
        void InitializeFlagsTableModel(ScaledTableView* table_view);

        /// @brief Populate the flags table.
        ///
        /// @param [in] flags_table The flags table to populate.
        void PopulateFlagsTable(FlagsTableItemModel* flags_table);

        /// @brief Resets the camera for multiple frames to make sure it converges on the view and perspective.
        void BurstResetCamera();

        /// @brief Resets the camera for multiple frames to make sure it converges on the view and perspective.
        void BurstUpdateCamera();

        /// @brief Get the camera fit function for the selected ray.
        ///
        /// @returns The camera fit function.
        std::function<ViewerFitParams(rra::renderer::Camera*)> GetCameraFitFunction();

    public slots:
        /// @brief Connect the incoming map of RendererAdapter instances with the model.
        ///
        /// @param [in] adapters A renderer adapter map used to alter various render states.
        void SetRendererAdapters(const rra::renderer::RendererAdapterMap& adapters);

    private:
        /// @brief Get the renderable rays from current rays.
        ///
        /// @param [out] out_first_ray_outline The index into the returned array of the consecutive outline rays.
        /// @param [out] out_outline_count     The number of outline rays.
        ///
        /// @return a list of renderable rays.
        std::vector<renderer::RayInspectorRay> GetRenderableRays(uint32_t* out_first_ray_outline, uint32_t* out_outline_count);

        RayInspectorKey                        key_ = {};                           ///< Dispatch identifiers
        RayInspectorRayTreeModel*              tree_model_;                         ///< Holds the ray tree data.
        RayInspectorRayTreeProxyModel*         proxy_model_;                        ///< Proxy model for the ray tree.
        RayInspectorSceneCollectionModel*      scene_collection_model_;             ///< The scene collection model.
        QStandardItemModel*                    stats_table_model_ = nullptr;        ///< Model associated with the stats table.
        std::vector<Ray>                       rays_{};                             ///< The rays currently being shown in inspector.
        std::vector<RraIntersectionResult>     results_{};                          ///< The result of each ray in the inspector.
        uint32_t                               ray_index_ = 0;                      ///< The ray index to render.
        std::unordered_map<uint64_t, uint64_t> tlas_address_to_index_;              ///< A map to keep track of tlas addresses to index.
        rra::FlagsTableItemModel*              flags_table_model_       = nullptr;  ///< Model associated with the flags table.
        uint32_t                               camera_reset_countdown_  = 3;        ///< To keep track of camera reset to allow the renderer to adjust.
        uint32_t                               camera_update_countdown_ = 3;  ///< To keep track of camera update without reset to allow the renderer to adjust.
        rra::renderer::RenderStateAdapter*     render_state_adapter_    = nullptr;  ///< The adapter used to toggle mesh render states.
    };
}  // namespace rra

#endif  // RRA_MODELS_RAY_INSPECTOR_MODEL_H_

