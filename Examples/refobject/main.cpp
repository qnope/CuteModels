#include "CuteModel/GetAsRefObject.h"
#include "CuteModel/Ref.h"
#include "CuteModel/ValueRole.h"

#include <QAbstractListModel>
#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QVariant>

using cute::getAsRefObject;
using cute::Ref;
using cute::ValueRole;

namespace {

// Minimal Core-only list model exposing its values through ValueRole.
// Placeholder until BasicListModel<T> exists.
class ValueListModel : public QAbstractListModel
{
public:
    explicit ValueListModel(QStringList values, QObject *parent = nullptr)
        : QAbstractListModel(parent)
        , m_values(std::move(values))
    {
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : m_values.size();
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || index.row() >= m_values.size())
            return {};
        if (role == ValueRole)
            return m_values.at(index.row());
        return {};
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (!index.isValid() || index.row() >= m_values.size())
            return false;
        if (role == ValueRole) {
            m_values[index.row()] = value.toString();
            emit dataChanged(index, index, {role});
            return true;
        }
        return false;
    }

private:
    QStringList m_values;
};

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    ValueListModel model(QStringList{QStringLiteral("alpha"),
                                     QStringLiteral("beta"),
                                     QStringLiteral("gamma")});

    Ref<QString> *ref = getAsRefObject<Ref<QString>>(model.index(1, 0));

    QObject::connect(ref, &cute::RefBase::valueChanged, [ref] {
        qInfo() << "value changed to:" << ref->getValue();
    });

    qInfo() << "initial value:" << ref->getValue();

    // Mutate the followed item and observe the notification + new value.
    model.setData(model.index(1, 0), QStringLiteral("BETA"), ValueRole);

    qInfo() << "final value:" << ref->getValue();

    return 0;
}
