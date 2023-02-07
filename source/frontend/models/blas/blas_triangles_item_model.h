//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the BLAS triangles item model.
//=============================================================================

#ifndef RRA_MODELS_BLAS_BLAS_TRIANGLES_ITEM_MODEL_H_
#define RRA_MODELS_BLAS_BLAS_TRIANGLES_ITEM_MODEL_H_

#include <QAbstractItemModel>

#include "qt_common/custom_widgets/scaled_table_view.h"

#include "public/rra_bvh.h"
#include "public/shared.h"

namespace rra
{
    /// @brief Structure describing the statistics needed for the Triangles list pane.
    struct BlasTrianglesStatistics
    {
        uint32_t              node_id;                             ///< The node ID.
        uint32_t              primitive_index;                     ///< The primitive index.
        uint64_t              triangle_address;                    ///< The base address of the triangle.
        uint64_t              triangle_offset;                     ///< The offset of the triangle in the TLAS.
        uint32_t              geometry_index;                      ///< The geometry index.
        bool                  geometry_flag_opaque;                ///< The opaque geometry flag.
        bool                  geometry_flag_no_duplicate_any_hit;  ///< The no duplicate anyhit invocation geometry flag.
        bool                  is_inactive;                         ///< Whether or not this triangle is inactive.
        float                 triangle_surface_area;               ///< The triangle surface area.
        float                 sah;                                 ///< The surface area heuristic.
        rra::renderer::float3 vertex_0;                            ///< Triangle vertex 0.
        rra::renderer::float3 vertex_1;                            ///< Triangle vertex 1.
        rra::renderer::float3 vertex_2;                            ///< Triangle vertex 2.
    };

    /// @brief Column Id's for the fields in the triangle list.
    enum BlasTrianglesColumn
    {
        kBlasTrianglesColumnGeometryIndex,
        kBlasTrianglesColumnGeometryFlagOpaque,
        kBlasTrianglesColumnGeometryFlagNoDuplicateAnyHit,
        kBlasTrianglesColumnPrimitiveIndex,
        kBlasTrianglesColumnNodeAddress,
        kBlasTrianglesColumnNodeOffset,
        kBlasTrianglesColumnActive,
        kBlasTrianglesColumnTriangleSurfaceArea,
        kBlasTrianglesColumnSAH,
        kBlasTrianglesColumnVertex0,
        kBlasTrianglesColumnVertex1,
        kBlasTrianglesColumnVertex2,

        kBlasTrianglesColumnCount,
    };

    /// @brief A class to handle the model data associated with BLAS list table.
    class BlasTrianglesItemModel : public QAbstractItemModel
    {
    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit BlasTrianglesItemModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~BlasTrianglesItemModel();

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

        /// @brief Add a triangle structure to the table.
        ///
        /// @param [in] stats  The statistics to add to the table.
        void AddTriangleStructure(const BlasTrianglesStatistics& stats);

        // QAbstractItemModel overrides. See Qt documentation for parameter and return values
        virtual QVariant      data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual QVariant      headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
        virtual QModelIndex   index(int row, int column, const QModelIndex& parent) const Q_DECL_OVERRIDE;
        virtual QModelIndex   parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
        virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

    private:
        int                                  num_rows_;     ///< The number of rows in the table.
        int                                  num_columns_;  ///< The number of columns in the table.
        std::vector<BlasTrianglesStatistics> cache_;        ///< Cached data from the backend.
    };
}  // namespace rra

#endif  // RRA_MODELS_BLAS_BLAS_TRIANGLES_ITEM_MODEL_H_
