#include "CuteModel/BasicListModel.h"
#include "CuteModel/GetAsRefObject.h"
#include "CuteModel/Ref.h"
#include "CuteModel/RefBase.h"

#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QVariant>

using cute::BasicListModel;
using cute::getAsRefObject;
using cute::Ref;

namespace {

class StringListModel : public BasicListModel<QString>
{
public:
    using BasicListModel<QString>::BasicListModel;
    using BasicListModel<QString>::data;

    QVariant data(const QString &value, int column, int role) const override
    {
        if (column != 0)
            return {};
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return value;
        return {};
    }
};

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    StringListModel model;
    model.push_back(QStringLiteral("alpha"));
    model.push_back(QStringLiteral("beta"));
    model.push_back(QStringLiteral("gamma"));

    Ref<QString> *ref = getAsRefObject<Ref<QString>>(model.index(1, 0));

    QObject::connect(ref, &cute::RefBase::valueChanged, [ref] {
        qInfo() << "value changed to:" << ref->getValue();
    });

    qInfo() << "initial value:" << ref->getValue();

    ref->setValue(QStringLiteral("BETA"));

    qInfo() << "final value:" << ref->getValue();

    return 0;
}
