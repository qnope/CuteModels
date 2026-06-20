#include "example/PersonController.h"

#include <Qt>

namespace example {

PersonController::PersonController(QObject *parent)
    : QObject(parent)
    , m_source({QStringLiteral("Name"), QStringLiteral("Age")})
    , m_proxy(&m_source)
    , m_selection(&m_proxy)
    , m_selected({QStringLiteral("Name"), QStringLiteral("Age")})
{
    m_source.reset(samplePeople());

    connect(&m_selection, &QItemSelectionModel::selectionChanged,
            this, &PersonController::onSelectionChanged);
    connect(&m_selection, &QItemSelectionModel::currentChanged,
            this, &PersonController::onCurrentChanged);

    updateCurrentRef();
}

QString PersonController::filterText() const
{
    return m_filterText;
}

void PersonController::setFilterText(const QString &text)
{
    if (m_filterText == text)
        return;

    m_filterText = text;
    if (text.isEmpty()) {
        m_proxy.clearFilter();
    } else {
        m_proxy.setFilterPredicate([text](const Person &person, const QModelIndex &) {
            return person.m_name.contains(text, Qt::CaseInsensitive);
        });
    }
    emit filterTextChanged();
    emit selectedRowsChanged();
}

QAbstractItemModel *PersonController::listModel()
{
    return &m_proxy;
}

QAbstractItemModel *PersonController::selectionListModel()
{
    return &m_selected;
}

QItemSelectionModel *PersonController::selectionModel()
{
    return &m_selection;
}

PersonRef *PersonController::currentRef() const
{
    return m_currentRef.get();
}

QVariantList PersonController::selectedRows() const
{
    QVariantList rows;
    const QModelIndexList selected = m_selection.selectedIndexes();
    for (const QModelIndex &index : selected) {
        if (index.column() != 0)
            continue;
        if (!rows.contains(index.row()))
            rows.append(index.row());
    }
    return rows;
}

int PersonController::currentRow() const
{
    return m_selection.currentIndex().row();
}

void PersonController::selectRow(int proxyRow)
{
    const QModelIndex index = m_proxy.index(proxyRow, 0);
    if (!index.isValid())
        return;

    m_selection.setCurrentIndex(index,
                                QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
}

void PersonController::onSelectionChanged(const QItemSelection &, const QItemSelection &)
{
    m_selected.reset(m_selection.selectedValues());
    emit selectedRowsChanged();
}

void PersonController::onCurrentChanged(const QModelIndex &, const QModelIndex &)
{
    updateCurrentRef();
    emit selectedRowsChanged();
}

void PersonController::updateCurrentRef()
{
    m_currentRef = std::make_unique<PersonRef>(m_selection.currentRef());
    emit currentRefChanged();
}

}
