import QtQuick
import QtQuick.Controls

Button {
    id: action
    required property QtObject theme
    property string symbol
    property bool selected: false
    readonly property color highlight: theme.light ? "#64DDF0" : theme.accent
    implicitWidth: 44
    implicitHeight: 44
    hoverEnabled: true
    Accessible.name: text
    ToolTip.visible: hovered
    ToolTip.delay: 500
    ToolTip.text: text
    contentItem: Label {
        text: action.symbol
        color: action.selected ? action.highlight : "#FFFFFF"
        font.pixelSize: 23
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: width / 2
        color: action.down ? "#CC263345" : action.hovered ? "#B3263345" : "#99080F1C"
        border.width: action.visualFocus ? 2 : 1
        border.color: action.visualFocus || action.selected ? action.highlight : action.hovered ? "#FFFFFF" : "#90FFFFFF"
    }
}
