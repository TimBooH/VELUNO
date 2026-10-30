import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: shelf
    required property QtObject theme
    required property QtObject personal
    visible: row.count > 0
    Layout.fillWidth: true
    Label {
        text: qsTr("Weiterschauen")
        color: shelf.theme.text
        font.pixelSize: 22
    }
    ListView {
        id: row
        Layout.fillWidth: true
        Layout.preferredHeight: 195
        orientation: ListView.Horizontal
        spacing: 16
        clip: true
        model: personal.continueWatching
        ScrollBar.horizontal: ScrollBar {}
        delegate: Rectangle {
            required property var entry
            width: 280
            height: 175
            radius: 8
            color: shelf.theme.surface
            Image {
                anchors.fill: parent
                source: library.assetUrl(entry.backdrop || entry.poster || "")
                fillMode: Image.PreserveAspectCrop
                opacity: 0.25
                asynchronous: true
            }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                Label {
                    text: qsTr("▶")
                    color: shelf.theme.accent
                    font.pixelSize: 38
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: entry.title
                    textFormat: Text.PlainText
                    color: shelf.theme.text
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Label {
                    text: (entry.kind === "series" ? "S" + entry.season + " E" + entry.episode + " · " : "") + Math.floor(entry.position / 60) + " / " + Math.floor(entry.duration / 60) + " Min."
                    color: shelf.theme.muted
                }
                ProgressBar {
                    from: 0
                    to: Math.max(1, entry.duration)
                    value: entry.position
                    Layout.fillWidth: true
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: shelf.personal.resume(entry.file_id)
                cursorShape: Qt.PointingHandCursor
            }
        }
    }
}
