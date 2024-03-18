//=============================================================================
// Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the dispatch pane.
//=============================================================================

#ifndef RRA_VIEWS_OVERVIEW_DISPATCH_PANE_H_
#define RRA_VIEWS_OVERVIEW_DISPATCH_PANE_H_

#include "ui_dispatch_pane.h"

#include <QWidget>
#include <QTimer>

#include "qt_common/custom_widgets/colored_legend_scene.h"
#include "qt_common/custom_widgets/colored_legend_graphics_view.h"
#include <public/rra_ray_history.h>

/// @brief Class declaration.
class DispatchPane : public QWidget
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent Pointer to the parent widget.
    explicit DispatchPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~DispatchPane();

    /// @brief Overridden Qt show event. Fired when this pane is opened.
    ///
    /// @param [in] event The show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

    /// @brief Set the dispatch Id in the UI.
    ///
    /// @param [in] dispatch_id  The dispatch id.
    void SetDispatchId(uint32_t dispatch_id);

    /// @brief Set the dispatch parameters in the UI.
    ///
    /// @param [in] dispatch_name The name of the dispatch.
    /// @param [in] width         The dispatch width.
    /// @param [in] height        The dispatch height.
    /// @param [in] depth         The dispatch depth.
    void SetDispatchDimensions(const QString& dispatch_name, int width, int height, int depth);

    /// @brief Set the dispatch traversal parameters in the UI.
    ///
    /// @param [in] ray_count                   The number of rays in this dispatch.
    /// @param [in] loop_iteration_count        The loop iteration count.
    /// @param [in] instance_intersection_count The instance intersection count.
    /// @param [in] rays_per_pixel              The total rays divided by pixel count.
    void SetTraversalParameters(uint64_t ray_count, uint64_t loop_iteration_count, uint64_t instance_intersection_count, float rays_per_pixel);

    /// @brief Set the shader invocation parameters in the UI.
    ///
    /// @param [in] dispatch_type      The type of the dispatch.
    /// @param [in] raygen_count       The number of RayGen shader invocations.
    /// @param [in] closest_hit_count  The number of closest hit shader invocations.
    /// @param [in] any_hit_count      The number of any hit shader invocations.
    /// @param [in] intersection_count The number of intersection shader invocations.
    /// @param [in] miss_count         The number of miss shader invocations.
    void SetInvocationParameters(DispatchType dispatch_type,
                                 uint64_t     raygen_count,
                                 uint64_t     closest_hit_count,
                                 uint64_t     any_hit_count,
                                 uint64_t     intersection_count,
                                 uint64_t     miss_count);

private slots:
    /// @brief Switch to the ray history pane.
    ///
    /// This is called when the user clicks on a dispatch from the overview pane.
    void NavigateToRayHistory();

private:
    /// @brief Configure the dispatch pane for ray tracing pipeline.
    void ConfigureForRaytracingPipeline();

    /// @brief Configure the dispatch pane for ray query.
    void ConfigureForComputeOrGraphicsDispatch();

    /// @brief Update the dispatch pane.
    void Update();

    /// @brief enumeration of shader invocation types.
    enum
    {
        kInvocationRaygen,
        kInvocationClosestHit,
        kInvocationAnyHit,
        kInvocationIntersection,
        kInvocationMiss,
        kInvocationCallable,

        kNumInvocations
    };

    Ui_DispatchPane* ui_;               ///< The Qt Instance of this object.
    uint32_t         dispatch_id_ = 0;  ///< The dispatch ID associated with this pane.
    QTimer           timer_;            ///< A timer used to redraw the widget at a specific rate.
};

#endif  // RRA_VIEWS_OVERVIEW_DISPATCH_PANE_H_