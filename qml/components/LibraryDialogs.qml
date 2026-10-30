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
    TitleDetails {
        id: detail
        theme: dialogs.theme
        library: dialogs.library
        personal: dialogs.personal
        onAssignRequested: function (entry) {
            dialogs.assign(entry);
        }
        onCollectionsRequested: memberships.open()
        onEditRequested: {
            metadataSearch.text = library.selectedTitle.title;
            nameEdit.text = library.selectedTitle.title;
            yearEdit.value = library.selectedTitle.year || 0;
            overviewEdit.text = library.selectedTitle.overview || "";
            metadata.showImages(library.selectedTitle.id);
            infoEditor.open();
        }
    }
    Dialog {
        id: infoEditor
        objectName: "titleInfoEditor"
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
                        enabled: metadata.enabled && metadata.configured
                        onClicked: metadata.search(library.selectedTitle.id, metadataSearch.text)
                    }
                    Button {
                        text: qsTr("Aktualisieren")
                        enabled: metadata.enabled && metadata.configured
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
        objectName: "titleCollections"
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
