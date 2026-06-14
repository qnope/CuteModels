#include "PersonModels.h"
#include "PersonViewController.h"
#include "WidgetWindow.h"

#include <QApplication>
#include <QTableView>

using namespace examples;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    PersonTableModel *model = makeSamplePersonTable(&app);
    PersonViewController *controller = makePersonController<Person>(
        model, [](const Person &person) { return personSummary(person); }, &app);

    QWidget *window = makeWidgetWindow(controller, new QTableView,
                                       QStringLiteral("CuteModel — BasicTableModel (Widgets)"));
    window->show();

    return app.exec();
}
