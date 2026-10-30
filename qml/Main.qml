import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "themes"
import "components"
import "pages"
import "player"
import "components"
import "wizard"
ApplicationWindow {
    id: window
    objectName: "mainWindow"
    width: 1240; height: 800
    minimumWidth: 980; minimumHeight: 600
    visible: true
    title: "VELUNO"
    color: colors.background
    readonly property QtObject mediaLibrary: library
    property var playingTitleId: 0
    property bool playerVisible: false
    function closePlayer() {
        window.mediaLibrary.cancelPendingPlayback()
        playback.stop()
        playerVisible = false
        if (visibility === Window.FullScreen) showNormal()
    }
    function toggleFullscreen() {
        if (visibility === Window.FullScreen) showNormal()
        else showFullScreen()
    }
    FileDialog {
        id: videoFileDialog
        title: "Videodatei öffnen"
        nameFilters: ["Videodateien (*.mkv *.mp4 *.m4v *.avi *.mov *.webm *.mpeg *.mpg *.ts *.m2ts *.wmv *.ogv)", "Alle Dateien (*)"]
        onAccepted: { window.mediaLibrary.cancelPendingPlayback(); window.playingTitleId = 0; if (selectedFile.toString().toLowerCase().indexOf("smb:") === 0) window.mediaLibrary.playFile(selectedFile.toString()); else { window.playerVisible = true; playback.openFile(selectedFile) } }
    }
    Shortcut { sequence: "Ctrl+O"; onActivated: videoFileDialog.open() }
    Shortcut { sequence: "Space"; enabled: window.playerVisible; onActivated: playback.togglePause() }
    Shortcut { sequence: "Left"; enabled: window.playerVisible; onActivated: playback.seekRelative(-10) }
    Shortcut { sequence: "Right"; enabled: window.playerVisible; onActivated: playback.seekRelative(10) }
    Shortcut { sequence: "Up"; enabled: window.playerVisible; onActivated: playback.volume += 5 }
    Shortcut { sequence: "Down"; enabled: window.playerVisible; onActivated: playback.volume -= 5 }
    Shortcut { sequence: "F"; enabled: window.playerVisible; onActivated: window.toggleFullscreen() }
    Shortcut { sequence: "Escape"; enabled: window.playerVisible; onActivated: { if (window.visibility === Window.FullScreen) window.showNormal() } }
    Shortcut { sequence: "M"; enabled: window.playerVisible; onActivated: playback.muted = !playback.muted }
    Shortcut { sequence: "S"; enabled: window.playerVisible; onActivated: playback.cycleSubtitles() }
    Shortcut { sequence: "A"; enabled: window.playerVisible; onActivated: playback.cycleAudio() }
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
        color: colors.background; height: visible ? 88 : 0
        visible: window.visibility !== Window.FullScreen
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
                    onClicked: { window.closePlayer(); currentPage = modelData }
                }
            }
            Item { Layout.fillWidth: true }
            TextField {
                Layout.preferredWidth: Math.max(140, Math.min(220, window.width - 860))
                placeholderText: "Filme und Serien suchen"
                id: globalSearch
                objectName: "globalSearch"
                property var matches: []
                Accessible.name: "Filme und Serien durchsuchen"
                function refresh() {
                    matches = window.mediaLibrary.search(text)
                    if (activeFocus && text.trim().length > 0 && matches.length > 0) searchPopup.open()
                    else if (text.trim().length === 0 || matches.length === 0) searchPopup.close()
                }
                function activateResult(mediaId) {
                    searchPopup.close()
                    window.mediaLibrary.openDetails(mediaId)
                    text = ""
                }
                onTextChanged: refresh()
                onActiveFocusChanged: {
                    if (activeFocus) refresh()
                    else Qt.callLater(function() { if (!searchPopup.activeFocus) searchPopup.close() })
                }
                Keys.onDownPressed: searchResults.currentIndex = Math.min(searchResults.currentIndex+1,matches.length-1)
                Keys.onUpPressed: searchResults.currentIndex = Math.max(0,searchResults.currentIndex-1)
                Keys.onReturnPressed: {
                    if (matches.length) activateResult(matches[Math.max(0,searchResults.currentIndex)].id)
                }
                Connections { target: window.mediaLibrary; function onCatalogChanged() { globalSearch.refresh() } }
                Popup {
                    id: searchPopup
                    objectName: "searchPopup"
                    x: 0; y: globalSearch.height + 4; width: 320
                    height: Math.min(330,searchResults.contentHeight+16)
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                    ListView {
                        id: searchResults; objectName: "searchResults"; anchors.fill: parent; clip: true
                        model: globalSearch.matches; currentIndex: -1
                        ScrollBar.vertical: ScrollBar {}
                        delegate: ItemDelegate {
                            required property var modelData
                            width: ListView.view.width
                            text: modelData.title + (modelData.year ? " (" + modelData.year + ")" : "") + (modelData.kind === "series" ? " · Serie" : " · Film")
                            highlighted: ListView.isCurrentItem
                            onClicked: globalSearch.activateResult(modelData.id)
                        }
                    }
                }
            }
            Button {
                id: profileButton
                objectName: "profileButton"
                text: window.mediaLibrary.profileName.slice(0,1).toUpperCase() + "  ▾"
                Accessible.name: "Profilmenü: " + window.mediaLibrary.profileName
                onClicked: profileMenu.open()
                Menu {
                    id: profileMenu
                    objectName: "profileMenu"
                    y: profileButton.height
                    MenuItem { text: window.mediaLibrary.profileName; enabled: false }
                    MenuItem { text: "Datei abspielen…"; onTriggered: videoFileDialog.open() }
                    MenuSeparator {}
                    MenuItem { objectName: "settingsMenuItem"; text: "Einstellungen"; onTriggered: { window.closePlayer(); currentPage = "Einstellungen" } }
                }
            }
        }
    }
    LibraryPage {
        anchors.fill: parent; anchors.margins: 40
        theme: colors
        library: window.mediaLibrary
        visible: currentPage !== "Einstellungen" && !window.playerVisible
        onOpenFileRequested: videoFileDialog.open()
        onFoldersRequested: { window.closePlayer(); currentPage = "Einstellungen"; settingsPage.section = 1 }
        onAssignRequested: function(entry) { libraryDialogs.assign(entry) }
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
        id: settingsPage
        library: window.mediaLibrary
        anchors.fill: parent; anchors.margins: 40
        visible: currentPage === "Einstellungen" && !window.playerVisible
        theme: colors; settings: appSettings
    }
    PlayerPage {
        anchors.fill: parent
        visible: window.playerVisible
        player: playback; theme: colors
        fullscreen: window.visibility === Window.FullScreen
        canChooseVersion: window.playingTitleId > 0 && window.mediaLibrary.selectedTitle.kind === "movie" && window.mediaLibrary.selectedTitle.file_count > 1
        onVersionsRequested: { window.closePlayer(); window.mediaLibrary.openDetails(window.playingTitleId) }
        onCloseRequested: window.closePlayer()
        onOpenRequested: videoFileDialog.open()
        onFullscreenRequested: window.toggleFullscreen()
    }
    LibraryDialogs { id: libraryDialogs; theme: colors; library: window.mediaLibrary }
    SetupWizard { theme: colors; library: window.mediaLibrary }
    Connections {
        target: window.mediaLibrary
        function onPlayRequested(url, network, titleId) { window.playingTitleId = titleId; window.playerVisible = true; playback.openFile(url, network) }
    }
    Dialog {
        parent: Overlay.overlay; anchors.centerIn: parent
        width: Math.min(parent.width-64,560)
        title: "VELUNO"; modal: true; standardButtons: Dialog.Ok
        visible: window.mediaLibrary.error.length > 0
        Label { width: parent.width; text: window.mediaLibrary.error; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: colors.text }
        onClosed: window.mediaLibrary.clearError()
    }
    onClosing: { window.mediaLibrary.cancelPendingPlayback(); playback.stop() }
    footer: Label {
        visible: !window.playerVisible && window.visibility !== Window.FullScreen
        text: window.mediaLibrary.networkBusy ? "SMB-Datei wird verbunden …" : window.mediaLibrary.busy ? (window.mediaLibrary.paused ? "Bibliotheksscan pausiert" : "Bibliotheksscan · " + window.mediaLibrary.checked + " Dateien geprüft") : window.mediaLibrary.status
        color: colors.muted; leftPadding: 28; bottomPadding: 16; font.pixelSize: 12
    }
}
