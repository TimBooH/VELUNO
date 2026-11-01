import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: shelf
    required property QtObject theme
    required property QtObject library
    required property QtObject personal
    property bool scrolling: false
    property int previewRevision: 0
    property int horizontalRevision: 0
    readonly property int cardWidth: Math.max(212, Math.min(292, Math.round(width / 5)))
    required property var model
    required property string heading
    signal detailsRequested(var id)
    signal playRequested(var id)
    visible: row.count > 0
    Layout.fillWidth: true
    spacing: 2
    RowLayout {
        Layout.fillWidth: true
        Label {
            text: shelf.heading
            color: shelf.theme.text
            font.pixelSize: 18
            font.weight: Font.DemiBold
            Layout.fillWidth: true
        }
        NavButton {
            theme: shelf.theme
            objectName: "shelfPrevious"
            text: "‹"
            font.pixelSize: 22
            padding: 2
            implicitWidth: 30
            implicitHeight: 28
            enabled: !row.atXBeginning
            Accessible.name: qsTr("Vorherige Titel")
            onClicked: row.page(-1)
        }
        NavButton {
            theme: shelf.theme
            objectName: "shelfNext"
            text: "›"
            font.pixelSize: 22
            padding: 2
            implicitWidth: 30
            implicitHeight: 28
            enabled: !row.atXEnd
            Accessible.name: qsTr("Weitere Titel")
            onClicked: row.page(1)
        }
    }
    ListView {
        id: row
        objectName: "posterRow"
        Layout.fillWidth: true
        Layout.preferredHeight: shelf.cardWidth * 9 / 16 + 10
        orientation: ListView.Horizontal
        spacing: 8
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
        onContentXChanged: shelf.horizontalRevision++
        onMovementStarted: scrollAnimation.stop()
        function page(direction, animated) {
            cancelFlick();
            let target = Math.max(0, Math.min(contentWidth - width, contentX + direction * Math.max(shelf.cardWidth + 8, width - shelf.cardWidth - 8)));
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
        delegate: LandscapeCard {
            theme: shelf.theme
            library: shelf.library
            personal: shelf.personal
            previewRevision: shelf.previewRevision + shelf.horizontalRevision
            previewAllowed: !row.moving && !scrollAnimation.running && !shelf.scrolling
            width: shelf.cardWidth
            height: implicitHeight
            onDetailsRequested: shelf.detailsRequested(entry.id)
            onPlayRequested: shelf.playRequested(entry.id)
        }
    }
}
