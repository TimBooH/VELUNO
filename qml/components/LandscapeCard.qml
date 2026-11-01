import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: card
    required property QtObject theme
    required property QtObject library
    required property QtObject personal
    required property var entry
    property bool previewAllowed: true
    property int previewRevision: 0
    onPreviewRevisionChanged: {
        if (openDelay)
            openDelay.stop();
        if (preview)
            preview.close();
    }
    readonly property int artworkStatus: backdrop.status === Image.Ready ? backdrop.status : poster.status
    readonly property bool favorite: {
        personal.revision;
        return personal.isFavorite(entry.id || 0);
    }
    signal detailsRequested
    signal playRequested
    objectName: "poster-" + entry.id
    implicitWidth: 252
    implicitHeight: width * 9 / 16
    padding: 0
    hoverEnabled: true
    Accessible.name: entry.title + ", " + qsTr("Details öffnen")
    onClicked: {
        preview.close();
        detailsRequested();
    }
    onHoveredChanged: {
        if (hovered && previewAllowed)
            openDelay.restart();
        else {
            openDelay.stop();
            closeDelay.restart();
        }
    }
    onPreviewAllowedChanged: if (!previewAllowed) {
        openDelay.stop();
        preview.close();
    }
    onVisibleChanged: if (!visible)
        preview.close()
    background: Rectangle {
        color: card.theme.surface
        radius: 4
    }
    contentItem: Item {
        clip: true
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
        }
        Image {
            id: poster
            objectName: "posterArtwork"
            anchors.fill: parent
            source: backdrop.source.toString().length === 0 || backdrop.status === Image.Error ? card.library.assetUrl(card.entry.poster || "") : ""
            sourceSize.width: Math.ceil(card.width * 2)
            asynchronous: true
            fillMode: Image.PreserveAspectCrop
            visible: backdrop.status !== Image.Ready
        }
        Image {
            id: backdrop
            objectName: "landscapeArtwork"
            anchors.fill: parent
            source: card.library.assetUrl(card.entry.backdrop || "")
            sourceSize.width: Math.ceil(card.width * 2)
            asynchronous: true
            fillMode: Image.PreserveAspectCrop
            visible: status === Image.Ready
        }
        Rectangle {
            anchors.fill: parent
            visible: card.artworkStatus === Image.Ready
            gradient: Gradient {
                GradientStop {
                    position: 0.2
                    color: "#00040A12"
                }
                GradientStop {
                    position: 1
                    color: "#D9080F1C"
                }
            }
        }
        Label {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 12
            text: card.entry.title
            textFormat: Text.PlainText
            color: card.artworkStatus === Image.Ready ? "#FFFFFF" : card.theme.text
            font.pixelSize: Math.max(16, card.width * 0.07)
            font.weight: Font.Bold
            wrapMode: Text.NoWrap
            maximumLineCount: 1
            elide: Text.ElideRight
        }
        Label {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 8
            visible: !card.entry.available
            text: qsTr("Offline")
            color: "white"
            font.pixelSize: 10
            padding: 4
            background: Rectangle {
                color: "#CC080F1C"
                radius: 3
            }
        }
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.width: card.visualFocus ? 2 : 0
            border.color: card.theme.accent
        }
        CardAction {
            objectName: "posterPlay"
            anchors.centerIn: parent
            theme: card.theme
            primary: true
            symbol: "\u25B6\uFE0E"
            text: card.entry.kind === "series" ? qsTr("Episoden") : qsTr("Abspielen")
            visible: !preview.opened && (card.hovered || card.visualFocus || hovered || visualFocus)
            onClicked: card.playRequested()
        }
    }
    Timer {
        id: openDelay
        interval: 320
        onTriggered: if (card.hovered && card.previewAllowed && card.visible) {
            const point = card.mapToItem(preview.parent, 0, 0);
            preview.x = Math.max(8, Math.min(point.x - (preview.width - card.width) / 2, preview.parent.width - preview.width - 8));
            preview.y = Math.max(8, Math.min(point.y - (preview.height - card.height) / 2, preview.parent.height - preview.height - 8));
            preview.open();
        }
    }
    Timer {
        id: closeDelay
        interval: 140
        onTriggered: if (!card.hovered && !previewHover.hovered)
            preview.close()
    }
    Popup {
        id: preview
        objectName: "cardPreview"
        parent: Overlay.overlay
        width: Math.min(card.width * 1.22, 360, parent ? parent.width - 16 : 360)
        height: width * 9 / 16 + 112
        padding: 0
        focus: false
        modal: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        enter: Transition {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 120
                easing.type: Easing.OutCubic
            }
        }
        exit: null
        Connections {
            target: preview.parent
            function onWidthChanged() {
                preview.close();
            }
            function onHeightChanged() {
                preview.close();
            }
        }
        background: Item {
            Rectangle {
                x: -6
                y: 1
                width: parent.width + 12
                height: parent.height + 9
                radius: 10
                color: "#30000000"
            }
            Rectangle {
                anchors.fill: parent
                radius: 6
                color: card.theme.surface
                border.color: card.theme.border
            }
        }
        contentItem: Item {
            HoverHandler {
                id: previewHover
                onHoveredChanged: {
                    if (hovered)
                        closeDelay.stop();
                    else
                        closeDelay.restart();
                }
            }
            Image {
                id: previewImage
                width: parent.width
                height: width * 9 / 16
                source: preview.visible ? (backdrop.status === Image.Ready ? backdrop.source : poster.source) : ""
                sourceSize.width: Math.ceil(width * 2)
                asynchronous: true
                fillMode: Image.PreserveAspectCrop
            }
            Rectangle {
                anchors.fill: previewImage
                visible: previewImage.status !== Image.Ready
                color: card.theme.raised
            }
            Rectangle {
                anchors.fill: previewImage
                gradient: Gradient {
                    GradientStop {
                        position: 0.3
                        color: "#00040A12"
                    }
                    GradientStop {
                        position: 1
                        color: "#D9080F1C"
                    }
                }
            }
            Label {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: previewImage.bottom
                anchors.margins: 14
                text: card.entry.title
                textFormat: Text.PlainText
                color: "#FFFFFF"
                font.pixelSize: 20
                font.weight: Font.Bold
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
            ColumnLayout {
                anchors.top: previewImage.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    CardAction {
                        objectName: "previewPlay"
                        theme: card.theme
                        primary: true
                        symbol: "\u25B6\uFE0E"
                        text: card.entry.kind === "series" ? qsTr("Episoden") : qsTr("Abspielen")
                        onClicked: {
                            preview.close();
                            card.playRequested();
                        }
                    }
                    CardAction {
                        objectName: "previewFavorite"
                        theme: card.theme
                        symbol: card.favorite ? "✓" : "+"
                        text: card.favorite ? qsTr("Favorit entfernen") : qsTr("Als Favorit")
                        onClicked: card.personal.toggleFavorite(card.entry.id)
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    CardAction {
                        objectName: "previewDetails"
                        theme: card.theme
                        symbol: "⌄"
                        text: qsTr("Details öffnen")
                        onClicked: {
                            preview.close();
                            card.detailsRequested();
                        }
                    }
                }
                Label {
                    Layout.fillWidth: true
                    text: [card.entry.year > 0 ? card.entry.year : "", card.entry.kind === "series" ? qsTr("Serie") : qsTr("Film"), card.entry.height >= 2160 ? "4K UHD" : card.entry.height >= 1080 ? "Full HD" : ""].filter(function (part) {
                            return part !== "";
                        }).join("  ·  ")
                    color: card.theme.muted
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }
        }
    }
}
