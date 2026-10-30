import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
Item {
    id: settingsPage
    required property QtObject library
    required property QtObject theme
    required property QtObject settings
    property int section: 0
    readonly property var sections: ["Allgemein", "Bibliothek", "Player", "Profile", "Datensicherung", "Updates", "Über VELUNO"]
    ColumnLayout {
        anchors.fill: parent; spacing: 24
        Label { text: "Einstellungen"; color: theme.text; font.pixelSize: 30; font.weight: Font.DemiBold }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 32
            ColumnLayout {
                objectName: "settingsNavigation"
                Layout.alignment: Qt.AlignTop
                Layout.minimumWidth: 180; Layout.maximumWidth: 180; Layout.preferredWidth: 180
                Repeater {
                    model: settingsPage.sections
                    NavButton {
                        required property int index
                        required property string modelData
                        text: modelData; theme: settingsPage.theme; selected: section === index
                        Layout.fillWidth: true
                        onClicked: section = index
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
                Label { text: settingsPage.sections[section]; color: theme.text; font.pixelSize: 22 }
                ColumnLayout {
                    visible: section === 0
                    spacing: 12
                    Label { text: "Erscheinungsbild"; color: theme.muted }
                    ComboBox {
                        objectName: "themeSelector"
                        model: ["Midnight Blue", "Light Mode"]
                        currentIndex: settings.theme === "light" ? 1 : 0
                        onActivated: settings.theme = currentIndex === 1 ? "light" : "midnight"
                        Accessible.name: "Erscheinungsbild"
                        Layout.preferredWidth: 240
                    }
                    Label { text: "Das Design wird automatisch gespeichert."; color: theme.muted }
                }
                ScrollView {
                    visible: section === 1
                    Layout.fillWidth: true; Layout.fillHeight: true
                    contentWidth: availableWidth
                    ColumnLayout {
                        width: parent.width; spacing: 18
                        FolderPanel { Layout.fillWidth: true; theme: settingsPage.theme; library: settingsPage.library }
                        CheckBox { text: "Medienordner während des Betriebs überwachen"; checked: library.monitoring; onToggled: library.monitoring = checked }
                        Label { text: "Online-Metadaten"; color: settingsPage.theme.text; font.weight: Font.DemiBold }
                        ComboBox {
                            model: ["Offline verwenden", "Online-Metadaten grundsätzlich erlauben"]
                            currentIndex: library.onlineConsent === "undecided" ? -1 : library.onlineConsent === "granted" ? 1 : 0
                            displayText: currentIndex < 0 ? "Bitte bewusst wählen" : currentText
                            onActivated: library.setOnlineConsent(currentIndex === 1 ? "granted" : "denied")
                            Accessible.name: "Metadatenfreigabe"
                        }
                        Label { text: "Dieser Build ruft keine Online-Metadaten ab."; color: settingsPage.theme.muted }
                        ScanPanel { Layout.fillWidth: true; theme: settingsPage.theme; library: settingsPage.library }
                    }
                }
                Label {
                    visible: section > 1 && section < 6
                    text: "Dieser Bereich wird in einem späteren Entwicklungsschritt umgesetzt."
                    color: theme.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
                Label {
                    visible: section === 6
                    text: "VELUNO · 0.1.0-dev\nNative Film- und Serienmediathek für Linux\nVELUNO ist ein vorläufiger Arbeitsname."
                    color: theme.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
                Label {
                    visible: settings.error.length > 0
                    text: "Design konnte nicht gespeichert werden: " + settings.error
                    color: theme.accent; wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
                Item { Layout.fillHeight: true }
            }
        }
    }
}
