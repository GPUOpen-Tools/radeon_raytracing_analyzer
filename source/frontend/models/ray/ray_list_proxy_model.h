//=============================================================================
// Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for a proxy filter that processes the ray list table.
//=============================================================================

#ifndef RRA_MODELS_RAY_LIST_PROXY_MODEL_H_
#define RRA_MODELS_RAY_LIST_PROXY_MODEL_H_

#include <QTableView>

#include "models/table_proxy_model.h"
#include "models/ray/ray_list_item_model.h"

#undef max

namespace rra
{
    /// @brief Class to filter out and sort the BLAS list table.
    class RayListProxyModel : public TableProxyModel
    {
        Q_OBJECT

    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit RayListProxyModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~RayListProxyModel();

        /// @brief Initialize the ray table model.
        ///
        /// @param [in] view        The table view.
        /// @param [in] num_rows    The table row count.
        /// @param [in] num_columns The table column count.
        ///
        /// @return the model for the ray table model.
        RayListItemModel* InitializeRayTableModels(QTableView* view, int num_rows, int num_columns);

        /// @brief Set whether rays in the list should be hidden based on their global invocation ID.
        ///
        /// @param min Filter rows with invocation IDs less than min.
        /// @param max Filter rows with invocation IDs greater than max.
        void SetFilterByInvocationId(GlobalInvocationID min, GlobalInvocationID max);

        /// @brief Set the dimension of the reshaped dispatch.
        ///
        /// @param x The reshaped x dimension.
        /// @param y The reshaped y dimension.
        /// @param z The reshaped z dimension.
        void SetReshapeDimension(uint32_t x, uint32_t y, uint32_t z);

        /// @brief Get the selected box extents of the heatmap image.
        ///
        /// @param filter_min_out Minimum filter extents.
        /// @param filter_max_out Maximum filter extents.
        void GetFilterMinAndMax(GlobalInvocationID* filter_min_out, GlobalInvocationID* filter_max_out);

    protected:
        /// @brief Make the filter run across multiple columns.
        ///
        /// @param [in] source_row    The target row.
        /// @param [in] source_parent The source parent.
        ///
        /// @return true if the row passed the filter, false if not.
        virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

        /// @brief The sort comparator.
        ///
        /// @param [in] left  The left item to compare.
        /// @param [in] right The right item to compare.
        ///
        /// @return true if left is less than right, false otherwise.
        virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

    private:
        uint32_t           reshape_width_{1};     ///< The reshaped x dimension.
        uint32_t           reshape_height_{1};    ///< The reshaped y dimension.
        uint32_t           reshape_depth_{1};     ///< The reshaped z dimension.
        GlobalInvocationID filter_min_{0, 0, 0};  ///< Hide rows with a global invocaion ID less than this value.
        GlobalInvocationID filter_max_{std::numeric_limits<uint32_t>::max(),
                                       std::numeric_limits<uint32_t>::max(),
                                       std::numeric_limits<uint32_t>::max()};  ///< Hide rows with a global invocaion ID greater than this value.
    };
}  // namespace rra

#endif  // RRA_MODELS_RAY_LIST_PROXY_MODEL_H_
