import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: settingsPage
    objectName: "settingsPage"
    required property QtObject updates
    required property QtObject metadata
    required property QtObject player
    required property QtObject playerSettings
    required property QtObject backups
    required property QtObject personal
    required property QtObject library
    required property QtObject theme
    required property QtObject settings
    property int section: 0
    readonly property var sections: ["Allgemein", "Bibliothek", "Player", "Profile", "Datensicherung", "Updates", "Über VELUNO"]
    ColumnLayout {
        anchors.fill: parent
        spacing: 24
        Label {
            text: qsTr("Einstellungen")
            color: theme.text
            font.pixelSize: 30
            font.weight: Font.DemiBold
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 32
            ColumnLayout {
                objectName: "settingsNavigation"
                Layout.alignment: Qt.AlignTop
                Layout.minimumWidth: 180
                Layout.maximumWidth: 180
                Layout.preferredWidth: 180
                Repeater {
                    model: settingsPage.sections
                    NavButton {
                        required property int index
                        required property string modelData
                        text: qsTr(modelData)
                        theme: settingsPage.theme
                        selected: section === index
                        Layout.fillWidth: true
                        onClicked: section = index
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 16
                Label {
                    objectName: "settingsSectionTitle"
                    text: qsTr(settingsPage.sections[section])
                    color: theme.text
                    font.pixelSize: 22
                }
                ColumnLayout {
                    visible: section === 0
                    spacing: 12
                    ComboBox {
                        model: ["Deutsch", "English"]
                        currentIndex: localization.language === "en" ? 1 : 0
                        onActivated: localization.language = currentIndex ? "en" : "de"
                        Accessible.name: "Sprache / Language"
                    }
                    Label {
                        text: qsTr("Erscheinungsbild")
                        color: theme.muted
                    }
                    ComboBox {
                        objectName: "themeSelector"
                        model: [qsTr("Midnight Blue"), qsTr("Light Mode")]
                        currentIndex: settings.theme === "light" ? 1 : 0
                        onActivated: settings.theme = currentIndex === 1 ? "light" : "midnight"
                        Accessible.name: qsTr("Erscheinungsbild")
                        Layout.preferredWidth: 240
                    }
                    Label {
                        text: qsTr("Das Design wird automatisch gespeichert.")
                        color: theme.muted
                    }
                }
                ScrollView {
                    visible: section === 1
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentWidth: availableWidth
                    ColumnLayout {
                        width: parent.width
                        spacing: 18
                        Label {
                            text: qsTr("TMDB-Metadaten")
                            color: settingsPage.theme.text
                            font.weight: Font.DemiBold
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Beim Einschalten werden Film- und Serientitel an TMDB gesendet, um Beschreibungen, Bewertungen, Poster und Hintergründe abzurufen. Lokale Medien bleiben unverändert.")
                            color: settingsPage.theme.muted
                            wrapMode: Text.WordWrap
                        }
                        Switch {
                            objectName: "tmdbEnabledSwitch"
                            text: qsTr("TMDB-Metadaten")
                            checked: metadata.enabled
                            onToggled: metadata.enabled = checked
                            Accessible.name: qsTr("TMDB-Metadaten ein- oder ausschalten")
                        }
                        Label {
                            objectName: "tmdbState"
                            Layout.fillWidth: true
                            text: !metadata.enabled ? qsTr("Aus · Gespeicherte Metadaten und Cover bleiben offline verfügbar.") : metadata.configured ? qsTr("Ein · Automatischer Abruf ist aktiviert.") : qsTr("Ein · Für den Abruf bitte noch den TMDB-Zugang hinterlegen.")
                            color: metadata.enabled ? settingsPage.theme.accent : settingsPage.theme.muted
                            wrapMode: Text.WordWrap
                        }
                        Label {
                            text: metadata.configured ? qsTr("TMDB-Zugang ist hinterlegt") : qsTr("Noch kein TMDB-Zugang hinterlegt")
                            color: settingsPage.theme.muted
                        }
                        TextField {
                            id: tmdbToken
                            objectName: "tmdbToken"
                            placeholderText: qsTr("TMDB API-Lesezugriffstoken (Bearer)")
                            echoMode: TextInput.Password
                            Layout.fillWidth: true
                            Accessible.name: qsTr("TMDB API-Zugang")
                        }
                        Flow {
                            Layout.fillWidth: true
                            spacing: 6
                            Button {
                                objectName: "tmdbSaveToken"
                                text: qsTr("Zugang speichern")
                                onClicked: {
                                    metadata.setToken(tmdbToken.text);
                                    tmdbToken.text = "";
                                }
                            }
                            Button {
                                objectName: "tmdbTestAccess"
                                opacity: enabled ? 1 : 0.4
                                text: qsTr("Zugang prüfen")
                                enabled: metadata.enabled && metadata.configured && !metadata.busy
                                onClicked: metadata.testAccess()
                            }
                            Button {
                                objectName: "tmdbRefresh"
                                opacity: enabled ? 1 : 0.4
                                text: qsTr("Metadaten aktualisieren")
                                enabled: metadata.enabled && metadata.configured && !metadata.busy
                                onClicked: metadata.refreshAll()
                            }
                        }
                        BusyIndicator {
                            running: metadata.busy
                            visible: running
                            Layout.preferredHeight: 32
                            Layout.preferredWidth: 32
                            Accessible.name: qsTr("Metadaten werden geladen")
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Bibliothek") + ": " + metadata.statistics.total + " · " + qsTr("TMDB zugeordnet") + ": " + metadata.statistics.matched + " · " + qsTr("Mit Poster") + ": " + metadata.statistics.posters + (metadata.busy ? " · " + qsTr("Ausstehend") + ": " + metadata.pendingCount : "")
                            color: settingsPage.theme.muted
                            wrapMode: Text.WordWrap
                        }
                        Button {
                            text: qsTr("Fehlende Bilder erneut laden")
                            opacity: enabled ? 1 : 0.4
                            enabled: metadata.enabled && metadata.configured && !metadata.busy
                            onClicked: metadata.retryMissingArtwork()
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Nicht zugeordnete Titel kannst du in den Titeldetails unter Informationen und Bilder bearbeiten gezielt bei TMDB suchen und auswählen.")
                            color: settingsPage.theme.muted
                            wrapMode: Text.WordWrap
                        }
                        Label {
                            text: metadata.status
                            color: settingsPage.theme.muted
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                        FolderPanel {
                            Layout.fillWidth: true
                            theme: settingsPage.theme
                            library: settingsPage.library
                        }
                        CheckBox {
                            text: qsTr("Medienordner während des Betriebs überwachen")
                            checked: library.monitoring
                            onToggled: library.monitoring = checked
                        }
                        ScanPanel {
                            Layout.fillWidth: true
                            theme: settingsPage.theme
                            library: settingsPage.library
                        }
                    }
                }
                PlayerSettingsPage {
                    visible: section === 2
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    theme: settingsPage.theme
                    player: settingsPage.player
                    preferences: settingsPage.playerSettings
                }
                UpdatesPage {
                    visible: section === 5
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    theme: settingsPage.theme
                    updates: settingsPage.updates
                    personal: settingsPage.personal
                    backups: settingsPage.backups
                }
                BackupPage {
                    visible: section === 4
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    theme: settingsPage.theme
                    backups: settingsPage.backups
                    personal: settingsPage.personal
                }
                ProfilesPage {
                    visible: section === 3
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    theme: settingsPage.theme
                    personal: settingsPage.personal
                }
                Label {
                    visible: section === 6
                    text: qsTr("VELUNO · 0.1.0-dev\nNative Film- und Serienmediathek für Linux\nVELUNO ist ein vorläufiger Arbeitsname.\n\nThis product uses the TMDB API but is not endorsed or certified by TMDB.\nMetadaten und Bilder: The Movie Database (TMDB).")
                    color: theme.muted
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Label {
                    visible: settings.error.length > 0
                    text: qsTr("Design konnte nicht gespeichert werden: ") + settings.error
                    color: theme.accent
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }
}
