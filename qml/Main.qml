import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "themes"
import "components"
import "pages"
import "player"
import "wizard"

ApplicationWindow {
    id: window
    objectName: "mainWindow"
    width: 1240
    height: 800
    minimumWidth: 980
    minimumHeight: 600
    visible: true
    title: qsTr("VELUNO")
    color: colors.background
    readonly property QtObject updateService: updates
    readonly property QtObject metadataService: metadata
    readonly property QtObject playerPreferences: playerSettings
    readonly property QtObject playbackController: playback
    readonly property QtObject backupData: backups
    readonly property QtObject personalData: personal
    readonly property QtObject mediaLibrary: library
    property var playingTitleId: 0
    property bool playerVisible: false
    function closePlayer() {
        window.mediaLibrary.cancelPendingPlayback();
        personal.saveProgress();
        playback.stop();
        playerVisible = false;
        if (visibility === Window.FullScreen)
            showNormal();
    }
    function toggleFullscreen() {
        if (visibility === Window.FullScreen)
            showNormal();
        else
            showFullScreen();
    }
    FileDialog {
        id: videoFileDialog
        title: qsTr("Videodatei öffnen")
        nameFilters: ["Videodateien (*.mkv *.mp4 *.m4v *.avi *.mov *.webm *.mpeg *.mpg *.ts *.m2ts *.wmv *.ogv)", "Alle Dateien (*)"]
        onAccepted: {
            personal.stopTracking();
            window.mediaLibrary.cancelPendingPlayback();
            window.playingTitleId = 0;
            if (selectedFile.toString().toLowerCase().indexOf("smb:") === 0)
                window.mediaLibrary.playFile(selectedFile.toString());
            else {
                window.playerVisible = true;
                playback.openFile(selectedFile);
            }
        }
    }
    Shortcut {
        sequence: playerSettings.shortcuts.next
        enabled: window.playerVisible
        onActivated: personal.playNext()
    }
    Shortcut {
        sequence: "Ctrl+O"
        onActivated: videoFileDialog.open()
    }
    Shortcut {
        sequence: playerSettings.shortcuts.pause
        enabled: window.playerVisible
        onActivated: playback.togglePause()
    }
    Shortcut {
        sequence: playerSettings.shortcuts.back
        enabled: window.playerVisible
        onActivated: playback.seekRelative(-10)
    }
    Shortcut {
        sequence: playerSettings.shortcuts.forward
        enabled: window.playerVisible
        onActivated: playback.seekRelative(10)
    }
    Shortcut {
        sequence: playerSettings.shortcuts.volume_up
        enabled: window.playerVisible
        onActivated: playback.volume += 5
    }
    Shortcut {
        sequence: playerSettings.shortcuts.volume_down
        enabled: window.playerVisible
        onActivated: playback.volume -= 5
    }
    Shortcut {
        sequence: playerSettings.shortcuts.fullscreen
        enabled: window.playerVisible
        onActivated: window.toggleFullscreen()
    }
    Shortcut {
        sequence: playerSettings.shortcuts.exit_fullscreen
        enabled: window.playerVisible
        onActivated: {
            if (window.visibility === Window.FullScreen)
                window.showNormal();
        }
    }
    Shortcut {
        sequence: playerSettings.shortcuts.mute
        enabled: window.playerVisible
        onActivated: playback.muted = !playback.muted
    }
    Shortcut {
        sequence: playerSettings.shortcuts.subtitles
        enabled: window.playerVisible
        onActivated: playback.cycleSubtitles()
    }
    Shortcut {
        sequence: playerSettings.shortcuts.audio
        enabled: window.playerVisible
        onActivated: playback.cycleAudio()
    }
    property string currentPage: "Startseite"
    readonly property var navigation: ["Startseite", "Filme", "Serien", "Favoriten", "Sammlungen"]
    Theme {
        id: colors
        light: appSettings.theme === "light"
    }
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
    palette.highlightedText: colors.accentText
    palette.mid: colors.border
    palette.dark: colors.border
    palette.light: colors.raised
    palette.placeholderText: colors.muted
    header: Rectangle {
        color: colors.background
        height: visible ? 80 : 0
        visible: window.visibility !== Window.FullScreen
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: colors.surface
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 40
            anchors.rightMargin: 40
            spacing: 6
            Label {
                text: qsTr("VELUNO")
                font.pixelSize: 24
                font.weight: Font.Bold
                font.letterSpacing: 2
                color: colors.accent
                Layout.rightMargin: 22
            }
            Repeater {
                model: window.navigation
                NavButton {
                    required property string modelData
                    theme: colors
                    text: qsTr(modelData)
                    selected: currentPage === modelData
                    onClicked: {
                        window.closePlayer();
                        currentPage = modelData;
                    }
                }
            }
            Item {
                Layout.fillWidth: true
            }
            TextField {
                id: globalSearch
                Layout.preferredWidth: Math.max(140, Math.min(220, window.width - 860))
                placeholderText: qsTr("Filme und Serien suchen")
                leftPadding: 14
                background: Rectangle {
                    radius: 18
                    color: globalSearch.activeFocus ? colors.raised : colors.surface
                    border.width: globalSearch.activeFocus ? 1 : 0
                    border.color: colors.accent
                }
                objectName: "globalSearch"
                property var matches: []
                Accessible.name: qsTr("Filme und Serien durchsuchen")
                function refresh() {
                    matches = window.mediaLibrary.search(text);
                    if (activeFocus && text.trim().length > 0 && matches.length > 0)
                        searchPopup.open();
                    else if (text.trim().length === 0 || matches.length === 0)
                        searchPopup.close();
                }
                function activateResult(mediaId) {
                    searchPopup.close();
                    window.mediaLibrary.openDetails(mediaId);
                    text = "";
                }
                onTextChanged: refresh()
                onActiveFocusChanged: {
                    if (activeFocus)
                        refresh();
                    else
                        Qt.callLater(function () {
                            if (!searchPopup.activeFocus)
                                searchPopup.close();
                        });
                }
                Keys.onDownPressed: searchResults.currentIndex = Math.min(searchResults.currentIndex + 1, matches.length - 1)
                Keys.onUpPressed: searchResults.currentIndex = Math.max(0, searchResults.currentIndex - 1)
                Keys.onReturnPressed: {
                    if (matches.length)
                        activateResult(matches[Math.max(0, searchResults.currentIndex)].id);
                }
                Connections {
                    target: window.mediaLibrary
                    function onCatalogChanged() {
                        globalSearch.refresh();
                    }
                }
                Popup {
                    id: searchPopup
                    objectName: "searchPopup"
                    x: 0
                    y: globalSearch.height + 4
                    width: 320
                    height: Math.min(330, searchResults.contentHeight + 16)
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                    ListView {
                        id: searchResults
                        objectName: "searchResults"
                        anchors.fill: parent
                        clip: true
                        model: globalSearch.matches
                        currentIndex: -1
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
                flat: true
                objectName: "profileButton"
                text: window.mediaLibrary.profileName.slice(0, 1).toUpperCase() + "  ▾"
                Accessible.name: qsTr("Profilmenü: ") + window.mediaLibrary.profileName
                contentItem: RowLayout {
                    Rectangle {
                        width: 30
                        height: 30
                        radius: 15
                        color: window.personalData.activeProfile.color || colors.accent
                        Image {
                            anchors.fill: parent
                            source: window.personalData.activeProfile.avatar_url || ""
                            fillMode: Image.PreserveAspectCrop
                            visible: !!window.personalData.activeProfile.avatar_url
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: !window.personalData.activeProfile.avatar_url
                            color: colors.accentText
                            text: window.personalData.activeProfile.avatar === "cinema" ? "▣" : window.personalData.activeProfile.avatar === "rocket" ? "↑" : window.personalData.activeProfile.avatar === "planet" ? "◉" : window.mediaLibrary.profileName.slice(0, 1).toUpperCase()
                        }
                    }
                    Label {
                        text: "▾"
                        color: colors.text
                    }
                }
                onClicked: profileMenu.open()
                Menu {
                    id: profileMenu
                    objectName: "profileMenu"
                    y: profileButton.height
                    MenuItem {
                        text: window.mediaLibrary.profileName
                        enabled: false
                    }
                    Menu {
                        id: profileSwitch
                        title: qsTr("Profil wechseln")
                        Instantiator {
                            model: personal.profiles
                            delegate: MenuItem {
                                required property var entry
                                text: entry.name
                                checkable: true
                                checked: entry.id === personal.activeProfile.id
                                onTriggered: {
                                    window.closePlayer();
                                    personal.switchProfile(entry.id);
                                }
                            }
                            onObjectAdded: function (index, object) {
                                profileSwitch.insertItem(index, object);
                            }
                            onObjectRemoved: function (index, object) {
                                profileSwitch.removeItem(object);
                            }
                        }
                    }
                    MenuItem {
                        text: qsTr("Datei abspielen…")
                        onTriggered: videoFileDialog.open()
                    }
                    MenuSeparator {}
                    MenuItem {
                        objectName: "settingsMenuItem"
                        text: qsTr("Einstellungen")
                        onTriggered: {
                            window.closePlayer();
                            currentPage = "Einstellungen";
                        }
                    }
                }
            }
        }
    }
    LibraryPage {
        personal: window.personalData
        metadata: window.metadataService
        anchors.fill: parent
        anchors.margins: 40
        theme: colors
        library: window.mediaLibrary
        visible: currentPage !== "Einstellungen" && currentPage !== "Sammlungen" && !window.playerVisible
        onOpenFileRequested: videoFileDialog.open()
        onFoldersRequested: {
            window.closePlayer();
            currentPage = "Einstellungen";
            settingsPage.section = 1;
        }
        onAssignRequested: function (entry) {
            libraryDialogs.assign(entry);
        }
        pageTitle: currentPage
        description: {
            switch (currentPage) {
            case "Filme":
                return "Deine persönliche Filmsammlung.";
            case "Serien":
                return "Deine Serien im Überblick.";
            case "Favoriten":
                return "Deine Lieblingsfilme und -serien.";
            case "Sammlungen":
                return "Platz für deine persönlichen Sammlungen.";
            default:
                return "Deine Filme und Serien. An einem Ort.";
            }
        }
    }
    SettingsPage {
        id: settingsPage
        updates: window.updateService
        metadata: window.metadataService
        player: window.playbackController
        playerSettings: window.playerPreferences
        backups: window.backupData
        personal: window.personalData
        library: window.mediaLibrary
        anchors.fill: parent
        anchors.margins: 40
        visible: currentPage === "Einstellungen" && !window.playerVisible
        theme: colors
        settings: appSettings
    }
    PlayerPage {
        anchors.fill: parent
        visible: window.playerVisible
        personal: window.personalData
        player: playback
        theme: colors
        fullscreen: window.visibility === Window.FullScreen
        canChooseVersion: window.playingTitleId > 0 && window.mediaLibrary.selectedTitle.kind === "movie" && window.mediaLibrary.selectedTitle.file_count > 1
        onVersionsRequested: {
            window.closePlayer();
            window.mediaLibrary.openDetails(window.playingTitleId);
        }
        onCloseRequested: window.closePlayer()
        onOpenRequested: videoFileDialog.open()
        onFullscreenRequested: window.toggleFullscreen()
    }
    CollectionsPage {
        anchors.fill: parent
        anchors.margins: 40
        visible: !window.playerVisible && window.currentPage === "Sammlungen"
        theme: colors
        personal: window.personalData
        library: window.mediaLibrary
    }
    LibraryDialogs {
        id: libraryDialogs
        metadata: window.metadataService
        personal: window.personalData
        theme: colors
        library: window.mediaLibrary
    }
    SetupWizard {
        personal: window.personalData
        theme: colors
        library: window.mediaLibrary
    }
    Connections {
        target: window.mediaLibrary
        function onPlayRequested(url, network, titleId) {
            window.playingTitleId = titleId;
            window.playerVisible = true;
            playback.openFile(url, network);
        }
    }
    Dialog {
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 560)
        title: qsTr("VELUNO")
        modal: true
        standardButtons: Dialog.Ok
        visible: window.mediaLibrary.error.length > 0
        Label {
            width: parent.width
            text: window.mediaLibrary.error
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            color: colors.text
        }
        onClosed: window.mediaLibrary.clearError()
    }
    Dialog {
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 560)
        height: 270
        title: qsTr("Wiederherstellung vorbereitet")
        visible: window.backupData.restartRequired
        modal: true
        closePolicy: Popup.NoAutoClose
        Label {
            width: parent.width
            text: qsTr("Die vollständige Wiederherstellung wird beim nächsten Programmstart aktiviert. Der bisherige Datenstand wurde vorsorglich gesichert.")
            wrapMode: Text.WordWrap
            color: colors.text
        }
        footer: DialogButtonBox {
            Button {
                text: qsTr("Jetzt schließen und VELUNO neu starten")
                onClicked: Qt.quit()
            }
        }
    }
    onClosing: {
        personal.saveProgress();
        window.mediaLibrary.cancelPendingPlayback();
        playback.stop();
    }
    footer: ItemDelegate {
        objectName: "scanStatusButton"
        height: visible ? 36 : 0
        onClicked: {
            window.currentPage = "Einstellungen";
            settingsPage.section = 1;
        }
        Accessible.name: qsTr("Scanstatus und Fehler anzeigen")
        visible: !window.playerVisible && window.visibility !== Window.FullScreen
        text: window.mediaLibrary.networkBusy ? "SMB-Datei wird verbunden …" : window.mediaLibrary.busy ? (window.mediaLibrary.paused ? "Bibliotheksscan pausiert" : "Bibliotheksscan · " + window.mediaLibrary.checked + " Dateien geprüft") : window.mediaLibrary.status
        leftPadding: 40
        rightPadding: 40
        font.pixelSize: 11
        contentItem: Label {
            text: parent.text + "  ›"
            color: colors.muted
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        background: Item {}
    }
}
