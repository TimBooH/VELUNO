import QtQuick
import QtQuick.Controls

Button {
    id: card
    required property QtObject theme
    required property var entry
    readonly property int artworkStatus: posterImage.status
    signal detailsRequested
    signal playRequested
    objectName: "poster-" + entry.id
    implicitWidth: 180
    implicitHeight: implicitWidth * 1.5 + 38
    hoverEnabled: true
    padding: 0
    scale: hovered && !down ? 1.035 : 1
    z: hovered || visualFocus ? 2 : 0
    Behavior on scale  {
        enabled: card.hovered && !card.visualFocus && !card.down
        NumberAnimation {
            duration: 130
            easing.type: Easing.OutCubic
        }
    }
    Accessible.name: entry.title + ", " + qsTr("Details öffnen")
    onClicked: detailsRequested()
    background: Item {
    }
    contentItem: Item {
        Rectangle {
            id: artwork
            width: parent.width
            height: width * 1.5
            radius: 4
            clip: true
            color: card.theme.surface
            // Typography remains useful offline, including unreadable cached artwork.
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: card.theme.raised
                    }
                    GradientStop {
                        position: 1
                        color: card.theme.surface
                    }
                }
                Rectangle {
                    width: parent.width * 0.5
                    height: parent.height * 1.4
                    rotation: 28
                    x: parent.width * 0.6
                    y: -parent.height * 0.2
                    color: card.theme.accent
                    opacity: 0.045
                }
                Label {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.margins: 16
                    text: card.entry.kind === "series" ? qsTr("SERIE") : qsTr("FILM")
                    font.pixelSize: 10
                    font.letterSpacing: 2
                    color: card.theme.muted
                }
                Label {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 16
                    anchors.bottomMargin: 20
                    text: card.entry.title
                    textFormat: Text.PlainText
                    font.pixelSize: Math.max(18, card.width * 0.13)
                    font.weight: Font.Bold
                    color: card.theme.text
                    wrapMode: Text.Wrap
                    maximumLineCount: 4
                    elide: Text.ElideRight
                }
            }
            Image {
                id: posterImage
                objectName: "posterArtwork"
                anchors.fill: parent
                source: library.assetUrl(card.entry.poster || "")
                sourceSize.width: Math.ceil(card.width * 2)
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
            }
            Rectangle {
                anchors.fill: parent
                visible: card.hovered || card.visualFocus
                color: "#55000000"
            }
            Rectangle {
                anchors.fill: parent
                radius: 4
                color: "transparent"
                border.width: card.visualFocus ? 2 : 0
                border.color: card.theme.accent
            }
            Button {
                id: playButton
                objectName: "posterPlay"
                anchors.centerIn: parent
                width: 48
                height: 48
                visible: card.hovered || card.visualFocus || hovered || visualFocus
                text: "\u25B6\uFE0E"
                Accessible.name: card.entry.kind === "series" ? qsTr("Episoden") : qsTr("Abspielen")
                contentItem: Label {
                    text: playButton.text
                    font.pixelSize: 20
                    color: card.theme.accentText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 24
                    color: playButton.down ? card.theme.muted : card.theme.accent
                }
                onClicked: card.playRequested()
            }
            Label {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 8
                visible: !card.entry.available
                text: qsTr("Offline")
                font.pixelSize: 10
                color: "white"
                padding: 5
                background: Rectangle {
                    radius: 3
                    color: "#CC0B1426"
                }
            }
        }
        Label {
            anchors.top: artwork.bottom
            anchors.topMargin: 9
            width: parent.width
            text: card.entry.title
            textFormat: Text.PlainText
            elide: Text.ElideRight
            font.pixelSize: 13
            font.weight: Font.DemiBold
            color: card.theme.text
        }
    }
}
