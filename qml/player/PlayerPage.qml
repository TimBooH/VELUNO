import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Veluno.Player 1.0
Item {
    id: page
    objectName: "playerPage"
    required property QtObject player
    required property QtObject theme
    property bool controlsVisible: true
    property bool fullscreen: false
    signal closeRequested()
    signal openRequested()
    signal fullscreenRequested()
    function revealControls() { controlsVisible = true; hideControls.restart() }
    function timestamp(value) {
        let seconds = Math.max(0, Math.floor(value))
        let minutes = Math.floor(seconds / 60)
        return Math.floor(minutes / 60) > 0
            ? Math.floor(minutes / 60) + ":" + ("0" + minutes % 60).slice(-2) + ":" + ("0" + seconds % 60).slice(-2)
            : minutes + ":" + ("0" + seconds % 60).slice(-2)
    }
    onVisibleChanged: if (visible) revealControls()
    Rectangle { anchors.fill: parent; color: "black" }
    VideoView { id: video; objectName: "videoView"; anchors.fill: parent; player: page.player }
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
        id: hideControls; interval: 2000
        onTriggered: {
            if (!player.paused && !panelHover.hovered && !headerHover.hovered) page.controlsVisible = false
            else restart()
        }
    }
    readonly property bool overlayVisible: controlsVisible || player.paused || player.loading || player.ended || player.error.length > 0
    Rectangle {
        id: topPanel
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        height: 64; color: "#E6111E34"; visible: page.overlayVisible
        HoverHandler { id: headerHover }
        RowLayout {
            anchors.fill: parent; anchors.margins: 12
            Button { text: "Zurück"; onClicked: page.closeRequested(); Accessible.name: "Player schließen" }
            Label { text: player.title; color: "#EFF5FF"; elide: Text.ElideMiddle; Layout.fillWidth: true; font.pixelSize: 16 }
            Button { text: "Datei öffnen"; onClicked: page.openRequested() }
        }
    }
    BusyIndicator { anchors.centerIn: parent; running: player.loading; visible: running }
    ColumnLayout {
        anchors.centerIn: parent; width: Math.min(parent.width - 64, 640)
        visible: player.error.length > 0
        Rectangle {
            Layout.fillWidth: true; implicitHeight: errorText.implicitHeight + 48
            radius: 12; color: theme.surface; border.color: theme.border
            Label {
                id: errorText; anchors.fill: parent; anchors.margins: 24
                text: player.error; color: theme.text; wrapMode: Text.WordWrap
            }
        }
        Button { text: "Andere Datei auswählen"; onClicked: page.openRequested(); Layout.alignment: Qt.AlignHCenter }
    }
    Rectangle {
        id: panel
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 126; color: "#E6111E34"; visible: page.overlayVisible
        HoverHandler { id: panelHover }
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 16; spacing: 12
            RowLayout {
                Layout.fillWidth: true
                Label { text: page.timestamp(player.position); color: "#EFF5FF"; Layout.preferredWidth: 65 }
                Slider {
                    id: timeline; objectName: "timeline"
                    Layout.fillWidth: true; from: 0; to: Math.max(1, player.duration)
                    Binding { target: timeline; property: "value"; value: player.position; when: !timeline.pressed; restoreMode: Binding.RestoreBindingOrValue }
                    enabled: player.active && !player.loading && player.duration > 0
                    onMoved: { player.seek(value); page.revealControls() }
                    Accessible.name: "Wiedergabeposition"
                }
                Label { text: page.timestamp(player.duration); color: "#EFF5FF"; Layout.preferredWidth: 65; horizontalAlignment: Text.AlignRight }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 12
                Button { text: "−10 s"; enabled: player.active; onClicked: { player.seekRelative(-10); page.revealControls() } }
                Button { objectName: "pauseButton"; text: player.paused ? "Fortsetzen" : "Pause"; enabled: player.active && !player.loading; onClicked: { player.togglePause(); page.revealControls() } }
                Button { text: "+10 s"; enabled: player.active; onClicked: { player.seekRelative(10); page.revealControls() } }
                Button { text: player.muted ? "Ton an" : "Stumm"; onClicked: player.muted = !player.muted }
                Slider {
                    objectName: "volumeSlider"
                    from: 0; to: 100; value: player.volume; Layout.preferredWidth: 130
                    onMoved: player.volume = value
                    Accessible.name: "Lautstärke"
                }
                Label { text: Math.round(player.volume) + " %"; color: "#EFF5FF"; Layout.preferredWidth: 48 }
                Item { Layout.fillWidth: true }
                ComboBox {
                    model: ["0,5×", "0,75×", "1×", "1,25×", "1,5×", "2×"]
                    readonly property var speeds: [0.5, 0.75, 1, 1.25, 1.5, 2]
                    currentIndex: speeds.indexOf(player.speed)
                    onActivated: player.speed = speeds[currentIndex]
                    Accessible.name: "Wiedergabegeschwindigkeit"
                }
                Button { text: page.fullscreen ? "Fenster" : "Vollbild"; onClicked: page.fullscreenRequested() }
            }
        }
    }
    Connections {
        target: player
        function onStateChanged() { if (player.paused || player.loading) page.revealControls() }
    }
}
