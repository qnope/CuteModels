#pragma once

#include "Person.h"

#include "CuteModel/BasicListModel.h"
#include "CuteModel/BasicTableModel.h"
#include "CuteModel/TreeModel.h"
#include "CuteModel/TreeNode.h"

#include <QModelIndex>
#include <QVariant>

#include <memory>

namespace examples {

using PersonNode = cute::TreeNode<Person>;
using PersonNodePtr = std::shared_ptr<PersonNode>;

class PersonListModel : public cute::BasicListModel<Person>
{
public:
    using cute::BasicListModel<Person>::BasicListModel;
    using cute::BasicListModel<Person>::data;

    QVariant data(const Person &value, const QModelIndex &index, int role) const override;
};

class PersonTableModel : public cute::BasicTableModel<Person>
{
public:
    using cute::BasicTableModel<Person>::BasicTableModel;
    using cute::BasicTableModel<Person>::data;

    QVariant data(const Person &value, const QModelIndex &index, int role) const override;
};

class PersonTreeModel : public cute::TreeModel<PersonNodePtr>
{
public:
    using cute::TreeModel<PersonNodePtr>::TreeModel;
    using cute::TreeModel<PersonNodePtr>::data;

    QVariant data(const PersonNodePtr &value, const QModelIndex &index, int role) const override;
};

PersonListModel *makeSamplePersonList(QObject *parent = nullptr);
PersonTableModel *makeSamplePersonTable(QObject *parent = nullptr);
PersonTreeModel *makeSamplePersonTree(QObject *parent = nullptr);

}
