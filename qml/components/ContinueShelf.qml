import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: shelf
    required property QtObject theme
    required property QtObject personal
    visible: row.count > 0
    Layout.fillWidth: true
    spacing: 12
    RowLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("Weiterschauen")
            color: shelf.theme.text
            font.pixelSize: 21
            font.weight: Font.DemiBold
            Layout.fillWidth: true
        }
        NavButton {
            theme: shelf.theme
            text: "‹"
            font.pixelSize: 28
            enabled: !row.atXBeginning
            Accessible.name: qsTr("Vorherige Titel")
            onClicked: row.page(-1)
        }
        NavButton {
            theme: shelf.theme
            text: "›"
            font.pixelSize: 28
            enabled: !row.atXEnd
            Accessible.name: qsTr("Weitere Titel")
            onClicked: row.page(1)
        }
    }
    ListView {
        id: row
        Layout.fillWidth: true
        Layout.preferredHeight: 218
        orientation: ListView.Horizontal
        spacing: 14
        clip: true
        model: personal.continueWatching
        boundsBehavior: Flickable.StopAtBounds
        onMovementStarted: scrollAnimation.stop()
        function page(direction) {
            cancelFlick();
            scrollAnimation.to = Math.max(0, Math.min(contentWidth - width, contentX + direction * Math.max(314, width - 314)));
            scrollAnimation.restart();
        }
        NumberAnimation {
            id: scrollAnimation
            target: row
            property: "contentX"
            duration: 220
            easing.type: Easing.OutCubic
        }
        ScrollBar.horizontal: ScrollBar {}
        delegate: Button {
            id: tile
            required property var entry
            width: 300
            height: 205
            hoverEnabled: true
            padding: 0
            Accessible.name: qsTr("Fortsetzen") + ": " + entry.title
            onClicked: shelf.personal.resume(entry.file_id)
            background: Item {}
            contentItem: Item {
                Rectangle {
                    id: thumbnail
                    width: parent.width
                    height: 169
                    radius: 5
                    clip: true
                    color: shelf.theme.raised
                    Image {
                        anchors.fill: parent
                        source: library.assetUrl(tile.entry.backdrop || tile.entry.poster || "")
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }
                    Rectangle {
                        anchors.fill: parent
                        gradient: Gradient {
                            GradientStop {
                                position: 0
                                color: "#10000000"
                            }
                            GradientStop {
                                position: 1
                                color: "#DD050B14"
                            }
                        }
                    }
                    Label {
                        anchors.centerIn: parent
                        text: "\u25B6\uFE0E"
                        font.pixelSize: tile.hovered || tile.visualFocus ? 36 : 28
                        color: "white"
                    }
                    Label {
                        anchors.left: parent.left
                        anchors.bottom: progress.top
                        anchors.margins: 12
                        text: (tile.entry.kind === "series" ? "S" + tile.entry.season + " E" + tile.entry.episode + " · " : "") + Math.floor(tile.entry.position / 60) + " / " + Math.floor(tile.entry.duration / 60) + " Min."
                        color: "#F3F5F8"
                        font.pixelSize: 12
                    }
                    Rectangle {
                        id: progress
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 3
                        color: "#667E8A9B"
                        Rectangle {
                            width: parent.width * Math.min(1, tile.entry.position / Math.max(1, tile.entry.duration))
                            height: parent.height
                            color: shelf.theme.accent
                        }
                    }
                    Rectangle {
                        anchors.fill: parent
                        radius: 5
                        color: "transparent"
                        border.width: tile.visualFocus ? 2 : 0
                        border.color: shelf.theme.accent
                    }
                }
                Label {
                    anchors.top: thumbnail.bottom
                    anchors.topMargin: 10
                    width: parent.width
                    text: tile.entry.title
                    textFormat: Text.PlainText
                    font.weight: Font.DemiBold
                    color: shelf.theme.text
                    elide: Text.ElideRight
                }
            }
        }
    }
}
