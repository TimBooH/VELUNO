import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: panel
    required property QtObject theme
    required property QtObject library
    spacing: 10
    Label {
        text: library.status
        textFormat: Text.PlainText
        color: theme.text
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    Label {
        visible: library.busy
        text: library.movieCount + " Filme · " + library.seriesCount + " Serien · " + library.unassignedCount + " nicht zugeordnet"
        color: theme.muted
    }
    ProgressBar {
        Layout.fillWidth: true
        visible: library.busy
        indeterminate: library.checked === 0
        from: 0
        to: Math.max(1, library.total)
        value: library.checked
    }
    Label {
        visible: library.busy
        text: library.checked + " / " + library.total + " Dateien geprüft"
        color: theme.muted
    }
    Label {
        visible: library.busy
        text: library.currentPath
        textFormat: Text.PlainText
        color: theme.muted
        elide: Text.ElideMiddle
        Layout.fillWidth: true
    }
    RowLayout {
        Button {
            objectName: "scanButton"
            text: qsTr("Bibliothek prüfen")
            enabled: !library.busy && library.sourceCount > 0
            onClicked: library.scan()
        }
        Button {
            objectName: "pauseScanButton"
            visible: library.busy
            text: library.paused ? "Fortsetzen" : "Pausieren"
            onClicked: library.togglePause()
        }
    }
    Label {
        visible: library.watchNotice.length > 0
        text: library.watchNotice
        color: theme.muted
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    Label {
        visible: library.errorCount > 0
        text: qsTr("Scanfehler (") + library.errorCount + ")"
        color: theme.text
        font.weight: Font.DemiBold
    }
    Label {
        visible: library.errorCount > 0
        text: qsTr("Wiederholen prüft den betroffenen Medienordner erneut. Dateipfade können für die Fehlersuche markiert und kopiert werden.")
        color: theme.muted
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    ListView {
        visible: count > 0
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(300, contentHeight)
        clip: true
        spacing: 8
        model: library.errors
        ScrollBar.vertical: ScrollBar {}
        delegate: Rectangle {
            required property var entry
            required property int index
            width: ListView.view.width
            height: errorContent.implicitHeight + 16
            color: panel.theme.surface
            radius: 6
            RowLayout {
                id: errorContent
                anchors.fill: parent
                anchors.margins: 8
                ColumnLayout {
                    Layout.fillWidth: true
                    TextArea {
                        readOnly: true
                        selectByMouse: true
                        padding: 0
                        background: null
                        wrapMode: TextEdit.Wrap
                        text: entry.path
                        textFormat: Text.PlainText
                        color: panel.theme.text
                        Layout.fillWidth: true
                    }
                    Label {
                        text: entry.message
                        textFormat: Text.PlainText
                        color: panel.theme.muted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
                Button {
                    text: qsTr("Wiederholen")
                    enabled: !panel.library.busy
                    onClicked: panel.library.scan(entry.source_id)
                    Accessible.name: qsTr("Zugehörigen Ordner erneut prüfen")
                }
            }
        }
    }
}
