import QtQuick
import QtQuick.Controls

Row {
    id: controls
    required property QtObject theme
    required property Window targetWindow
    readonly property bool maximized: targetWindow.visibility === Window.Maximized
    spacing: 2
    component WindowButton: Button {
        id: button
        property string operation
        width: 38
        height: 40
        padding: 0
        hoverEnabled: true
        Accessible.name: text
        ToolTip.visible: hovered
        ToolTip.delay: 600
        ToolTip.text: text
        background: Rectangle {
            radius: 4
            color: button.operation === "close" && (button.hovered || button.down) ? "#C42B36" : button.down ? controls.theme.raised : button.hovered ? controls.theme.surface : "transparent"
            border.width: button.visualFocus ? 2 : 0
            border.color: controls.theme.accent
        }
        contentItem: Canvas {
            id: icon
            readonly property color ink: button.operation === "close" && (button.hovered || button.down) ? "#FFFFFF" : controls.theme.text
            readonly property bool restore: controls.maximized
            onInkChanged: requestPaint()
            onRestoreChanged: requestPaint()
            onPaint: {
                let ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                ctx.strokeStyle = ink;
                ctx.lineWidth = 1.5;
                const x = Math.round((width - 12) / 2);
                const y = Math.round((height - 12) / 2);
                ctx.beginPath();
                if (button.operation === "minimize") {
                    ctx.moveTo(x, y + 8);
                    ctx.lineTo(x + 12, y + 8);
                } else if (button.operation === "close") {
                    ctx.moveTo(x + 1, y + 1);
                    ctx.lineTo(x + 11, y + 11);
                    ctx.moveTo(x + 11, y + 1);
                    ctx.lineTo(x + 1, y + 11);
                } else if (restore) {
                    ctx.rect(x, y + 3, 9, 9);
                    ctx.moveTo(x + 3, y + 3);
                    ctx.lineTo(x + 3, y);
                    ctx.lineTo(x + 12, y);
                    ctx.lineTo(x + 12, y + 9);
                    ctx.lineTo(x + 9, y + 9);
                } else {
                    ctx.rect(x + 1, y + 1, 10, 10);
                }
                ctx.stroke();
            }
        }
    }
    WindowButton {
        objectName: "windowMinimize"
        operation: "minimize"
        text: qsTr("Minimieren")
        onClicked: controls.targetWindow.showMinimized()
    }
    WindowButton {
        objectName: "windowMaximize"
        operation: "maximize"
        text: controls.maximized ? qsTr("Wiederherstellen") : qsTr("Maximieren")
        onClicked: {
            if (controls.maximized)
                controls.targetWindow.showNormal();
            else
                controls.targetWindow.showMaximized();
        }
    }
    WindowButton {
        objectName: "windowClose"
        operation: "close"
        text: qsTr("Schließen")
        onClicked: controls.targetWindow.close()
    }
}
