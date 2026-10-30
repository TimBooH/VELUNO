import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ColumnLayout {
    id: page
    required property QtObject theme
    required property QtObject backups
    required property QtObject personal
    Label {
        text: qsTr("Täglich werden sieben erfolgreiche Tagesstände aufbewahrt. Manuelle und Update-Sicherungen bleiben erhalten. Videos werden nicht kopiert.")
        color: theme.muted
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    Label {
        text: backups.directory
        color: theme.text
        textFormat: Text.PlainText
        elide: Text.ElideMiddle
        Layout.fillWidth: true
    }
    Flow {
        Layout.fillWidth: true
        spacing: 8
        Button {
            text: qsTr("Speicherort wählen")
            enabled: !backups.busy
            onClicked: locationPicker.open()
        }
        Button {
            text: qsTr("Standardordner")
            enabled: !backups.busy
            onClicked: backups.resetDirectory()
        }
        Button {
            objectName: "manualBackup"
            text: qsTr("Jetzt sichern")
            enabled: !backups.busy && !backups.restartRequired
            onClicked: {
                personal.saveProgress();
                backups.create("manual");
            }
        }
        Button {
            text: qsTr("Sicherung auswählen")
            enabled: !backups.busy
            onClicked: restorePicker.open()
        }
    }
    BusyIndicator {
        running: backups.busy
        visible: running
    }
    Label {
        text: backups.status
        color: theme.accent
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    Button {
        visible: backups.restartRequired
        text: qsTr("Jetzt schließen und VELUNO neu starten")
        onClicked: Qt.quit()
    }
    Label {
        visible: backups.restartRequired
        text: qsTr("Die vollständige Wiederherstellung wird beim nächsten Programmstart aktiviert. Der bisherige Datenstand wurde vorsorglich gesichert.")
        color: theme.muted
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    ListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: 8
        model: backups.backups
        ScrollBar.vertical: ScrollBar {}
        delegate: Rectangle {
            required property var modelData
            width: ListView.view.width
            height: 92
            color: page.theme.surface
            radius: 8
            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                ColumnLayout {
                    Layout.fillWidth: true
                    Label {
                        text: (modelData.kind || "Sicherung") + " · " + (modelData.created || modelData.name)
                        color: page.theme.text
                        textFormat: Text.PlainText
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                    Label {
                        text: modelData.valid ? "Vollständigkeit, Prüfsummen und Datenbank geprüft" : modelData.error
                        color: modelData.valid ? page.theme.muted : page.theme.accent
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
                Button {
                    text: qsTr("Wiederherstellen")
                    enabled: modelData.valid && !page.backups.busy && !page.backups.restartRequired
                    onClicked: {
                        restoration.path = modelData.path;
                        restoration.open();
                    }
                }
                Button {
                    text: qsTr("Löschen")
                    enabled: !page.backups.busy
                    onClicked: {
                        deletion.path = modelData.path;
                        deletion.open();
                    }
                }
            }
        }
    }
    FolderDialog {
        id: locationPicker
        title: qsTr("Sicherungsordner wählen")
        onAccepted: backups.setDirectory(selectedFolder)
    }
    FolderDialog {
        id: restorePicker
        title: qsTr("Vollständige .veluno-backup-Sicherung auswählen")
        onAccepted: {
            restoration.path = selectedFolder.toString();
            restoration.open();
        }
    }
    Dialog {
        id: restoration
        property string path
        parent: Overlay.overlay
        anchors.centerIn: parent
        title: qsTr("Alle VELUNO-Daten wiederherstellen?")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        Label {
            width: 480
            text: qsTr("Profile, Bibliothek, Einstellungen und Bilder werden gemeinsam ersetzt. Vorher wird der aktuelle Datenstand gesichert. Die Wiederherstellung wird beim Neustart aktiviert. Originalvideos bleiben erhalten.")
            wrapMode: Text.WordWrap
            color: page.theme.text
        }
        onAccepted: {
            personal.stopTracking();
            backups.restore(path);
        }
    }
    Dialog {
        id: deletion
        property string path
        parent: Overlay.overlay
        anchors.centerIn: parent
        title: qsTr("Diese Sicherung endgültig löschen?")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: backups.remove(path)
    }
}
