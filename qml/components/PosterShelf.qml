import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: shelf
    required property QtObject theme
    required property var model
    required property string heading
    signal detailsRequested(var id)
    signal playRequested(var id)
    visible: row.count > 0
    Layout.fillWidth: true
    spacing: 8
    RowLayout {
        Layout.fillWidth: true
        Label {
            text: shelf.heading
            color: shelf.theme.text
            font.pixelSize: 22
            font.weight: Font.DemiBold
            Layout.fillWidth: true
        }
        NavButton {
            theme: shelf.theme
            objectName: "shelfPrevious"
            text: "‹"
            font.pixelSize: 28
            enabled: !row.atXBeginning
            Accessible.name: qsTr("Vorherige Titel")
            onClicked: row.page(-1)
        }
        NavButton {
            theme: shelf.theme
            objectName: "shelfNext"
            text: "›"
            font.pixelSize: 28
            enabled: !row.atXEnd
            Accessible.name: qsTr("Weitere Titel")
            onClicked: row.page(1)
        }
    }
    ListView {
        id: row
        objectName: "posterRow"
        Layout.fillWidth: true
        Layout.preferredHeight: 326
        orientation: ListView.Horizontal
        spacing: 10
        clip: true
        topMargin: 5
        bottomMargin: 5
        model: shelf.model
        Keys.onPressed: function (event) {
            if (event.key === Qt.Key_Home) {
                scrollAnimation.stop();
                positionViewAtBeginning();
                event.accepted = true;
            } else if (event.key === Qt.Key_End) {
                scrollAnimation.stop();
                positionViewAtEnd();
                event.accepted = true;
            }
        }
        boundsBehavior: Flickable.StopAtBounds
        onMovementStarted: scrollAnimation.stop()
        function page(direction, animated) {
            cancelFlick();
            let target = Math.max(0, Math.min(contentWidth - width, contentX + direction * Math.max(194, width - 194)));
            if (animated === false) {
                scrollAnimation.stop();
                contentX = target;
                return;
            }
            scrollAnimation.to = target;
            scrollAnimation.restart();
        }
        NumberAnimation {
            id: scrollAnimation
            target: row
            property: "contentX"
            duration: 220
            easing.type: Easing.OutCubic
        }
        Keys.onRightPressed: page(1, false)
        Keys.onLeftPressed: page(-1, false)
        ScrollBar.horizontal: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
        delegate: PosterCard {
            theme: shelf.theme
            width: 184
            height: implicitHeight
            onDetailsRequested: shelf.detailsRequested(entry.id)
            onPlayRequested: shelf.playRequested(entry.id)
        }
    }
}
