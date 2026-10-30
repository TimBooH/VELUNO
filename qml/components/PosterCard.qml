import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: card
    required property QtObject theme
    required property var entry
    signal detailsRequested
    signal playRequested
    objectName: "poster-" + entry.id
    implicitWidth: 170
    implicitHeight: implicitWidth * 1.68
    hoverEnabled: true
    scale: hovered && !down ? 1.02 : 1
    padding: 0
    Accessible.name: entry.title + ", Details öffnen"
    onClicked: detailsRequested()
    background: Rectangle {
        radius: 9
        color: theme.surface
        border.color: card.visualFocus ? theme.accent : theme.border
        border.width: card.visualFocus ? 2 : 1
    }
    contentItem: ColumnLayout {
        spacing: 8
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 8
            color: theme.raised
            radius: 6
            Image {
                anchors.fill: parent
                source: library.assetUrl(entry.poster || "")
                fillMode: Image.PreserveAspectCrop
                visible: !!entry.poster
                asynchronous: true
                cache: true
            }
            Label {
                visible: !entry.poster
                anchors.centerIn: parent
                text: entry.title.slice(0, 1).toUpperCase()
                font.pixelSize: 52
                color: theme.accent
            }
            Label {
                visible: !entry.poster
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 12
                text: qsTr("Kein Cover")
                font.pixelSize: 12
                color: theme.muted
            }
            Button {
                anchors.centerIn: parent
                visible: card.hovered || card.visualFocus
                text: entry.kind === "series" ? "Episoden" : "Abspielen"
                onClicked: card.playRequested()
            }
        }
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            text: entry.title
            textFormat: Text.PlainText
            elide: Text.ElideRight
            font.weight: Font.DemiBold
            color: theme.text
        }
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: 12
            text: (entry.year > 0 ? entry.year + " · " : "") + (entry.available ? entry.file_count + (entry.kind === "series" ? " Dateien" : " Version(en)") : "Nicht verfügbar")
            font.pixelSize: 12
            color: theme.muted
            elide: Text.ElideRight
        }
    }
}
