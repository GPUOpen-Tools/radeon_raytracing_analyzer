//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the BLAS geometries item model.
//=============================================================================

#ifndef RRA_MODELS_BLAS_BLAS_GEOMETRIES_ITEM_MODEL_H_
#define RRA_MODELS_BLAS_BLAS_GEOMETRIES_ITEM_MODEL_H_

#include <QAbstractItemModel>

#include "qt_common/custom_widgets/scaled_table_view.h"

#include "public/rra_bvh.h"
#include "public/shared.h"

namespace rra
{
    /// @brief Structure describing the statistics needed for the Geometry list pane.
    struct BlasGeometriesStatistics
    {
        uint32_t geometry_index;                      ///< The geometry index.
        bool     geometry_flag_opaque;                ///< The opaque geometry flag.
        bool     geometry_flag_no_duplicate_any_hit;  ///< The no duplicate anyhit invocation geometry flag.
        uint32_t primitive_count;                     ///< The number of primitives (eg triangles) in this geometry.
    };

    /// @brief Column Id's for the fields in the geometry list.
    enum BlasGeometriesColumn
    {
        kBlasGeometriesColumnGeometryIndex,
        kBlasGeometriesColumnGeometryFlagOpaque,
        kBlasGeometriesColumnGeometryFlagNoDuplicateAnyHit,
        kBlasGeometriesColumnPrimitiveCount,
        kBlasGeometriesColumnPadding,

        kBlasGeometriesColumnCount,
    };

    /// @brief A class to handle the model data associated with BLAS list table.
    class BlasGeometriesItemModel : public QAbstractItemModel
    {
    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit BlasGeometriesItemModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~BlasGeometriesItemModel();

        /// @brief Set the number of rows in the table.
        ///
        /// @param [in] rows The number of rows required.
        void SetRowCount(int rows);

        /// @brief Set the number of columns in the table.
        ///
        /// @param [in] columns The number of columns required.
        void SetColumnCount(int columns);

        /// @brief Initialize the acceleration structure list table.
        ///
        /// @param [in] acceleration_structure_table  The table to initialize.
        void Initialize(ScaledTableView* acceleration_structure_table);

        /// @brief Add a geometry structure to the table.
        ///
        /// @param [in] stats  The statistics to add to the table.
        void AddGeometryStructure(const BlasGeometriesStatistics& stats);

        // QAbstractItemModel overrides. See Qt documentation for parameter and return values
        virtual QVariant      data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual QVariant      headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
        virtual QModelIndex   index(int row, int column, const QModelIndex& parent) const Q_DECL_OVERRIDE;
        virtual QModelIndex   parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
        virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

    private:
        int                                   num_rows_;     ///< The number of rows in the table.
        int                                   num_columns_;  ///< The number of columns in the table.
        std::vector<BlasGeometriesStatistics> cache_;        ///< Cached data from the backend.
    };
}  // namespace rra

#endif  // RRA_MODELS_BLAS_BLAS_GEOMETRIES_ITEM_MODEL_H_
