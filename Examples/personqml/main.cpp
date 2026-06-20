#include "example/PersonController.h"
#include "example/PersonRef.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QString>
#include <QtQml>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    qmlRegisterUncreatableType<example::PersonRef>(
        "Cute.Examples", 1, 0, "PersonRef",
        QStringLiteral("PersonRef instances are owned by the controller"));

    example::PersonController controller;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("controller"), &controller);
    engine.loadFromModule("CuteExamples", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
