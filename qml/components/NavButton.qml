import QtQuick
import QtQuick.Controls

Button {
    id: control
    required property QtObject theme
    property bool selected: false
    opacity: enabled ? 1 : 0.3
    padding: 10
    font.pixelSize: 14
    font.weight: selected ? Font.DemiBold : Font.Normal
    contentItem: Text {
        text: control.text
        font: control.font
        color: control.selected || control.hovered ? control.theme.text : control.theme.muted
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 6
        color: control.down ? control.theme.raised : control.hovered ? control.theme.surface : "transparent"
        border.width: control.visualFocus ? 2 : 0
        border.color: control.theme.accent
    }
}
