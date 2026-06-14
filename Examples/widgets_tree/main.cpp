#include "PersonModels.h"
#include "PersonViewController.h"
#include "WidgetWindow.h"

#include <QApplication>
#include <QTreeView>

using namespace examples;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    PersonTreeModel *model = makeSamplePersonTree(&app);
    PersonViewController *controller = makePersonController<PersonNodePtr>(
        model,
        [](const PersonNodePtr &node) { return personSummary(node->payload()); },
        &app);

    QWidget *window = makeWidgetWindow(controller, new QTreeView,
                                       QStringLiteral("CuteModel — TreeModel (Widgets)"));
    window->show();

    return app.exec();
}
