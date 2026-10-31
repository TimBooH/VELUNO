import QtQuick

Item {
    id: frame
    required property Window targetWindow
    property int grip: 6
    objectName: "windowResizeHandles"
    enabled: targetWindow.visibility === Window.Windowed
    visible: enabled
    z: 1000
    Repeater {
        model: [Qt.LeftEdge, Qt.RightEdge, Qt.TopEdge, Qt.BottomEdge, Qt.LeftEdge | Qt.TopEdge, Qt.RightEdge | Qt.TopEdge, Qt.LeftEdge | Qt.BottomEdge, Qt.RightEdge | Qt.BottomEdge]
        MouseArea {
            required property int modelData
            readonly property bool hasLeft: (modelData & Qt.LeftEdge) !== 0
            readonly property bool hasRight: (modelData & Qt.RightEdge) !== 0
            readonly property bool hasTop: (modelData & Qt.TopEdge) !== 0
            readonly property bool hasBottom: (modelData & Qt.BottomEdge) !== 0
            objectName: "windowResize-" + modelData
            x: hasRight ? frame.width - frame.grip : hasLeft ? 0 : frame.grip
            y: hasBottom ? frame.height - frame.grip : hasTop ? 0 : frame.grip
            width: hasLeft || hasRight ? frame.grip : frame.width - frame.grip * 2
            height: hasTop || hasBottom ? frame.grip : frame.height - frame.grip * 2
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            cursorShape: hasLeft && hasTop || hasRight && hasBottom ? Qt.SizeFDiagCursor : hasRight && hasTop || hasLeft && hasBottom ? Qt.SizeBDiagCursor : hasLeft || hasRight ? Qt.SizeHorCursor : Qt.SizeVerCursor
            onPressed: frame.targetWindow.startSystemResize(modelData)
        }
    }
}
