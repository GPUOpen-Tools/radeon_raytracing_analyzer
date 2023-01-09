//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the BLAS properties model.
//=============================================================================

#ifndef RRA_MODELS_BLAS_BLAS_PROPERTIES_MODEL_H_
#define RRA_MODELS_BLAS_BLAS_PROPERTIES_MODEL_H_

#include <QVariant>
#include "qt_common/utils/model_view_mapper.h"

namespace rra
{
    /// @brief Enum containing indices for the widgets shared between the model and UI.
    enum BlasPropertiesWidgets
    {
        kBlasPropertiesBaseAddress,

        kBlasPropertiesNumNodes,
        kBlasPropertiesNumBoxNodes,
        kBlasPropertiesNumBox16Nodes,
        kBlasPropertiesNumBox32Nodes,
        kBlasPropertiesNumTriangleNodes,
        kBlasPropertiesNumProceduralNodes,
        kBlasPropertiesNumTriangles,
        kBlasPropertiesNumInstances,

        kBlasPropertiesBuildFlagAllowUpdate,
        kBlasPropertiesBuildFlagAllowCompaction,
        kBlasPropertiesBuildFlagLowMemory,
        kBlasPropertiesBuildFlagBuildType,

        kBlasPropertiesMemory,

        kBlasPropertiesRootSAH,
        kBlasPropertiesMinSAH,
        kBlasPropertiesMeanSAH,
        kBlasPropertiesMaxDepth,
        kBlasPropertiesAvgDepth,

        kBlasPropertiesNumWidgets,
    };

    /// @brief Container class that holds model data for the BLAS list pane.
    class BlasPropertiesModel : public ModelViewMapper
    {
    public:
        /// @brief Constructor.
        ///
        /// param [in] num_model_widgets the number of widgets that require updating by the model.
        explicit BlasPropertiesModel(int32_t num_model_widgets);

        /// @brief Destructor.
        virtual ~BlasPropertiesModel();

        /// @brief Initialize blank data for the model.
        void ResetModelValues();

        /// @brief Update the model.
        ///
        /// @param [in] tlas_index  The index of the TLAS used to update the model.
        /// @param [in] blas_index  The index of the BLAS used to update the model.
        void Update(uint64_t tlas_index, uint64_t blas_index);

    private:
    };
}  // namespace rra

#endif  // RRA_MODELS_BLAS_BLAS_PROPERTIES_MODEL_H_
