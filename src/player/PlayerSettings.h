#pragma once
#include "MpvEngine.h"
#include "database/Database.h"
namespace veluno {
class PlayerSettings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap options READ options NOTIFY changed)
    Q_PROPERTY(QVariantMap shortcuts READ shortcuts NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
  public:
    PlayerSettings(Database &db, MpvEngine &player, QObject *parent = nullptr);
    QVariantMap options() const {
        return m_options;
    }
    QVariantMap shortcuts() const {
        return m_shortcuts;
    }
    QString error() const {
        return m_error;
    }
    Q_INVOKABLE void setOption(const QString &key, const QVariant &value);
    Q_INVOKABLE bool setShortcut(const QString &action, const QString &sequence);
    Q_INVOKABLE void resetShortcuts();
  signals:
    void changed();

  private:
    void persist();
    Database &m_db;
    MpvEngine &m_player;
    QVariantMap m_options, m_shortcuts;
    QString m_error;
};
} // namespace veluno
