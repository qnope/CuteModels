#pragma once

#include "example/Person.h"
#include "example/PersonListModel.h"
#include "example/PersonRef.h"

#include "CuteModel/RowFilterProxyModel.h"
#include "CuteModel/SelectionModel.h"

#include <QAbstractItemModel>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariantList>

#include <memory>

namespace example {

class PersonController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(QAbstractItemModel *listModel READ listModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *selectionListModel READ selectionListModel CONSTANT)
    Q_PROPERTY(QItemSelectionModel *selectionModel READ selectionModel CONSTANT)
    Q_PROPERTY(example::PersonRef *currentRef READ currentRef NOTIFY currentRefChanged)
    Q_PROPERTY(QVariantList selectedRows READ selectedRows NOTIFY selectedRowsChanged)
    Q_PROPERTY(int currentRow READ currentRow NOTIFY selectedRowsChanged)

public:
    explicit PersonController(QObject *parent = nullptr);

    QString filterText() const;
    void setFilterText(const QString &text);

    QAbstractItemModel *listModel();
    QAbstractItemModel *selectionListModel();
    QItemSelectionModel *selectionModel();
    PersonRef *currentRef() const;
    QVariantList selectedRows() const;
    int currentRow() const;

    Q_INVOKABLE void selectRow(int proxyRow);

signals:
    void filterTextChanged();
    void currentRefChanged();
    void selectedRowsChanged();

private:
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
    void updateCurrentRef();

    PersonListModel m_source;
    cute::RowFilterProxyModel<Person> m_proxy;
    cute::SelectionModel<Person> m_selection;
    PersonListModel m_selected;
    std::unique_ptr<PersonRef> m_currentRef;
    QString m_filterText;
};

}
