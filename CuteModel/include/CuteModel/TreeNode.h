#pragma once

#include <QModelIndex>
#include <QtGlobal>

#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace cute {

template <typename Node>
class TreeModel;

template <typename T>
class TreeNode : public std::enable_shared_from_this<TreeNode<T>>
{
public:
    using payload_type = T;

    template <typename First, typename... Rest,
              typename = std::enable_if_t<
                  !std::is_same_v<std::decay_t<First>, TreeNode>>>
    explicit TreeNode(First &&first, Rest &&...rest)
    {
        ::new (static_cast<void *>(m_payloadStorage.data()))
            T(std::forward<First>(first), std::forward<Rest>(rest)...);
    }

    TreeNode(const TreeNode &) = delete;
    TreeNode(TreeNode &&) = delete;
    TreeNode &operator=(const TreeNode &) = delete;
    TreeNode &operator=(TreeNode &&) = delete;

    ~TreeNode()
    {
        if (!isRoot())
            payloadRef().~T();
    }

    const T &payload() const noexcept
    {
        Q_ASSERT(!isRoot());
        return payloadRef();
    }

    void setPayload(T value)
    {
        if (isRoot())
            return;
        if (m_model)
            m_model->setStorageValue(modelIndex(0), std::move(value));
        else
            constructPayload(std::move(value));
    }

    std::shared_ptr<TreeNode<T>> parent() const noexcept { return m_parent.lock(); }
    int indexInParent() const noexcept { return m_indexInParent; }

    int childCount() const noexcept { return static_cast<int>(m_children.size()); }
    bool empty() const noexcept { return m_children.empty(); }

    const std::shared_ptr<TreeNode<T>> &child(int row) const
    {
        Q_ASSERT(row >= 0 && row < childCount());
        return m_children[static_cast<std::size_t>(row)];
    }

    const std::vector<std::shared_ptr<TreeNode<T>>> &children() const noexcept
    {
        return m_children;
    }

    TreeModel<TreeNode<T>> *model() const noexcept { return m_model; }

    QModelIndex modelIndex(int column = 0) const
    {
        if (!m_model || m_indexInParent < 0)
            return {};
        return m_model->createIndex(m_indexInParent, column,
                                    const_cast<TreeNode<T> *>(this));
    }

    void push_back_child(std::shared_ptr<TreeNode<T>> child)
    {
        insert_child(childCount(), std::move(child));
    }

    void insert_child(int row, std::shared_ptr<TreeNode<T>> child)
    {
        std::vector<std::shared_ptr<TreeNode<T>>> single;
        single.push_back(std::move(child));
        insert_range(row, std::move(single));
    }

    void append_range(std::vector<std::shared_ptr<TreeNode<T>>> children)
    {
        insert_range(childCount(), std::move(children));
    }

    void insert_range(int row, std::vector<std::shared_ptr<TreeNode<T>>> children)
    {
        if (children.empty() || row < 0 || row > childCount())
            return;
        for (const auto &c : children)
            if (!c)
                return;

        const int count = static_cast<int>(children.size());
        const int last = row + count - 1;
        const QModelIndex parentIdx = modelIndex(0);
        if (m_model)
            m_model->beginInsertRows(parentIdx, row, last);

        for (int i = 0; i < count; ++i) {
            auto &c = children[static_cast<std::size_t>(i)];
            c->m_parent = this->weak_from_this();
            c->m_indexInParent = row + i;
            if (m_model)
                propagateModelDown(c.get(), m_model);
        }

        m_children.insert(m_children.begin() + row,
                          std::make_move_iterator(children.begin()),
                          std::make_move_iterator(children.end()));
        renumberFrom(row + count);

        if (m_model)
            m_model->endInsertRows();
    }

    void erase_child(int row) { erase_children(row, row); }

    void erase_children(int first, int last)
    {
        if (first < 0 || last < first || last >= childCount())
            return;

        const QModelIndex parentIdx = modelIndex(0);
        if (m_model)
            m_model->beginRemoveRows(parentIdx, first, last);

        for (int i = first; i <= last; ++i)
            m_children[static_cast<std::size_t>(i)]->detachSubtree();

        m_children.erase(m_children.begin() + first, m_children.begin() + last + 1);
        renumberFrom(first);

        if (m_model)
            m_model->endRemoveRows();
    }

    void clear_children()
    {
        if (!m_children.empty())
            erase_children(0, childCount() - 1);
    }

private:
    friend class TreeModel<TreeNode<T>>;

    struct RootTag {};

    TreeNode(RootTag, TreeModel<TreeNode<T>> *model) : m_model(model) {}

    template <typename... CtorArgs>
    void constructPayload(CtorArgs &&...ctorArgs)
    {
        payloadRef().~T();
        ::new (static_cast<void *>(m_payloadStorage.data()))
            T(std::forward<CtorArgs>(ctorArgs)...);
    }

    bool isRoot() const noexcept { return m_model && m_indexInParent < 0; }

    T &payloadRef() noexcept
    {
        return *std::launder(reinterpret_cast<T *>(m_payloadStorage.data()));
    }

    const T &payloadRef() const noexcept
    {
        return *std::launder(reinterpret_cast<const T *>(m_payloadStorage.data()));
    }

    void renumberFrom(int firstRow)
    {
        const int count = childCount();
        for (int i = firstRow; i < count; ++i)
            m_children[static_cast<std::size_t>(i)]->m_indexInParent = i;
    }

    static void propagateModelDown(TreeNode<T> *node, TreeModel<TreeNode<T>> *model)
    {
        node->m_model = model;
        for (auto &c : node->m_children)
            propagateModelDown(c.get(), model);
    }

    void detachSubtree() noexcept
    {
        m_parent.reset();
        m_indexInParent = -1;
        clearModelInSubtree();
    }

    void clearModelInSubtree() noexcept
    {
        m_model = nullptr;
        for (auto &c : m_children)
            c->clearModelInSubtree();
    }

    TreeModel<TreeNode<T>> *m_model = nullptr;
    std::weak_ptr<TreeNode<T>> m_parent;
    int m_indexInParent = -1;
    std::vector<std::shared_ptr<TreeNode<T>>> m_children;

    alignas(T) std::array<std::byte, sizeof(T)> m_payloadStorage;
};

}
