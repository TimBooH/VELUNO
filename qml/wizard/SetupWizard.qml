import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
Dialog {
    id: wizard
    required property QtObject theme
    required property QtObject library
    parent: Overlay.overlay; anchors.centerIn: parent
    objectName: "setupWizard"
    width: Math.min(parent.width - 64,920); height: Math.min(parent.height - 48,650)
    modal: true; closePolicy: Popup.NoAutoClose
    visible: !library.setupComplete
    title: headings[library.wizardStep]
    readonly property var headings: ["Willkommen bei VELUNO", "Profil anpassen", "Medienordner auswählen", "Online-Metadaten", "Bibliotheksscan", "Bereit für deine Mediathek"]
    ColumnLayout {
        anchors.fill: parent; spacing: 16
        Label { text: "Schritt " + (library.wizardStep + 1) + " von 6"; color: theme.accent }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true
            contentWidth: availableWidth
            ColumnLayout {
                width: parent.width; spacing: 16
                Label { visible: library.wizardStep === 0; text: "Deine lokale Film- und Serienmediathek. VELUNO liest deine Medien an ihrem bisherigen Speicherort.\n\nSprache: Deutsch"; wrapMode: Text.WordWrap; color: wizard.theme.text; Layout.fillWidth: true }
                ColumnLayout {
                    visible: library.wizardStep === 1; Layout.fillWidth: true
                    Label { text: "Name des Standardprofils"; color: wizard.theme.text }
                    TextField { id: profileName; text: library.profileName; Layout.fillWidth: true; Accessible.name: "Profilname" }
                    Label { text: "Du kannst diesen Schritt mit dem vorhandenen Standardprofil überspringen."; color: wizard.theme.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
                FolderPanel { visible: library.wizardStep === 2; Layout.fillWidth: true; theme: wizard.theme; library: wizard.library }
                ColumnLayout {
                    visible: library.wizardStep === 3; Layout.fillWidth: true
                    Label { text: "Möchtest du Online-Metadaten grundsätzlich erlauben? Dieser Build ruft keine Online-Metadaten ab. Deine Auswahl wird gespeichert."; color: wizard.theme.text; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    RadioButton { objectName: "offlineChoice"; text: "Offline verwenden"; checked: library.onlineConsent === "denied"; onClicked: library.setOnlineConsent("denied") }
                    RadioButton { text: "Online-Metadaten grundsätzlich erlauben"; checked: library.onlineConsent === "granted"; onClicked: library.setOnlineConsent("granted") }
                }
                ScanPanel { visible: library.wizardStep === 4; Layout.fillWidth: true; theme: wizard.theme; library: wizard.library }
                Label { visible: library.wizardStep === 4; text: "Du kannst weitergehen, während der Scan im Hintergrund läuft."; color: wizard.theme.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Label { visible: library.wizardStep === 5; text: "Die Einrichtung ist abgeschlossen. Bereits erkannte Medien sind auf der Startseite sichtbar. Medienordner und die Metadatenentscheidung findest du später in den Einstellungen."; color: wizard.theme.text; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            }
        }
        RowLayout {
            Button { text: "Zurück"; enabled: library.wizardStep > 0; onClicked: library.setWizardStep(library.wizardStep - 1) }
            Item { Layout.fillWidth: true }
            Button {
                objectName: "wizardNext"
                text: library.wizardStep === 5 ? "Zur Startseite" : "Weiter"
                enabled: library.wizardStep !== 3 || library.onlineConsent !== "undecided"
                onClicked: {
                    if (library.wizardStep === 1 && profileName.text.trim().length > 0) library.setProfileName(profileName.text)
                    if (library.wizardStep === 5) library.finishSetup()
                    else { library.setWizardStep(library.wizardStep + 1); if (library.wizardStep === 4 && !library.busy) library.scan() }
                }
            }
        }
    }
}
