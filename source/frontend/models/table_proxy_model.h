//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for a proxy filter that processes multiple columns.
//=============================================================================

#ifndef RRA_MODELS_TABLE_PROXY_MODEL_H_
#define RRA_MODELS_TABLE_PROXY_MODEL_H_

#include <set>
#include <QSortFilterProxyModel>

namespace rra
{
    /// @brief Base class to filter out and sort a table.
    class TableProxyModel : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit TableProxyModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~TableProxyModel();

        /// @brief Specify which columns should be sorted/filtered.
        ///
        /// @param [in] columns A list of columns to filter.
        void SetFilterKeyColumns(const QList<qint32>& columns);

        /// @brief Specify string to use as search filter.
        ///
        /// @param [in] filter The search filter.
        void SetSearchFilter(const QString& filter);

        /// @brief Specify range to use as size filter.
        ///
        /// @param [in] min The minimum size.
        /// @param [in] max The maximum size.
        void SetSizeFilter(uint64_t min, uint64_t max);

        /// @brief Get content from proxy model.
        ///
        /// @param [in] row    The row where the data is located.
        /// @param [in] column The column where the data is located.
        ///
        /// @return The contents at row, column.
        qulonglong GetData(int row, int column);

        /// @brief Find a model index corresponding to the passed in data.
        ///
        /// @param [in] lookup The value to find.
        /// @param [in] column The column to search.
        ///
        /// @return the model index containing the data.
        QModelIndex FindModelIndex(qulonglong lookup, int column) const;

    protected:
        /// @brief Make the filter run across multiple columns.
        ///
        /// Must be implemented by derived classes.
        ///
        /// @param [in] source_row    The target row.
        /// @param [in] source_parent The source parent.
        ///
        /// @return true if the row passed the filter, false if not.
        virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const = 0;

        /// @brief Implement the comparison for sorting.
        ///
        /// Must be implemented by derived classes.
        ///
        /// @param [in] left  The left item to compare.
        /// @param [in] right The right item to compare.
        ///
        /// @return If left < right then true, else false.
        virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const = 0;

        /// @brief Filter the search string.
        ///
        /// @param [in] row           The row to apply the size filter to.
        /// @param [in] source_parent The parent model index in the source model.
        ///
        /// @return true if the table item at row,column is to be shown, false if not.
        bool FilterSearchString(int row, const QModelIndex& source_parent) const;

        /// @brief Extract a uint64_t from a model index.
        ///
        /// @param [in] index The model index of where the required data is.
        ///
        /// @return the uint64 value stored at the model index.
        uint64_t GetIndexValue(const QModelIndex& index) const;

        std::set<qint32> column_filters_;  ///< Holds which columns are being filtered.
        QString          search_filter_;   ///< The current search string.
        uint64_t         min_size_;        ///< The minimum size of the size filter.
        uint64_t         max_size_;        ///< The maximum size of the size filter.
    };
}  // namespace rra

#endif  // RRA_MODELS_TABLE_PROXY_MODEL_H_
