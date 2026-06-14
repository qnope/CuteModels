#include "PersonModels.h"
#include "PersonViewController.h"
#include "WidgetWindow.h"

#include <QApplication>
#include <QListView>

using namespace examples;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    PersonListModel *model = makeSamplePersonList(&app);
    PersonViewController *controller = makePersonController<Person>(
        model, [](const Person &person) { return personSummary(person); }, &app);

    QWidget *window = makeWidgetWindow(controller, new QListView,
                                       QStringLiteral("CuteModel — BasicListModel (Widgets)"));
    window->show();

    return app.exec();
}
