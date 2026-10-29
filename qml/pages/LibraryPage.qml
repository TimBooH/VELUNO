import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    required property QtObject theme
    property string pageTitle: "Startseite"
    property string description: "Deine Filme und Serien. An einem Ort."
    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        Label { text: pageTitle; font.pixelSize: 30; font.weight: Font.DemiBold; color: theme.text }
        Label { text: description; color: theme.muted; font.pixelSize: 15; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        Item { Layout.fillHeight: true }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 190
            color: theme.surface; radius: 12
            border.color: theme.border
            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 48, 560)
                spacing: 12
                Label {
                    text: pageTitle === "Startseite" ? "Deine Mediathek beginnt hier" : "Noch keine Einträge"
                    color: theme.text; font.pixelSize: 22; font.weight: Font.Medium
                    Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                }
                Label {
                    text: "Die Bibliothek ist noch leer."
                    color: theme.muted; font.pixelSize: 15
                    Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter
                }
            }
        }
        Item { Layout.fillHeight: true }
    }
}
