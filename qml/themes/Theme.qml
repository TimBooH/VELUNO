import QtQuick
QtObject {
    property bool light: false
    readonly property color background: light ? "#F4F6FA" : "#080F1C"
    readonly property color surface: light ? "#FFFFFF" : "#101B2B"
    readonly property color raised: light ? "#E7EDF5" : "#1A2A40"
    readonly property color text: light ? "#15253B" : "#EFF5FF"
    readonly property color muted: light ? "#506179" : "#A5B5CC"
    readonly property color border: light ? "#CBD5E3" : "#2A3B50"
    readonly property color accent: light ? "#006B80" : "#64DDF0"
    readonly property color accentText: light ? "#FFFFFF" : "#092331"
}
