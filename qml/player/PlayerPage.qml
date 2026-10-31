import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Veluno.Player 1.0

Item {
    id: page
    objectName: "playerPage"
    required property QtObject personal
    required property QtObject player
    required property QtObject theme
    property bool controlsVisible: true
    property bool fullscreen: false
    property bool canChooseVersion: false
    signal versionsRequested
    signal closeRequested
    signal openRequested
    signal fullscreenRequested
    function revealControls() {
        controlsVisible = true;
        hideControls.restart();
    }
    function timestamp(value) {
        let seconds = Math.max(0, Math.floor(value));
        let minutes = Math.floor(seconds / 60);
        return Math.floor(minutes / 60) > 0 ? Math.floor(minutes / 60) + ":" + ("0" + minutes % 60).slice(-2) + ":" + ("0" + seconds % 60).slice(-2) : minutes + ":" + ("0" + seconds % 60).slice(-2);
    }
    onVisibleChanged: if (visible)
        revealControls()
    Rectangle {
        anchors.fill: parent
        color: "black"
    }
    VideoView {
        id: video
        objectName: "videoView"
        anchors.fill: parent
        player: page.player
    }
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onEntered: page.revealControls()
        onPositionChanged: page.revealControls()
        onClicked: page.revealControls()
        onDoubleClicked: page.fullscreenRequested()
    }
    Timer {
        id: hideControls
        interval: 2000
        onTriggered: {
            if (!player.paused && !panelHover.hovered && !headerHover.hovered && !diagnostics.opened)
                page.controlsVisible = false;
            else
                restart();
        }
    }
    readonly property bool overlayVisible: controlsVisible || player.paused || player.loading || player.ended || player.error.length > 0
    Rectangle {
        id: topPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: headerContent.implicitHeight + 24
        color: "#E6111E34"
        visible: page.overlayVisible
        HoverHandler {
            id: headerHover
        }
        ColumnLayout {
            id: headerContent
            anchors.fill: parent
            anchors.margins: 12
            RowLayout {
                Layout.fillWidth: true
                Button {
                    text: qsTr("Zurück")
                    onClicked: page.closeRequested()
                    Accessible.name: qsTr("Player schließen")
                }
                Label {
                    text: player.title
                    textFormat: Text.PlainText
                    color: "#EFF5FF"
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                    font.pixelSize: 16
                }
                Button {
                    text: qsTr("Datei öffnen")
                    onClicked: page.openRequested()
                }
            }
            Flow {
                Layout.fillWidth: true
                spacing: 6
                Button {
                    text: qsTr("Spuren")
                    onClicked: tracks.open()
                }
                Button {
                    objectName: "playbackDiagnosticsButton"
                    text: qsTr("Wiedergabedetails")
                    onClicked: diagnostics.open()
                }
                Button {
                    text: qsTr("Abspann markieren")
                    visible: personal.hasNext
                    onClicked: personal.markCredits()
                }
                Button {
                    text: qsTr("Nächste Folge")
                    visible: personal.hasNext
                    onClicked: personal.playNext()
                }
            }
        }
    }
    Dialog {
        id: diagnostics
        objectName: "playbackDiagnostics"
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 620)
        height: Math.min(parent.height - 48, 440)
        modal: true
        title: qsTr("Wiedergabedetails")
        footer: DialogButtonBox {
            Button {
                objectName: "playbackDiagnosticsClose"
                text: qsTr("Schließen")
                onClicked: diagnostics.close()
            }
        }
        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth
            ColumnLayout {
                width: parent.width
                spacing: 12
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Diese Angaben meldet der laufende Player. Nicht gemeldete Werte bestätigen keine Hardwarebeschleunigung.")
                    color: page.theme.muted
                    wrapMode: Text.WordWrap
                }
                Repeater {
                    model: [
                        {
                            key: "rendering",
                            label: qsTr("Bildausgabe")
                        },
                        {
                            key: "hwdec-current",
                            label: qsTr("Hardwaredecoder")
                        },
                        {
                            key: "video-codec",
                            label: qsTr("Videocodec")
                        },
                        {
                            key: "video-format",
                            label: qsTr("Videoformat")
                        },
                        {
                            key: "audio-codec-name",
                            label: qsTr("Audiocodec")
                        },
                        {
                            key: "audio-device",
                            label: qsTr("Audioausgabe")
                        }
                    ]
                    Label {
                        required property var modelData
                        Layout.fillWidth: true
                        text: modelData.label + ": " + (modelData.key === "hwdec-current" && page.player.diagnostics[modelData.key] === "no" ? qsTr("Software-Decodierung") : page.player.diagnostics[modelData.key] || qsTr("Nicht gemeldet"))
                        textFormat: Text.PlainText
                        color: page.theme.text
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
        onOpened: hideControls.stop()
        onClosed: page.revealControls()
    }

    Rectangle {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 145
        width: 340
        height: 120
        radius: 8
        color: theme.surface
        visible: personal.nextCountdown >= 0
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            Label {
                text: qsTr("Nächste Folge in ") + personal.nextCountdown + " Sekunden"
                color: theme.text
            }
            RowLayout {
                Button {
                    text: qsTr("Abbrechen")
                    onClicked: personal.cancelNext()
                }
                Button {
                    text: qsTr("Jetzt abspielen")
                    onClicked: personal.playNext()
                }
            }
        }
    }
    BusyIndicator {
        anchors.centerIn: parent
        running: player.loading
        visible: running
    }
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 640)
        visible: player.error.length > 0
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: errorText.implicitHeight + 48
            radius: 12
            color: theme.surface
            border.color: theme.border
            Label {
                id: errorText
                anchors.fill: parent
                anchors.margins: 24
                text: player.error
                color: theme.text
                wrapMode: Text.WordWrap
            }
        }
        Button {
            text: qsTr("Andere Filmversion wählen")
            visible: page.canChooseVersion
            onClicked: page.versionsRequested()
            Layout.alignment: Qt.AlignHCenter
        }
        Button {
            text: qsTr("Andere Datei auswählen")
            onClicked: page.openRequested()
            Layout.alignment: Qt.AlignHCenter
        }
    }
    Popup {
        id: tracks
        anchors.centerIn: parent
        width: 520
        height: Math.min(parent.height - 80, 460)
        modal: true
        ColumnLayout {
            anchors.fill: parent
            Label {
                text: qsTr("Audiospuren")
                color: theme.text
            }
            Repeater {
                model: player.audioTracks
                Button {
                    required property var modelData
                    text: (modelData.lang || "") + " · " + (modelData.title || "Spur " + modelData.id)
                    onClicked: player.selectAudio(modelData.id)
                }
            }
            Label {
                text: qsTr("Untertitel")
                color: theme.text
            }
            Button {
                text: qsTr("Untertitel aus")
                onClicked: player.selectSubtitle(0)
            }
            Repeater {
                model: player.subtitleTracks
                Button {
                    required property var modelData
                    text: (modelData.lang || "") + " · " + (modelData.title || "Spur " + modelData.id)
                    onClicked: player.selectSubtitle(modelData.id)
                }
            }
            Button {
                text: qsTr("Externe Untertitel öffnen")
                onClicked: externalSubtitles.open()
            }
        }
    }
    FileDialog {
        id: externalSubtitles
        nameFilters: ["Untertitel (*.srt *.ass *.ssa *.vtt *.sub)", "Alle Dateien (*)"]
        onAccepted: player.loadSubtitle(selectedFile)
    }
    Rectangle {
        id: panel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 126
        color: "#E6111E34"
        visible: page.overlayVisible
        HoverHandler {
            id: panelHover
        }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: page.timestamp(player.position)
                    color: "#EFF5FF"
                    Layout.preferredWidth: 65
                }
                Slider {
                    id: timeline
                    objectName: "timeline"
                    Layout.fillWidth: true
                    from: 0
                    to: Math.max(1, player.duration)
                    Binding {
                        target: timeline
                        property: "value"
                        value: player.position
                        when: !timeline.pressed
                        restoreMode: Binding.RestoreBindingOrValue
                    }
                    enabled: player.active && !player.loading && player.duration > 0
                    onMoved: {
                        player.seek(value);
                        page.revealControls();
                    }
                    Accessible.name: qsTr("Wiedergabeposition")
                }
                Label {
                    text: page.timestamp(player.duration)
                    color: "#EFF5FF"
                    Layout.preferredWidth: 65
                    horizontalAlignment: Text.AlignRight
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Button {
                    text: qsTr("−10 s")
                    enabled: player.active
                    onClicked: {
                        player.seekRelative(-10);
                        page.revealControls();
                    }
                }
                Button {
                    objectName: "pauseButton"
                    text: player.paused ? qsTr("Fortsetzen") : qsTr("Pause")
                    enabled: player.active && !player.loading
                    onClicked: {
                        player.togglePause();
                        page.revealControls();
                    }
                }
                Button {
                    text: qsTr("+10 s")
                    enabled: player.active
                    onClicked: {
                        player.seekRelative(10);
                        page.revealControls();
                    }
                }
                Button {
                    text: player.muted ? qsTr("Ton an") : qsTr("Stumm")
                    onClicked: player.muted = !player.muted
                }
                Slider {
                    objectName: "volumeSlider"
                    from: 0
                    to: 100
                    value: player.volume
                    Layout.preferredWidth: 130
                    onMoved: player.volume = value
                    Accessible.name: qsTr("Lautstärke")
                }
                Label {
                    text: Math.round(player.volume) + " %"
                    color: "#EFF5FF"
                    Layout.preferredWidth: 48
                }
                Item {
                    Layout.fillWidth: true
                }
                ComboBox {
                    model: [qsTr("0,5×"), qsTr("0,75×"), qsTr("1×"), qsTr("1,25×"), qsTr("1,5×"), qsTr("2×")]
                    readonly property var speeds: [0.5, 0.75, 1, 1.25, 1.5, 2]
                    currentIndex: speeds.indexOf(player.speed)
                    onActivated: player.speed = speeds[currentIndex]
                    Accessible.name: qsTr("Wiedergabegeschwindigkeit")
                }
                Button {
                    text: page.fullscreen ? qsTr("Fenster") : qsTr("Vollbild")
                    onClicked: page.fullscreenRequested()
                }
            }
        }
    }
    Connections {
        target: player
        function onStateChanged() {
            if (player.paused || player.loading)
                page.revealControls();
        }
    }
}
