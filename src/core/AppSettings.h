#pragma once
#include <QObject>
#include "database/Database.h"
namespace veluno {
class AppSettings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
public:
    explicit AppSettings(Database &database, QObject *parent = nullptr);
    QString theme() const { return m_theme; }
    QString error() const { return m_error; }
    void setTheme(const QString &theme);
signals:
    void themeChanged();
    void errorChanged();
private:
    Database &m_database;
    QString m_theme;
    QString m_error;
};
}
