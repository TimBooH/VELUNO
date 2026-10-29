import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "themes"
import "components"
import "pages"
ApplicationWindow {
    id: window
    objectName: "mainWindow"
    width: 1240; height: 800
    minimumWidth: 980; minimumHeight: 600
    visible: true
    title: "VELUNO"
    color: colors.background
    property string currentPage: "Startseite"
    readonly property var navigation: ["Startseite", "Filme", "Serien", "Favoriten", "Sammlungen"]
    Theme { id: colors; light: appSettings.theme === "light" }
    font.family: "Sans Serif"
    font.pixelSize: 14
    palette.window: colors.background
    palette.windowText: colors.text
    palette.base: colors.surface
    palette.alternateBase: colors.raised
    palette.text: colors.text
    palette.button: colors.raised
    palette.buttonText: colors.text
    palette.highlight: colors.accent
    palette.highlightedText: colors.onAccent
    palette.mid: colors.border
    palette.dark: colors.border
    palette.light: colors.raised
    palette.placeholderText: colors.muted
    header: Rectangle {
        color: colors.background; height: 88
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: colors.border }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 28; anchors.rightMargin: 28
            spacing: 6
            Label { text: "VELUNO"; font.pixelSize: 22; font.weight: Font.Bold; font.letterSpacing: 2; color: colors.accent; Layout.rightMargin: 20 }
            Repeater {
                model: window.navigation
                NavButton {
                    required property string modelData
                    theme: colors
                    text: modelData
                    selected: currentPage === modelData
                    onClicked: currentPage = modelData
                }
            }
            Item { Layout.fillWidth: true }
            TextField {
                Layout.preferredWidth: Math.max(140, Math.min(220, window.width - 860))
                placeholderText: "Filme und Serien suchen"
                enabled: false
                Accessible.name: "Suche – noch nicht verfügbar"
                ToolTip.visible: hovered
                ToolTip.text: "Die lokale Suche folgt mit der Bibliothek."
            }
            Button {
                id: profileButton
                objectName: "profileButton"
                text: "S  ▾"
                Accessible.name: "Profilmenü: Standardprofil"
                onClicked: profileMenu.open()
                Menu {
                    id: profileMenu
                    objectName: "profileMenu"
                    y: profileButton.height
                    MenuItem { text: "Standardprofil"; enabled: false }
                    MenuSeparator {}
                    MenuItem { objectName: "settingsMenuItem"; text: "Einstellungen"; onTriggered: currentPage = "Einstellungen" }
                }
            }
        }
    }
    LibraryPage {
        anchors.fill: parent; anchors.margins: 40
        theme: colors
        visible: currentPage !== "Einstellungen"
        pageTitle: currentPage
        description: {
            switch (currentPage) {
            case "Filme": return "Deine persönliche Filmsammlung."
            case "Serien": return "Deine Serien im Überblick."
            case "Favoriten": return "Deine Lieblingsfilme und -serien."
            case "Sammlungen": return "Platz für deine persönlichen Sammlungen."
            default: return "Deine Filme und Serien. An einem Ort."
            }
        }
    }
    SettingsPage {
        anchors.fill: parent; anchors.margins: 40
        visible: currentPage === "Einstellungen"
        theme: colors; settings: appSettings
    }
    footer: Label {
        text: "VELUNO 0.1.0-dev · Technische Grundlage"
        color: colors.muted; leftPadding: 28; bottomPadding: 16; font.pixelSize: 12
    }
}
