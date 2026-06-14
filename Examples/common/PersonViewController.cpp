#include "PersonViewController.h"

#include <QItemSelection>
#include <QModelIndexList>

#include <algorithm>
#include <vector>

namespace examples {

PersonViewController::PersonViewController(QAbstractItemModel *proxy,
                                          FilterSetter setFilter,
                                          IndexSummary indexSummary,
                                          RefFactory makeRef,
                                          RefSummary refSummary,
                                          QObject *parent)
    : QObject(parent)
    , m_proxy(proxy)
    , m_selection(new QItemSelectionModel(proxy, this))
    , m_setFilter(std::move(setFilter))
    , m_indexSummary(std::move(indexSummary))
    , m_makeRef(std::move(makeRef))
    , m_refSummary(std::move(refSummary))
{
    connect(m_selection, &QItemSelectionModel::selectionChanged, this,
            &PersonViewController::handleSelectionChanged);
    connect(m_selection, &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex &current, const QModelIndex &) {
                handleCurrentChanged(current);
            });
}

QStringList PersonViewController::selectedSummaries() const
{
    QStringList summaries;
    std::vector<QModelIndex> seen;
    const QModelIndexList indexes = m_selection->selectedIndexes();
    for (const QModelIndex &index : indexes) {
        const QModelIndex column0 = index.sibling(index.row(), 0);
        if (std::find(seen.cbegin(), seen.cend(), column0) != seen.cend())
            continue;
        seen.push_back(column0);
        summaries.append(m_indexSummary(column0));
    }
    return summaries;
}

QString PersonViewController::currentSummary() const
{
    if (!m_currentRef || !m_currentRef->index().isValid())
        return {};
    return m_refSummary(m_currentRef.get());
}

void PersonViewController::setFilter(const QString &text)
{
    m_setFilter(text);
}

void PersonViewController::selectRow(int row)
{
    selectCell(row, 0);
}

void PersonViewController::selectCell(int row, int column)
{
    selectIndex(m_proxy->index(row, column));
}

void PersonViewController::selectIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    m_selection->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect
                                            | QItemSelectionModel::Rows);
}

void PersonViewController::handleSelectionChanged()
{
    emit selectedSummariesChanged();
}

void PersonViewController::handleCurrentChanged(const QModelIndex &current)
{
    m_currentRef = m_makeRef(current);
    if (m_currentRef) {
        connect(m_currentRef.get(), &cute::RefBase::valueChanged, this,
                &PersonViewController::currentSummaryChanged);
    }
    emit currentSummaryChanged();
}

}
