import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: dialogs
    required property QtObject theme
    required property QtObject metadata
    required property QtObject personal
    required property QtObject library
    function assign(entry) {
        assignment.fileId = entry.id;
        assignment.kind = entry.kind || library.selectedTitle.kind;
        titleInput.text = entry.title || (entry.kind ? "" : library.selectedTitle.title || "");
        yearInput.value = entry.year || (entry.kind ? 0 : library.selectedTitle.year || 0);
        seasonInput.value = entry.season || 0;
        episodeInput.value = entry.episode || 1;
        assignment.open();
    }
    Connections {
        target: library
        function onDetailsRequested() {
            detail.open();
        }
        function onPlayRequested() {
            detail.close();
        }
    }
    Dialog {
        id: detail
        objectName: "titleDetails"
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 850)
        height: Math.min(parent.height - 64, 590)
        title: library.selectedTitle.title || "Details"
        modal: true
        standardButtons: Dialog.Close
        ColumnLayout {
            anchors.fill: parent
            spacing: 12
            Label {
                text: (library.selectedTitle.year || "") + (library.selectedTitle.kind === "series" ? " · Staffeln und Episoden" : " · Filmversionen – höchste Auflösung zuerst")
                color: dialogs.theme.muted
            }
            Label {
                text: qsTr("Titel wurden lokal aus Dateinamen erkannt. Originaldateien bleiben unverändert.")
                color: dialogs.theme.muted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Image {
                visible: !!library.selectedTitle.backdrop
                source: library.assetUrl(library.selectedTitle.backdrop || "")
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
            }
            Label {
                text: library.selectedTitle.genres || ""
                color: dialogs.theme.muted
                visible: text.length > 0
            }
            ScrollView {
                visible: !!library.selectedTitle.overview
                Layout.fillWidth: true
                Layout.preferredHeight: 85
                contentWidth: availableWidth
                Label {
                    width: parent.width
                    text: library.selectedTitle.overview || ""
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                    color: dialogs.theme.text
                }
            }
            RowLayout {
                Button {
                    text: qsTr("Informationen")
                    onClicked: {
                        metadataSearch.text = library.selectedTitle.title;
                        nameEdit.text = library.selectedTitle.title;
                        yearEdit.value = library.selectedTitle.year || 0;
                        overviewEdit.text = library.selectedTitle.overview || "";
                        metadata.showImages(library.selectedTitle.id);
                        infoEditor.open();
                    }
                }
                Button {
                    text: {
                        personal.revision;
                        return personal.isFavorite(library.selectedTitle.id || 0) ? qsTr("Favorit entfernen") : qsTr("Als Favorit");
                    }
                    onClicked: personal.toggleFavorite(library.selectedTitle.id)
                }
                Button {
                    text: {
                        personal.revision;
                        return personal.isSeen(library.selectedTitle.id || 0) ? qsTr("Als ungesehen") : qsTr("Als gesehen");
                    }
                    onClicked: personal.markSeen(library.selectedTitle.id, !personal.isSeen(library.selectedTitle.id))
                }
                Button {
                    text: qsTr("Sammlungen")
                    onClicked: memberships.open()
                }
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: library.selectedFiles
                clip: true
                spacing: 10
                ScrollBar.vertical: ScrollBar {}
                delegate: Rectangle {
                    required property var entry
                    width: ListView.view.width
                    height: 106
                    color: dialogs.theme.surface
                    radius: 7
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                text: (dialogs.library.selectedTitle.kind === "series" ? "S" + entry.season + " E" + entry.episode + " · " : "") + entry.width + " × " + entry.height + (entry.available ? "" : " · Nicht verfügbar")
                                color: dialogs.theme.text
                            }
                            Label {
                                text: entry.path
                                textFormat: Text.PlainText
                                color: dialogs.theme.muted
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                            Label {
                                visible: !!entry.diagnostic
                                text: entry.diagnostic || ""
                                textFormat: Text.PlainText
                                color: dialogs.theme.muted
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }
                        Button {
                            text: qsTr("Zuordnen")
                            onClicked: dialogs.assign(entry)
                        }
                        Button {
                            text: qsTr("Abspielen")
                            enabled: entry.available
                            onClicked: {
                                dialogs.library.playFile(entry.path);
                                if (!dialogs.library.error.length)
                                    detail.close();
                            }
                        }
                    }
                }
            }
        }
    }
    Dialog {
        id: infoEditor
        parent: Overlay.overlay
        anchors.centerIn: parent
        title: qsTr("Titelinformationen und Bilder")
        width: Math.min(parent.width - 64, 900)
        height: Math.min(parent.height - 64, 680)
        modal: true
        standardButtons: Dialog.Close
        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth
            ColumnLayout {
                width: parent.width
                spacing: 12
                TextField {
                    id: nameEdit
                    placeholderText: qsTr("Titel")
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: yearEdit
                    from: 0
                    to: 9999
                    editable: true
                    Accessible.name: qsTr("Jahr")
                }
                TextArea {
                    id: overviewEdit
                    placeholderText: qsTr("Beschreibung")
                    wrapMode: TextEdit.WordWrap
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                }
                Button {
                    text: qsTr("Eigene Änderungen speichern")
                    onClicked: metadata.edit(library.selectedTitle.id, nameEdit.text, overviewEdit.text, yearEdit.value)
                }
                RowLayout {
                    TextField {
                        id: metadataSearch
                        placeholderText: qsTr("Bei TMDB suchen")
                        Layout.fillWidth: true
                    }
                    Button {
                        text: qsTr("Suchen")
                        enabled: metadata.configured
                        onClicked: metadata.search(library.selectedTitle.id, metadataSearch.text)
                    }
                    Button {
                        text: qsTr("Aktualisieren")
                        enabled: metadata.configured
                        onClicked: metadata.refreshTitle(library.selectedTitle.id)
                    }
                }
                Label {
                    text: metadata.status
                    color: dialogs.theme.muted
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Repeater {
                    model: metadata.results
                    ItemDelegate {
                        required property var modelData
                        text: modelData.title + " (" + modelData.year + ")"
                        Layout.fillWidth: true
                        onClicked: metadata.choose(modelData.id)
                    }
                }
                RowLayout {
                    Button {
                        text: qsTr("Eigenes Poster")
                        onClicked: {
                            coverPicker.kind = "poster";
                            coverPicker.open();
                        }
                    }
                    Button {
                        text: qsTr("Eigenes Hintergrundbild")
                        onClicked: {
                            coverPicker.kind = "backdrop";
                            coverPicker.open();
                        }
                    }
                }
                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    Repeater {
                        model: metadata.images
                        Button {
                            required property var modelData
                            width: 130
                            height: 185
                            onClicked: metadata.selectImage(modelData.id)
                            contentItem: Image {
                                source: modelData.url
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                            }
                        }
                    }
                }
            }
        }
    }
    FileDialog {
        id: coverPicker
        property string kind: "poster"
        nameFilters: ["Bilder (*.png *.jpg *.jpeg *.webp)"]
        onAccepted: metadata.selectLocalImage(library.selectedTitle.id, kind, selectedFile)
    }
    Dialog {
        id: memberships
        parent: Overlay.overlay
        anchors.centerIn: parent
        title: qsTr("Zu Sammlungen hinzufügen")
        width: 430
        height: 360
        modal: true
        standardButtons: Dialog.Close
        ListView {
            anchors.fill: parent
            model: personal.collections
            clip: true
            delegate: CheckDelegate {
                required property var entry
                width: ListView.view.width
                text: entry.name
                checked: {
                    personal.revision;
                    return personal.collectionContains(entry.id, library.selectedTitle.id || 0);
                }
                onClicked: personal.toggleCollectionTitle(entry.id, library.selectedTitle.id)
            }
        }
    }
    Dialog {
        id: assignment
        objectName: "assignmentDialog"
        property var fileId: 0
        property string kind: "movie"
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 480
        title: qsTr("Datei zuordnen")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        ColumnLayout {
            anchors.fill: parent
            spacing: 12
            Label {
                text: qsTr("Titel")
                color: dialogs.theme.text
            }
            TextField {
                id: titleInput
                objectName: "assignmentTitle"
                Layout.fillWidth: true
                placeholderText: qsTr("Film- oder Serientitel")
                Accessible.name: qsTr("Titel der Zuordnung")
            }
            RowLayout {
                Label {
                    text: qsTr("Jahr (0 = unbekannt)")
                    color: dialogs.theme.text
                }
                SpinBox {
                    id: yearInput
                    from: 0
                    to: 9999
                    editable: true
                }
            }
            RowLayout {
                visible: assignment.kind === "series"
                Label {
                    text: qsTr("Staffel")
                    color: dialogs.theme.text
                }
                SpinBox {
                    id: seasonInput
                    from: 0
                    to: 999
                    editable: true
                }
                Label {
                    text: qsTr("Episode")
                    color: dialogs.theme.text
                }
                SpinBox {
                    id: episodeInput
                    from: 1
                    to: 9999
                    editable: true
                }
            }
            Label {
                text: qsTr("Die Zuordnung wird gespeichert und durch weitere Scans nicht überschrieben.")
                color: dialogs.theme.muted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
        onAccepted: {
            library.assignFile(fileId, titleInput.text, yearInput.value, seasonInput.value, episodeInput.value);
            if (!library.error.length)
                detail.close();
        }
    }
}
