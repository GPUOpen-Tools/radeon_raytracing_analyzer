//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the TLAS properties model.
//=============================================================================

#ifndef RRA_MODELS_TLAS_TLAS_PROPERTIES_MODEL_H_
#define RRA_MODELS_TLAS_TLAS_PROPERTIES_MODEL_H_

#include <QVariant>
#include "qt_common/utils/model_view_mapper.h"

namespace rra
{
    /// @brief Enum containing indices for the widgets shared between the model and UI.
    enum TlasPropertiesWidgets
    {
        kTlasPropertiesBaseAddress,

        kTlasPropertiesNumNodes,
        kTlasPropertiesNumBoxNodes,
        kTlasPropertiesNumBox16Nodes,
        kTlasPropertiesNumBox32Nodes,
        kTlasPropertiesNumInstanceNodes,
        kTlasPropertiesNumBlases,
        kTlasPropertiesNumTriangles,

        kTlasPropertiesBuildFlagAllowUpdate,
        kTlasPropertiesBuildFlagAllowCompaction,
        kTlasPropertiesBuildFlagLowMemory,
        kTlasPropertiesBuildFlagBuildType,

        kTlasPropertiesMemoryTlas,
        kTlasPropertiesMemoryTotal,

        kTlasPropertiesNumWidgets,
    };

    /// @brief Container class that holds model data for the BLAS list pane.
    class TlasPropertiesModel : public ModelViewMapper
    {
    public:
        /// @brief Constructor.
        ///
        /// param [in] num_model_widgets the number of widgets that require updating by the model.
        explicit TlasPropertiesModel(int32_t num_model_widgets);

        /// @brief Destructor.
        virtual ~TlasPropertiesModel();

        /// @brief Initialize blank data for the model.
        void ResetModelValues();

        /// @brief Update the model.
        ///
        /// @param [in] tlas_index  The index of the TLAS used to update the table.
        void Update(uint64_t tlas_index);

    private:
    };
}  // namespace rra

#endif  // RRA_MODELS_TLAS_TLAS_PROPERTIES_MODEL_H_
