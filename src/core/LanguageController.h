#pragma once
#include "database/Database.h"
#include <QHash>
#include <QTranslator>
namespace veluno {
class LanguageController final : public QTranslator {
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
  public:
    explicit LanguageController(Database &database, QObject *parent = nullptr);
    ~LanguageController() override;
    QString language() const {
        return m_language;
    }
    void setLanguage(const QString &language);
    QString translate(const char *context, const char *source, const char *disambiguation = nullptr,
                      int n = -1) const override;
    bool isEmpty() const override {
        return false;
    }
  signals:
    void languageChanged();

  private:
    Database &m_db;
    QString m_language;
    QHash<QString, QString> m_english;
};
} // namespace veluno
