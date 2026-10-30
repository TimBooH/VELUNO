import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: page
    required property QtObject theme
    required property QtObject updates
    required property QtObject personal
    required property QtObject backups
    contentWidth: availableWidth
    ColumnLayout {
        width: parent.width
        spacing: 14
        Label {
            text: page.updates.buildInfo.version + " · " + page.updates.buildInfo.build
            color: page.theme.muted
            textFormat: Text.PlainText
        }
        ComboBox {
            model: [qsTr("Stable"), qsTr("Unstable")]
            currentIndex: page.updates.channel === "unstable" ? 1 : 0
            enabled: !page.updates.busy
            onActivated: page.updates.setChannel(currentIndex ? "unstable" : "stable")
        }
        Label {
            text: qsTr("Updates werden geprüft und heruntergeladen. Die Installation startest ausschließlich du. Zuvor wird eine separate Sicherung erstellt.")
            color: page.theme.muted
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Label {
            text: qsTr("Stable-Manifest (HTTPS)")
            color: page.theme.text
        }
        TextField {
            id: stableUrl
            text: page.updates.settings.stable
            placeholderText: qsTr("https://…/stable.json")
            Layout.fillWidth: true
            enabled: !page.updates.busy
        }
        Label {
            text: qsTr("Unstable-Manifest (dauerhafte HTTPS-Quelle)")
            color: page.theme.text
        }
        TextField {
            id: unstableUrl
            text: page.updates.settings.unstable
            placeholderText: qsTr("https://…/unstable.json")
            Layout.fillWidth: true
            enabled: !page.updates.busy
        }
        Label {
            text: qsTr("Öffentlicher Ed25519-Prüfschlüssel (PEM)")
            color: page.theme.text
        }
        TextArea {
            id: publicKey
            text: page.updates.settings.key
            placeholderText: qsTr("-----BEGIN PUBLIC KEY-----")
            wrapMode: TextEdit.WrapAnywhere
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            enabled: !page.updates.busy
        }
        RowLayout {
            Button {
                text: qsTr("Quelle speichern")
                enabled: !page.updates.busy
                onClicked: page.updates.configure(stableUrl.text, unstableUrl.text, publicKey.text)
            }
            Button {
                text: qsTr("Jetzt prüfen")
                enabled: !page.updates.busy
                onClicked: page.updates.check()
            }
        }
        Label {
            text: page.updates.status
            color: page.theme.accent
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Label {
            text: page.updates.candidate.notes || ""
            textFormat: Text.PlainText
            color: page.theme.text
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        ProgressBar {
            visible: page.updates.busy
            from: 0
            to: 1
            value: page.updates.progress
            Layout.fillWidth: true
        }
        RowLayout {
            Button {
                text: qsTr("Update herunterladen")
                enabled: !!page.updates.candidate.version && !page.updates.busy
                onClicked: page.updates.download()
            }
            Button {
                text: qsTr("Download abbrechen")
                visible: page.updates.busy
                onClicked: page.updates.cancel()
            }
            Button {
                text: qsTr("Geprüftes Update installieren")
                enabled: page.updates.downloaded && !page.updates.busy && !page.backups.busy && !page.updates.restartRequired
                onClicked: installation.open()
            }
        }
        Button {
            visible: page.updates.restartRequired
            text: qsTr("Jetzt schließen und VELUNO neu starten")
            onClicked: Qt.quit()
        }
    }
    Dialog {
        id: installation
        parent: Overlay.overlay
        anchors.centerIn: parent
        title: qsTr("Update jetzt installieren?")
        width: 540
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        Label {
            width: parent.width
            text: qsTr("Die Wiedergabe wird beendet und dein Datenstand gesichert. Erst nach erfolgreicher Sicherung wird die AppImage-Datei atomar ersetzt. Die bisherige Datei bleibt erhalten. Anschließend ist ein Neustart nötig.")
            wrapMode: Text.WordWrap
            color: page.theme.text
        }
        onAccepted: {
            page.personal.stopTracking();
            page.updates.install();
        }
    }
}
