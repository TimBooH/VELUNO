import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: shelf
    objectName: "continueShelf"
    required property QtObject theme
    required property QtObject personal
    readonly property int cardWidth: Math.max(212, Math.min(292, Math.round(width / 5)))
    visible: row.count > 0
    Layout.fillWidth: true
    spacing: 2
    RowLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("Weiterschauen")
            color: shelf.theme.text
            font.pixelSize: 18
            font.weight: Font.DemiBold
            Layout.fillWidth: true
        }
        NavButton {
            theme: shelf.theme
            text: "‹"
            font.pixelSize: 22
            padding: 2
            implicitWidth: 30
            implicitHeight: 28
            enabled: !row.atXBeginning
            Accessible.name: qsTr("Vorherige Titel")
            onClicked: row.page(-1)
        }
        NavButton {
            theme: shelf.theme
            text: "›"
            font.pixelSize: 22
            padding: 2
            implicitWidth: 30
            implicitHeight: 28
            enabled: !row.atXEnd
            Accessible.name: qsTr("Weitere Titel")
            onClicked: row.page(1)
        }
    }
    ListView {
        id: row
        Layout.fillWidth: true
        Layout.preferredHeight: shelf.cardWidth * 9 / 16 + 10
        orientation: ListView.Horizontal
        spacing: 8
        clip: true
        model: personal.continueWatching
        Keys.onPressed: function (event) {
            if (event.key === Qt.Key_Home) {
                scrollAnimation.stop();
                positionViewAtBeginning();
                event.accepted = true;
            } else if (event.key === Qt.Key_End) {
                scrollAnimation.stop();
                positionViewAtEnd();
                event.accepted = true;
            }
        }
        boundsBehavior: Flickable.StopAtBounds
        onMovementStarted: scrollAnimation.stop()
        function page(direction) {
            cancelFlick();
            scrollAnimation.to = Math.max(0, Math.min(contentWidth - width, contentX + direction * Math.max(shelf.cardWidth + 8, width - shelf.cardWidth - 8)));
            scrollAnimation.restart();
        }
        NumberAnimation {
            id: scrollAnimation
            target: row
            property: "contentX"
            duration: 220
            easing.type: Easing.OutCubic
        }
        ScrollBar.horizontal: ScrollBar {
        }
        delegate: Button {
            id: tile
            required property var entry
            width: shelf.cardWidth
            height: width * 9 / 16
            hoverEnabled: true
            padding: 0
            Accessible.name: qsTr("Fortsetzen") + ": " + entry.title
            onClicked: shelf.personal.resume(entry.file_id)
            background: Item {
            }
            contentItem: Item {
                Rectangle {
                    id: thumbnail
                    width: parent.width
                    height: parent.height
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
                    Canvas {
                        anchors.centerIn: parent
                        width: 36
                        height: 36
                        visible: tile.hovered || tile.visualFocus
                        onPaint: {
                            let ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);
                            ctx.fillStyle = "#FFFFFF";
                            ctx.beginPath();
                            ctx.moveTo(10, 6);
                            ctx.lineTo(29, 18);
                            ctx.lineTo(10, 30);
                            ctx.closePath();
                            ctx.fill();
                        }
                    }
                    Label {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: progress.top
                        anchors.margins: 12
                        text: tile.entry.title
                        textFormat: Text.PlainText
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        color: "#FFFFFF"
                        elide: Text.ElideRight
                    }
                    Label {
                        visible: tile.hovered || tile.visualFocus
                        anchors.left: parent.left
                        anchors.top: parent.top
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
            }
        }
    }
}
