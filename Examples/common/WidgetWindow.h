#pragma once

#include "PersonViewController.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

namespace examples {

template <typename ViewT>
QWidget *makeWidgetWindow(PersonViewController *controller, ViewT *view, const QString &title)
{
    view->setModel(controller->viewModel());
    view->setSelectionModel(controller->selectionModel());
    view->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *filterEdit = new QLineEdit;
    filterEdit->setPlaceholderText(QStringLiteral("Filter…"));
    QObject::connect(filterEdit, &QLineEdit::textChanged, controller,
                     &PersonViewController::setFilter);

    auto *leftColumn = new QVBoxLayout;
    leftColumn->addWidget(new QLabel(QStringLiteral("Filter")));
    leftColumn->addWidget(filterEdit);
    leftColumn->addStretch();

    auto *selectedList = new QListWidget;
    QObject::connect(controller, &PersonViewController::selectedSummariesChanged, selectedList,
                     [selectedList, controller] {
                         selectedList->clear();
                         selectedList->addItems(controller->selectedSummaries());
                     });

    auto *rightColumn = new QVBoxLayout;
    rightColumn->addWidget(new QLabel(QStringLiteral("Selected")));
    rightColumn->addWidget(selectedList);

    auto *topRow = new QHBoxLayout;
    topRow->addLayout(leftColumn, 0);
    topRow->addWidget(view, 1);
    topRow->addLayout(rightColumn, 0);

    auto *refLabel = new QLabel(QStringLiteral("Ref: —"));
    refLabel->setFrameShape(QFrame::StyledPanel);
    refLabel->setMargin(8);
    QObject::connect(controller, &PersonViewController::currentSummaryChanged, refLabel,
                     [refLabel, controller] {
                         const QString summary = controller->currentSummary();
                         refLabel->setText(QStringLiteral("Ref: ")
                                           + (summary.isEmpty() ? QStringLiteral("—") : summary));
                     });

    auto *window = new QWidget;
    window->setWindowTitle(title);
    window->resize(820, 480);
    auto *layout = new QVBoxLayout(window);
    layout->addLayout(topRow, 1);
    layout->addWidget(refLabel, 0);
    return window;
}

}
