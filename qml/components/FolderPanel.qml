import QtQuick
import QtQuick.Controls
// Required by Qt 6.4 FolderDialog fallback; explicit import also bundles it in AppImage.
import Qt.labs.folderlistmodel
import QtQuick.Dialogs
import QtQuick.Layouts
ColumnLayout {
    id: panel
    required property QtObject theme
    required property QtObject library
    spacing: 12
    RowLayout {
        ComboBox { id: category; model: ["Filme", "Serien"]; Accessible.name: "Kategorie des Medienordners" }
        Button { objectName: "addSmbButton"; text: "SMB-Freigabe"; onClicked: smbDialog.open() }
        Button { objectName: "addFolderButton"; text: "Ordner hinzufügen"; onClicked: picker.open() }
    }
    Label {
        text: "Film- und Serienordner werden getrennt und mit ihren Unterordnern eingelesen. Deine Originaldateien bleiben unverändert."
        color: theme.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true
    }
    ListView {
        Layout.fillWidth: true; Layout.preferredHeight: Math.min(180,Math.max(54,contentHeight))
        clip: true; model: library.sources; spacing: 8
        ScrollBar.vertical: ScrollBar {}
        delegate: Rectangle {
            required property var entry
            visible: entry.enabled
            width: ListView.view.width; height: visible ? 62 : 0
            color: panel.theme.surface; radius: 6; border.color: panel.theme.border
            RowLayout {
                anchors.fill: parent; anchors.margins: 10
                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: entry.kind === "movie" ? "Filme" : "Serien"; color: panel.theme.accent }
                    Label { text: entry.path; textFormat: Text.PlainText; color: panel.theme.text; elide: Text.ElideMiddle; Layout.fillWidth: true }
                }
                Button { text: "Entfernen"; enabled: !panel.library.busy; onClicked: { removal.sourceId = entry.id; removal.open() } }
            }
        }
    }
    Label { visible: library.sourceCount === 0; text: "Noch keine Medienordner ausgewählt."; color: theme.muted }
    Dialog {
        id: smbDialog
        objectName: "smbSourceDialog"
        parent: Overlay.overlay; anchors.centerIn: parent
        width: Math.min(parent.width - 64,620)
        title: "SMB-Freigabe hinzufügen"; modal: true
        ColumnLayout {
            anchors.fill: parent; spacing: 14
            Label { text: "Adresse der Freigabe oder eines Unterordners"; color: panel.theme.text }
            TextField { id: smbAddress; objectName: "smbAddress"; Layout.fillWidth: true; placeholderText: "smb://NAS/Filme"; Accessible.name: "SMB-Adresse"; selectByMouse: true }
            Label { text: "Die Anmeldung übernimmt KDE. Falls nötig, öffne die Freigabe zuerst in Dolphin. Gib hier keine Zugangsdaten ein."; wrapMode: Text.WordWrap; Layout.fillWidth: true; color: panel.theme.muted }
        }
        footer: DialogButtonBox {
            Button { text: "Abbrechen"; DialogButtonBox.buttonRole: DialogButtonBox.RejectRole; onClicked: smbDialog.close() }
            Button { objectName: "confirmSmbSource"; text: "Hinzufügen"; enabled: smbAddress.text.trim().length > 0; onClicked: { if (panel.library.addSource(smbAddress.text.trim(),category.currentIndex === 0 ? "movie" : "series")) smbDialog.close() } }
        }
    }
    FolderDialog {
        id: picker
        objectName: "mediaFolderPicker"
        title: category.currentIndex === 0 ? "Filmordner wählen" : "Serienordner wählen"
        onAccepted: panel.library.addSource(selectedFolder,category.currentIndex === 0 ? "movie" : "series")
    }
    Dialog {
        id: removal
        property var sourceId: 0
        parent: Overlay.overlay; anchors.centerIn: parent
        title: "Ordner nicht mehr überwachen?"; modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        Label { width: 420; text: "Bibliothekseinträge bleiben erhalten und werden als nicht verfügbar markiert. Originaldateien werden nicht verändert."; wrapMode: Text.WordWrap; color: panel.theme.text }
        onAccepted: panel.library.removeSource(sourceId)
    }
}
