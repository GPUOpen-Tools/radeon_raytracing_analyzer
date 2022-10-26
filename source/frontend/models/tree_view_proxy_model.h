//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for a proxy filter used to filter a tree view.
//=============================================================================

#ifndef RRA_MODELS_TREE_VIEW_PROXY_MODEL_H_
#define RRA_MODELS_TREE_VIEW_PROXY_MODEL_H_

#include <set>
#include <QSortFilterProxyModel>

namespace rra
{
    /// @brief Base class to filter out and sort a table.
    class TreeViewProxyModel : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit TreeViewProxyModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~TreeViewProxyModel();

        /// @brief Notify the proxy model of the search string.
        ///
        /// @param search_text The search string the user has selected.
        void SetSearchText(const QString& search_text);

    private:
        /// @brief Make the filter run across multiple columns.
        ///
        /// @param [in] source_row    The target row.
        /// @param [in] source_parent The source parent.
        ///
        /// @return true if the row passed the filter, false if not.
        virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

        QString search_text_;  ///< The search string.
    };
}  // namespace rra

#endif  // RRA_MODELS_TREE_VIEW_PROXY_MODEL_H_
