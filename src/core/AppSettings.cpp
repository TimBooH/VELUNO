#include "AppSettings.h"
#include <exception>
namespace veluno {
AppSettings::AppSettings(Database &database, QObject *parent) : QObject(parent), m_database(database), m_theme(database.setting("theme", "midnight")) {
    if (m_theme != "light" && m_theme != "midnight") m_theme = "midnight";
}
void AppSettings::setTheme(const QString &theme) {
    if ((theme != "light" && theme != "midnight") || theme == m_theme) return;
    try {
        m_database.setSetting("theme", theme);
        m_theme = theme;
        m_error.clear(); emit errorChanged(); emit themeChanged();
    } catch (const std::exception &e) { m_error = QString::fromUtf8(e.what()); emit errorChanged(); }
}
}
