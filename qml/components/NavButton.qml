import QtQuick
import QtQuick.Controls
Button {
    id: control
    required property QtObject theme
    property bool selected: false
    padding: 12
    font.pixelSize: 14
    font.weight: selected ? Font.DemiBold : Font.Normal
    contentItem: Text {
        text: control.text
        font: control.font
        color: control.selected ? control.theme.accent : control.theme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 6
        color: control.down ? control.theme.raised : control.hovered || control.selected ? control.theme.surface : "transparent"
        border.width: control.visualFocus ? 2 : 0
        border.color: control.theme.accent
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 24; height: 2
            color: control.theme.accent
            visible: control.selected
        }
    }
}
