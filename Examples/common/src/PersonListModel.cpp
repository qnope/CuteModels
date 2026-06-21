#include "example/PersonListModel.h"

namespace example {

std::vector<Person> samplePeople()
{
    return {
        Person(QStringLiteral("Alice"), 30, QStringLiteral("alice@example.com")),
        Person(QStringLiteral("Bob"), 24, QStringLiteral("bob@example.com")),
        Person(QStringLiteral("Carol"), 41, QStringLiteral("carol@example.com")),
        Person(QStringLiteral("Dave"), 37, QStringLiteral("dave@example.com")),
        Person(QStringLiteral("Erin"), 29, QStringLiteral("erin@example.com")),
        Person(QStringLiteral("Frank"), 52, QStringLiteral("frank@example.com")),
        Person(QStringLiteral("Grace"), 33, QStringLiteral("grace@example.com")),
    };
}

}
