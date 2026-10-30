import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../components"

ColumnLayout {
    id: page
    required property QtObject theme
    required property QtObject personal
    required property QtObject library
    RowLayout {
        Layout.fillWidth: true
        Button {
            visible: !!personal.activeCollection.id
            text: qsTr("Alle Sammlungen")
            onClicked: personal.openCollection(0)
        }
        Label {
            text: personal.activeCollection.name || "Sammlungen"
            color: theme.text
            font.pixelSize: 28
            Layout.fillWidth: true
        }
        Button {
            text: personal.activeCollection.id ? qsTr("Umbenennen") : qsTr("Neue Sammlung")
            onClicked: {
                nameInput.text = personal.activeCollection.name || "";
                editor.open();
            }
        }
        Button {
            visible: !!personal.activeCollection.id
            text: qsTr("Eigenes Cover")
            onClicked: coverPicker.open()
        }
        Button {
            visible: !!personal.activeCollection.id
            text: qsTr("Löschen")
            onClicked: removal.open()
        }
        ComboBox {
            visible: !!personal.activeCollection.id
            model: [qsTr("Titel"), qsTr("Hinzufügedatum")]
            currentIndex: personal.activeCollection.sort === "recent" ? 1 : 0
            onActivated: personal.setCollectionSort(currentIndex ? "recent" : "title")
        }
    }
    ListView {
        visible: !personal.activeCollection.id
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: personal.collections
        spacing: 12
        clip: true
        ScrollBar.vertical: ScrollBar {}
        delegate: Rectangle {
            required property var entry
            width: ListView.view.width
            height: 150
            radius: 8
            color: page.theme.surface
            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                Rectangle {
                    Layout.preferredWidth: 180
                    Layout.fillHeight: true
                    color: page.theme.raised
                    Image {
                        anchors.fill: parent
                        source: entry.cover_url || ""
                        fillMode: Image.PreserveAspectCrop
                        visible: !!entry.cover_url
                    }
                    Row {
                        anchors.fill: parent
                        visible: !entry.cover_url
                        Repeater {
                            model: entry.posters
                            Rectangle {
                                required property var modelData
                                width: 44
                                height: 125
                                color: page.theme.raised
                                border.color: page.theme.border
                                Image {
                                    anchors.fill: parent
                                    source: page.library.assetUrl(modelData.poster || "")
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                }
                                Label {
                                    visible: !modelData.poster
                                    anchors.centerIn: parent
                                    text: modelData.title.substring(0, 1)
                                    color: page.theme.accent
                                    font.pixelSize: 24
                                }
                            }
                        }
                    }
                }
                Label {
                    text: entry.name
                    textFormat: Text.PlainText
                    color: page.theme.text
                    font.pixelSize: 21
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Öffnen")
                    onClicked: page.personal.openCollection(entry.id)
                }
            }
        }
    }
    GridView {
        visible: !!personal.activeCollection.id
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: personal.collectionTitles
        cellWidth: library.tileSize + 18
        cellHeight: library.tileSize * 1.68 + 18
        ScrollBar.vertical: ScrollBar {}
        delegate: PosterCard {
            theme: page.theme
            width: page.library.tileSize
            height: width * 1.68
            onDetailsRequested: page.library.openDetails(entry.id)
            onPlayRequested: page.library.playTitle(entry.id)
        }
    }
    Label {
        visible: personal.error.length > 0
        text: personal.error
        color: theme.accent
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    Dialog {
        id: editor
        parent: Overlay.overlay
        anchors.centerIn: parent
        title: qsTr("Sammlung")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        TextField {
            id: nameInput
            placeholderText: qsTr("Name")
            width: 360
        }
        onAccepted: personal.saveCollection(personal.activeCollection.id || 0, nameInput.text)
    }
    Dialog {
        id: removal
        parent: Overlay.overlay
        anchors.centerIn: parent
        title: qsTr("Sammlung löschen?")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        Label {
            text: qsTr("Titel und Originalvideos bleiben erhalten.")
            color: page.theme.text
        }
        onAccepted: personal.deleteCollection(personal.activeCollection.id)
    }
    FileDialog {
        id: coverPicker
        nameFilters: ["Bilder (*.png *.jpg *.jpeg *.webp)"]
        onAccepted: personal.setCollectionCover(selectedFile)
    }
}
