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
    spacing: 12
    Label { text: heading; color: theme.text; font.pixelSize: 22; font.weight: Font.DemiBold }
    ListView {
        id: row
        Layout.fillWidth: true; Layout.preferredHeight: 285
        orientation: ListView.Horizontal; spacing: 14; clip: true
        model: shelf.model
        ScrollBar.horizontal: ScrollBar {}
        delegate: PosterCard {
            theme: shelf.theme
            width: 164; height: 276
            onDetailsRequested: shelf.detailsRequested(entry.id)
            onPlayRequested: shelf.playRequested(entry.id)
        }
    }
}
