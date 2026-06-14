#pragma once

#include "CuteModel/AbstractSourceModel.h"
#include "CuteModel/RefBase.h"
#include "CuteModel/RowFilterProxyModel.h"
#include "CuteModel/ValueModelAccessor.h"

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>
#include <utility>

namespace examples {

class PersonViewController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *viewModel READ viewModel CONSTANT)
    Q_PROPERTY(QItemSelectionModel *selectionModel READ selectionModel CONSTANT)
    Q_PROPERTY(QStringList selectedSummaries READ selectedSummaries NOTIFY selectedSummariesChanged)
    Q_PROPERTY(QString currentSummary READ currentSummary NOTIFY currentSummaryChanged)

public:
    using FilterSetter = std::function<void(const QString &)>;
    using IndexSummary = std::function<QString(const QModelIndex &)>;
    using RefFactory = std::function<std::unique_ptr<cute::RefBase>(const QModelIndex &)>;
    using RefSummary = std::function<QString(cute::RefBase *)>;

    PersonViewController(QAbstractItemModel *proxy,
                         FilterSetter setFilter,
                         IndexSummary indexSummary,
                         RefFactory makeRef,
                         RefSummary refSummary,
                         QObject *parent = nullptr);

    QAbstractItemModel *viewModel() const { return m_proxy; }
    QItemSelectionModel *selectionModel() const { return m_selection; }
    QStringList selectedSummaries() const;
    QString currentSummary() const;

    Q_INVOKABLE void setFilter(const QString &text);
    Q_INVOKABLE void selectRow(int row);
    Q_INVOKABLE void selectCell(int row, int column);
    Q_INVOKABLE void selectIndex(const QModelIndex &index);

signals:
    void selectedSummariesChanged();
    void currentSummaryChanged();

private:
    void handleSelectionChanged();
    void handleCurrentChanged(const QModelIndex &current);

    QAbstractItemModel *m_proxy;
    QItemSelectionModel *m_selection;
    FilterSetter m_setFilter;
    IndexSummary m_indexSummary;
    RefFactory m_makeRef;
    RefSummary m_refSummary;
    std::unique_ptr<cute::RefBase> m_currentRef;
};

template <typename T>
PersonViewController *makePersonController(cute::AbstractSourceModel<T> *source,
                                          std::function<QString(const T &)> summary,
                                          QObject *parent = nullptr)
{
    auto *proxy = new cute::RowFilterProxyModel<T>(source, parent);
    proxy->setRecursiveFilteringEnabled(true);
    auto *accessor = static_cast<cute::ValueModelAccessor<T> *>(proxy);

    PersonViewController::FilterSetter setFilter = [proxy, summary](const QString &text) {
        if (text.isEmpty()) {
            proxy->clearFilter();
            return;
        }
        proxy->setFilterPredicate([summary, text](const T &value, const QModelIndex &) {
            return summary(value).contains(text, Qt::CaseInsensitive);
        });
    };

    PersonViewController::IndexSummary indexSummary =
        [accessor, summary](const QModelIndex &index) {
            return summary(accessor->getStorageValue(index));
        };

    PersonViewController::RefFactory makeRef =
        [accessor](const QModelIndex &index) -> std::unique_ptr<cute::RefBase> {
        return accessor->template getRef<>(index);
    };

    PersonViewController::RefSummary refSummary = [summary](cute::RefBase *base) {
        auto *ref = static_cast<typename cute::ValueModelAccessor<T>::Ref *>(base);
        return summary(ref->getValue());
    };

    return new PersonViewController(proxy, std::move(setFilter), std::move(indexSummary),
                                    std::move(makeRef), std::move(refSummary), parent);
}

}
