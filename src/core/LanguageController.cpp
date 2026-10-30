#include "LanguageController.h"
#include <QCoreApplication>
namespace veluno {
LanguageController::LanguageController(Database &db, QObject *parent)
    : QTranslator(parent), m_db(db) {
    m_english = {
#include "../../resources/translations/English.inc"
    };
    m_language = db.setting("language", "de");
    if (m_language != "en")
        m_language = "de";
    QCoreApplication::installTranslator(this);
}
LanguageController::~LanguageController() {
    QCoreApplication::removeTranslator(this);
}
QString LanguageController::translate(const char *, const char *source, const char *, int) const {
    return m_language == "en" ? m_english.value(QString::fromUtf8(source)) : QString{};
}
void LanguageController::setLanguage(const QString &language) {
    if (language != "de" && language != "en")
        return;
    m_db.setSetting("language", language);
    m_language = language;
    emit languageChanged();
}
} // namespace veluno
