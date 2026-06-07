#pragma once

#include "CuteModel/BaseModel.h"
#include "CuteModel/TreeNode.h"

#include <QModelIndex>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace cute {

template <typename T>
class TreeModel : public BaseModel<T>
{
public:
    using Node = TreeNode<T>;

    explicit TreeModel(QObject *parent = nullptr)
        : BaseModel<T>(parent)
        , m_root(std::shared_ptr<TreeNode<T>>(
              new TreeNode<T>(typename TreeNode<T>::RootTag{}, this)))
    {}

    explicit TreeModel(QStringList headers, QObject *parent = nullptr)
        : BaseModel<T>(parent)
        , m_root(std::shared_ptr<TreeNode<T>>(
              new TreeNode<T>(typename TreeNode<T>::RootTag{}, this)))
        , m_headers(headers.isEmpty() ? QStringList{QString()} : std::move(headers))
    {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return nodeAt(parent)->childCount();
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override
    {
        return static_cast<int>(m_headers.size());
    }

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override
    {
        if (row < 0 || column < 0 || column >= static_cast<int>(m_headers.size()))
            return {};
        const auto parentNode = nodeAt(parent);
        if (row >= parentNode->childCount())
            return {};
        return this->createIndex(row, column, parentNode->child(row).get());
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        if (!child.isValid())
            return {};
        const auto parentNode = nodeAt(child)->parent();
        if (!parentNode || parentNode == m_root)
            return {};
        return this->createIndex(parentNode->indexInParent(), 0, parentNode.get());
    }

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override
    {
        return !nodeAt(parent)->empty();
    }

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole
            && section >= 0 && section < m_headers.size())
            return m_headers.at(section);
        return QAbstractItemModel::headerData(section, orientation, role);
    }

    bool insertRows(int row, int count,
                    const QModelIndex &parent = QModelIndex()) override
    {
        const auto parentNode = nodeAt(parent);
        if (row < 0 || row > parentNode->childCount() || count < 0)
            return false;
        if (count == 0)
            return true;

        if constexpr (std::is_default_constructible_v<T>) {
            std::vector<std::shared_ptr<TreeNode<T>>> fresh;
            fresh.reserve(static_cast<std::size_t>(count));
            for (int i = 0; i < count; ++i)
                fresh.push_back(std::make_shared<TreeNode<T>>(T{}));
            parentNode->insert_range(row, std::move(fresh));
            return true;
        } else {
            return false;
        }
    }

    bool removeRows(int row, int count,
                    const QModelIndex &parent = QModelIndex()) override
    {
        const auto parentNode = nodeAt(parent);
        if (row < 0 || count < 0 || row + count > parentNode->childCount())
            return false;
        if (count == 0)
            return true;
        parentNode->erase_children(row, row + count - 1);
        return true;
    }

    const std::shared_ptr<Node> &root() const noexcept { return m_root; }

    std::shared_ptr<Node> nodeAt(const QModelIndex &index) const
    {
        if (!index.isValid())
            return m_root;
        auto *node = static_cast<TreeNode<T> *>(index.internalPointer());
        return node->shared_from_this();
    }

    void clear()
    {
        if (m_root->m_children.empty())
            return;
        this->beginResetModel();
        std::vector<std::shared_ptr<TreeNode<T>>> old;
        old.swap(m_root->m_children);
        for (auto &c : old)
            c->detachSubtree();
        this->endResetModel();
    }

protected:
    const T &getStorageValue(const QModelIndex &index) const override
    {
        return nodeAt(index)->payloadRef();
    }

    void setStorageValue(const QModelIndex &index, T value) override
    {
        const auto node = nodeAt(index);
        node->constructPayload(std::move(value));
        const QModelIndex parentIdx = this->parent(index);
        const int lastCol = static_cast<int>(m_headers.size()) - 1;
        const QModelIndex left = this->index(index.row(), 0, parentIdx);
        const QModelIndex right = (lastCol > 0)
            ? this->index(index.row(), lastCol, parentIdx)
            : left;
        emit this->dataChanged(left, right);
    }

private:
    friend class TreeNode<T>;

    std::shared_ptr<Node> m_root;
    QStringList m_headers{QString()};
};

}
