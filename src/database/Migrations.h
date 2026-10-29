#pragma once
#include <QSqlDatabase>
#include <QStringList>
#include <QVector>
namespace veluno {
struct Migration { int version; QStringList statements; };
QVector<Migration> schemaMigrations();
// One transaction per schema version; newer schemas must never be downgraded.
void migrate(QSqlDatabase &db, const QVector<Migration> &migrations);
void execute(QSqlDatabase &db, const QString &sql);
}
