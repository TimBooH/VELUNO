import QtQuick
import QtQuick.Controls

Button {
    id: action
    required property QtObject theme
    property string symbol
    property bool primary: false
    implicitWidth: 36
    implicitHeight: 36
    padding: 0
    hoverEnabled: true
    Accessible.name: text
    ToolTip.visible: hovered
    ToolTip.delay: 500
    ToolTip.text: text
    contentItem: Label {
        text: action.symbol
        font.pixelSize: 20
        color: action.primary ? "#101722" : action.theme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: width / 2
        color: action.primary ? (action.down ? "#BAC3CF" : "#FFFFFF") : action.hovered || action.down ? action.theme.raised : action.theme.surface
        border.width: action.visualFocus ? 2 : 1
        border.color: action.visualFocus ? action.theme.accent : action.primary ? "#FFFFFF" : action.theme.muted
    }
}
