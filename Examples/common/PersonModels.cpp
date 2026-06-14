#include "PersonModels.h"

#include <QStringList>

#include <vector>

namespace examples {

QVariant PersonListModel::data(const Person &value, const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return {};
    switch (index.column()) {
    case 0:
        return value.name;
    case 1:
        return value.age;
    case 2:
        return value.city;
    default:
        return {};
    }
}

QVariant PersonTableModel::data(const Person &value, const QModelIndex &, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return personSummary(value);
    return {};
}

QVariant PersonTreeModel::data(const PersonNodePtr &value, const QModelIndex &index, int role) const
{
    if (index.column() != 0)
        return {};
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return value->payload().name;
    return {};
}

PersonListModel *makeSamplePersonList(QObject *parent)
{
    auto *model = new PersonListModel(QStringList{QStringLiteral("Name"),
                                                  QStringLiteral("Age"),
                                                  QStringLiteral("City")},
                                      parent);
    model->reset({Person{QStringLiteral("Alice"), 30, QStringLiteral("Paris")},
                  Person{QStringLiteral("Bob"), 24, QStringLiteral("Lyon")},
                  Person{QStringLiteral("Carol"), 41, QStringLiteral("Marseille")},
                  Person{QStringLiteral("Dave"), 35, QStringLiteral("Paris")},
                  Person{QStringLiteral("Erin"), 28, QStringLiteral("Nantes")},
                  Person{QStringLiteral("Frank"), 52, QStringLiteral("Lille")}});
    return model;
}

PersonTableModel *makeSamplePersonTable(QObject *parent)
{
    auto *model = new PersonTableModel(parent);
    model->reset({{Person{QStringLiteral("Alice"), 30, QStringLiteral("Paris")},
                   Person{QStringLiteral("Bob"), 24, QStringLiteral("Lyon")},
                   Person{QStringLiteral("Carol"), 41, QStringLiteral("Marseille")}},
                  {Person{QStringLiteral("Dave"), 35, QStringLiteral("Paris")},
                   Person{QStringLiteral("Erin"), 28, QStringLiteral("Nantes")},
                   Person{QStringLiteral("Frank"), 52, QStringLiteral("Lille")}},
                  {Person{QStringLiteral("Gina"), 19, QStringLiteral("Nice")},
                   Person{QStringLiteral("Hugo"), 46, QStringLiteral("Rennes")},
                   Person{QStringLiteral("Ivy"), 33, QStringLiteral("Toulouse")}}});
    return model;
}

namespace {

PersonNodePtr makeNode(Person person)
{
    return std::make_shared<PersonNode>(std::move(person));
}

}

PersonTreeModel *makeSamplePersonTree(QObject *parent)
{
    auto *model = new PersonTreeModel(QStringList{QStringLiteral("Name")}, parent);

    auto paris = makeNode(Person{QStringLiteral("Paris"), 0, QStringLiteral("Paris")});
    paris->push_back_child(makeNode(Person{QStringLiteral("Alice"), 30, QStringLiteral("Paris")}));
    paris->push_back_child(makeNode(Person{QStringLiteral("Dave"), 35, QStringLiteral("Paris")}));

    auto lyon = makeNode(Person{QStringLiteral("Lyon"), 0, QStringLiteral("Lyon")});
    lyon->push_back_child(makeNode(Person{QStringLiteral("Bob"), 24, QStringLiteral("Lyon")}));

    auto south = makeNode(Person{QStringLiteral("South"), 0, QStringLiteral("South")});
    south->push_back_child(makeNode(Person{QStringLiteral("Carol"), 41, QStringLiteral("Marseille")}));
    south->push_back_child(makeNode(Person{QStringLiteral("Gina"), 19, QStringLiteral("Nice")}));
    south->push_back_child(makeNode(Person{QStringLiteral("Ivy"), 33, QStringLiteral("Toulouse")}));

    std::vector<PersonNodePtr> roots;
    roots.push_back(std::move(paris));
    roots.push_back(std::move(lyon));
    roots.push_back(std::move(south));
    model->root()->append_range(std::move(roots));

    return model;
}

}
