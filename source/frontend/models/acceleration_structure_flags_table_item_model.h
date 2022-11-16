//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the flags table item model.
///
/// Used for the flags table shown in the TLAS left pane.
///
//=============================================================================

#ifndef RRA_MODELS_TLAS_FLAGS_TABLE_ITEM_MODEL_H_
#define RRA_MODELS_TLAS_FLAGS_TABLE_ITEM_MODEL_H_

#include <QAbstractItemModel>

#include "vulkan/include/vulkan/vulkan_core.h"

#include "qt_common/custom_widgets/scaled_table_view.h"

#include "public/rra_bvh.h"

namespace rra
{
    /// @brief Column Id's for the fields in the acceleration structure list.
    enum FlagsTableColumn
    {
        kInstanceFlagsTableColumnCheckbox,
        kInstanceFlagsTableColumnFlag,
    };

    /// @brief A class to handle the model data associated with BLAS list table.
    class FlagsTableItemModel : public QAbstractItemModel
    {
    public:
        /// @brief Constructor.
        explicit FlagsTableItemModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~FlagsTableItemModel();

        /// @brief Set the number of rows in the table.
        ///
        /// @param [in] rows The number of rows required.
        void SetRowCount(int rows);

        /// @brief Set the number of columns in the table.
        ///
        /// @param [in] columns The number of columns required.
        void SetColumnCount(int columns);

        /// @brief Initialize the flag table.
        ///
        /// @param [in] table_view  The table to initialize.
        void Initialize(ScaledTableView* table_view);

        /// @brief Set the checkmark status of a row in the table.
        ///
        /// @param [in] row     The row which will have its checkmark status set.
        /// @param [in] checked Whether the row should be checked or unchecked.
        void SetRowChecked(int row, bool checked);

        /// @brief Set the flag name of a row in the table.
        ///
        /// @param [in] row       The row to set the flag name of.
        /// @param [in] flag_name The flag name.
        void SetRowFlagName(int row, QString flag_name);

        // QAbstractItemModel overrides. See Qt documentation for parameter and return values
        virtual QVariant      data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual QModelIndex   index(int row, int column, const QModelIndex& parent) const Q_DECL_OVERRIDE;
        virtual QModelIndex   parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
        virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

    private:
        struct RowData
        {
            bool    checked{};
            QString flag_name{};
        };

        int                  num_rows_{};     ///< The number of rows in the table.
        int                  num_columns_{};  ///< The number of columns in the table.
        std::vector<RowData> row_data_{};     ///> The data for each row of the table.
    };
}  // namespace rra

#endif  // RRA_MODELS_TLAS_FLAGS_TABLE_ITEM_MODEL_H_
