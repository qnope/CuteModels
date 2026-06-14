#include "PersonModels.h"
#include "PersonViewController.h"

#include <QGuiApplication>
#include <QMetaType>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

using namespace examples;

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    qRegisterMetaType<Person>();

    PersonTableModel *model = makeSamplePersonTable(&app);
    PersonViewController *controller = makePersonController<Person>(
        model, [](const Person &person) { return personSummary(person); }, &app);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("controller"), controller);
    engine.load(QUrl(QStringLiteral("qrc:/CuteTableExample/qml_table/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
