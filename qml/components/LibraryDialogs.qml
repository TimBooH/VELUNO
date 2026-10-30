import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: dialogs
    required property QtObject theme
    required property QtObject library
    function assign(entry) {
        assignment.fileId = entry.id
        assignment.kind = entry.kind || library.selectedTitle.kind
        titleInput.text = entry.title || (entry.kind ? "" : library.selectedTitle.title || "")
        yearInput.value = entry.year || (entry.kind ? 0 : library.selectedTitle.year || 0)
        seasonInput.value = entry.season || 0
        episodeInput.value = entry.episode || 1
        assignment.open()
    }
    Connections { target: library; function onDetailsRequested() { detail.open() } function onPlayRequested() { detail.close() } }
    Dialog {
        id: detail
        objectName: "titleDetails"
        parent: Overlay.overlay; anchors.centerIn: parent
        width: Math.min(parent.width - 64,850); height: Math.min(parent.height - 64,590)
        title: library.selectedTitle.title || "Details"
        modal: true; standardButtons: Dialog.Close
        ColumnLayout {
            anchors.fill: parent; spacing: 12
            Label { text: (library.selectedTitle.year || "") + (library.selectedTitle.kind === "series" ? " · Staffeln und Episoden" : " · Filmversionen – höchste Auflösung zuerst"); color: dialogs.theme.muted }
            Label { text: "Titel wurden lokal aus Dateinamen erkannt. Originaldateien bleiben unverändert."; color: dialogs.theme.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            ListView {
                Layout.fillWidth: true; Layout.fillHeight: true; model: library.selectedFiles; clip: true; spacing: 10
                ScrollBar.vertical: ScrollBar {}
                delegate: Rectangle {
                    required property var entry
                    width: ListView.view.width; height: 106; color: dialogs.theme.surface; radius: 7
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 10
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label { text: (dialogs.library.selectedTitle.kind === "series" ? "S" + entry.season + " E" + entry.episode + " · " : "") + entry.width + " × " + entry.height + (entry.available ? "" : " · Nicht verfügbar"); color: dialogs.theme.text }
                            Label { text: entry.path; textFormat: Text.PlainText; color: dialogs.theme.muted; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            Label { visible: !!entry.diagnostic; text: entry.diagnostic || ""; textFormat: Text.PlainText; color: dialogs.theme.muted; elide: Text.ElideRight; Layout.fillWidth: true }
                        }
                        Button { text: "Zuordnen"; onClicked: dialogs.assign(entry) }
                        Button { text: "Abspielen"; enabled: entry.available; onClicked: { dialogs.library.playFile(entry.path); if (!dialogs.library.error.length) detail.close() } }
                    }
                }
            }
        }
    }
    Dialog {
        id: assignment
        objectName: "assignmentDialog"
        property var fileId: 0
        property string kind: "movie"
        parent: Overlay.overlay; anchors.centerIn: parent
        width: 480; title: "Datei zuordnen"; modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        ColumnLayout {
            anchors.fill: parent; spacing: 12
            Label { text: "Titel"; color: dialogs.theme.text }
            TextField { id: titleInput; objectName: "assignmentTitle"; Layout.fillWidth: true; placeholderText: "Film- oder Serientitel"; Accessible.name: "Titel der Zuordnung" }
            RowLayout {
                Label { text: "Jahr (0 = unbekannt)"; color: dialogs.theme.text }
                SpinBox { id: yearInput; from: 0; to: 9999; editable: true }
            }
            RowLayout {
                visible: assignment.kind === "series"
                Label { text: "Staffel"; color: dialogs.theme.text }
                SpinBox { id: seasonInput; from: 0; to: 999; editable: true }
                Label { text: "Episode"; color: dialogs.theme.text }
                SpinBox { id: episodeInput; from: 1; to: 9999; editable: true }
            }
            Label { text: "Die Zuordnung wird gespeichert und durch weitere Scans nicht überschrieben."; color: dialogs.theme.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
        onAccepted: { library.assignFile(fileId,titleInput.text,yearInput.value,seasonInput.value,episodeInput.value); if (!library.error.length) detail.close() }
    }
}
