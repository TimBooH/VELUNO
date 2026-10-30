#include "PlayerSettings.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <cmath>
namespace veluno {
static QVariantMap defaults() {
    return {{"pause", "Space"},
            {"back", "Left"},
            {"forward", "Right"},
            {"volume_up", "Up"},
            {"volume_down", "Down"},
            {"fullscreen", "F"},
            {"exit_fullscreen", "Esc"},
            {"mute", "M"},
            {"subtitles", "S"},
            {"audio", "A"},
            {"next", "N"}};
}
PlayerSettings::PlayerSettings(Database &db, MpvEngine &player, QObject *parent)
    : QObject(parent), m_db(db), m_player(player) {
    m_options = {{"decoder", "auto"},   {"audio_device", "auto"}, {"sub_size", 36},
                 {"sub_position", 100}, {"sub_delay", 0.0},       {"sub_background", false}};
    const auto saved =
        QJsonDocument::fromJson(db.setting("player_options").toUtf8()).object().toVariantMap();
    for (auto it = saved.begin(); it != saved.end(); ++it)
        if (m_options.contains(it.key()))
            m_options[it.key()] = it.value();
    m_shortcuts = defaults();
    const auto keys =
        QJsonDocument::fromJson(db.setting("shortcuts").toUtf8()).object().toVariantMap();
    if (!keys.isEmpty()) {
        auto canonical = defaults();
        bool valid = true;
        QStringList used{"Ctrl+O"};
        for (auto it = canonical.begin(); it != canonical.end(); ++it) {
            const auto value = keys.value(it.key(), it.value()).toString();
            const auto sequence = QKeySequence::fromString(value, QKeySequence::PortableText);
            const auto text = sequence.toString(QKeySequence::PortableText);
            if (sequence.isEmpty() || text.isEmpty() || sequence.count() != 1 ||
                used.contains(text)) {
                valid = false;
                break;
            }
            used.append(text);
            it.value() = text;
        }
        if (valid)
            m_shortcuts = canonical;
    }
    m_player.configure(m_options);
}
void PlayerSettings::persist() {
    m_db.setSetting(
        "player_options",
        QString::fromUtf8(QJsonDocument::fromVariant(m_options).toJson(QJsonDocument::Compact)));
    m_db.setSetting(
        "shortcuts",
        QString::fromUtf8(QJsonDocument::fromVariant(m_shortcuts).toJson(QJsonDocument::Compact)));
}
void PlayerSettings::setOption(const QString &key, const QVariant &value) {
    m_error.clear();
    if (!m_options.contains(key))
        return;
    bool valid = true;
    if (key == "decoder")
        valid = QStringList{"auto", "hardware", "software"}.contains(value.toString());
    if (key == "audio_device")
        valid = !value.toString().isEmpty() && !value.toString().contains(QChar::Null);
    if (key == "sub_size")
        valid =
            std::isfinite(value.toDouble()) && value.toDouble() >= 12 && value.toDouble() <= 100;
    if (key == "sub_position")
        valid = std::isfinite(value.toDouble()) && value.toDouble() >= 0 && value.toDouble() <= 100;
    if (key == "sub_delay")
        valid = std::isfinite(value.toDouble()) && std::abs(value.toDouble()) <= 120;
    if (!valid) {
        m_error = "Ungültige Player-Einstellung.";
        emit changed();
        return;
    }
    const auto before = m_options;
    m_options[key] = value;
    try {
        persist();
        m_player.configure(m_options);
    } catch (const std::exception &e) {
        m_options = before;
        m_error = QString::fromUtf8(e.what());
    }
    emit changed();
}
bool PlayerSettings::setShortcut(const QString &action, const QString &value) {
    m_error.clear();
    const auto sequence = QKeySequence::fromString(value, QKeySequence::PortableText);
    const auto text = sequence.toString(QKeySequence::PortableText);
    if (!m_shortcuts.contains(action) || sequence.isEmpty() || text.isEmpty() ||
        sequence.count() != 1 || text == "Ctrl+O")
        m_error = "Bitte eine gültige einzelne Taste oder Tastenkombination wählen. Strg+O ist "
                  "reserviert.";
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it)
        if (it.key() != action && it.value().toString() == text)
            m_error = "Diese Tastenkombination wird bereits verwendet.";
    if (!m_error.isEmpty()) {
        emit changed();
        return false;
    }
    const auto before = m_shortcuts;
    m_shortcuts[action] = text;
    try {
        persist();
    } catch (const std::exception &e) {
        m_shortcuts = before;
        m_error = QString::fromUtf8(e.what());
        emit changed();
        return false;
    }
    emit changed();
    return true;
}
void PlayerSettings::resetShortcuts() {
    m_shortcuts = defaults();
    try {
        persist();
        m_error.clear();
    } catch (const std::exception &e) {
        m_error = QString::fromUtf8(e.what());
    }
    emit changed();
}
} // namespace veluno
