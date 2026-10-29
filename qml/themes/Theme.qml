import QtQuick
QtObject {
    property bool light: false
    readonly property color background: light ? "#F4F6FA" : "#0B1426"
    readonly property color surface: light ? "#FFFFFF" : "#111E34"
    readonly property color raised: light ? "#E7EDF5" : "#1B2C46"
    readonly property color text: light ? "#15253B" : "#EFF5FF"
    readonly property color muted: light ? "#506179" : "#A5B5CC"
    readonly property color border: light ? "#CBD5E3" : "#2C3E59"
    readonly property color accent: light ? "#006B80" : "#64DDF0"
    readonly property color onAccent: light ? "#FFFFFF" : "#092331"
}
