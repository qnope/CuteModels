#include "example/PersonController.h"
#include "example/PersonRef.h"

#include <QApplication>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPointer>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

using example::PersonController;
using example::PersonRef;

namespace {

class DetailPanel : public QWidget
{
public:
    explicit DetailPanel(PersonController *controller, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_controller(controller)
    {
        auto *layout = new QFormLayout(this);
        m_name = new QLabel(this);
        m_age = new QLabel(this);
        m_email = new QLabel(this);
        layout->addRow(QStringLiteral("Name:"), m_name);
        layout->addRow(QStringLiteral("Age:"), m_age);
        layout->addRow(QStringLiteral("Email:"), m_email);

        connect(m_controller, &PersonController::currentRefChanged,
                this, &DetailPanel::onCurrentRefChanged);
        onCurrentRefChanged();
    }

    void onCurrentRefChanged()
    {
        if (m_ref)
            m_ref->disconnect(this);
        m_ref = m_controller->currentRef();
        if (m_ref)
            connect(m_ref, &PersonRef::changed, this, &DetailPanel::refresh);
        refresh();
    }

    void refresh()
    {
        if (!m_ref || !m_ref->valid()) {
            m_name->clear();
            m_age->clear();
            m_email->clear();
            return;
        }
        m_name->setText(m_ref->name());
        m_age->setText(QString::number(m_ref->age()));
        m_email->setText(m_ref->email());
    }

private:
    PersonController *m_controller;
    QPointer<PersonRef> m_ref;
    QLabel *m_name;
    QLabel *m_age;
    QLabel *m_email;
};

}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    PersonController controller;

    QWidget window;
    window.setWindowTitle(QStringLiteral("BasicListModel — Widgets"));

    auto *rootLayout = new QVBoxLayout(&window);
    auto *topLayout = new QHBoxLayout;

    auto *leftLayout = new QVBoxLayout;
    auto *filterEdit = new QLineEdit;
    filterEdit->setPlaceholderText(QStringLiteral("Filter by name..."));
    auto *listView = new QListView;
    listView->setModel(controller.listModel());
    listView->setModelColumn(0);
    listView->setSelectionModel(controller.selectionModel());
    listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    leftLayout->addWidget(filterEdit);
    leftLayout->addWidget(listView);

    auto *selectionView = new QListView;
    selectionView->setModel(controller.selectionListModel());
    selectionView->setModelColumn(0);
    selectionView->setSelectionMode(QAbstractItemView::NoSelection);

    topLayout->addLayout(leftLayout, 1);
    topLayout->addWidget(selectionView, 1);

    auto *detail = new DetailPanel(&controller);

    rootLayout->addLayout(topLayout, 1);
    rootLayout->addWidget(detail);

    QObject::connect(filterEdit, &QLineEdit::textChanged,
                     &controller, &PersonController::setFilterText);

    window.resize(640, 440);
    window.show();

    return app.exec();
}
