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

    PersonTreeModel *model = makeSamplePersonTree(&app);
    PersonViewController *controller = makePersonController<PersonNodePtr>(
        model,
        [](const PersonNodePtr &node) { return personSummary(node->payload()); },
        &app);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("controller"), controller);
    engine.load(QUrl(QStringLiteral("qrc:/CuteTreeExample/qml_tree/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
