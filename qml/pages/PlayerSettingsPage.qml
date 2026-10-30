import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: page
    required property QtObject theme
    required property QtObject player
    required property QtObject preferences
    contentWidth: availableWidth
    ColumnLayout {
        width: parent.width
        spacing: 16
        Label {
            text: qsTr("Audioausgabe")
            color: page.theme.text
        }
        ComboBox {
            objectName: "audioDeviceSelector"
            model: page.player.audioDevices
            textRole: "description"
            valueRole: "name"
            Layout.preferredWidth: 400
            currentIndex: {
                count;
                page.preferences.options;
                return Math.max(0, indexOfValue(page.preferences.options.audio_device));
            }
            onActivated: page.preferences.setOption("audio_device", currentValue)
        }
        Label {
            text: qsTr("Decodierung")
            color: page.theme.text
        }
        ComboBox {
            Layout.preferredWidth: 400
            model: [qsTr("Automatisch mit Software-Fallback"), qsTr("Hardware bevorzugen"), qsTr("Software")]
            readonly property var values: ["auto", "hardware", "software"]
            currentIndex: values.indexOf(page.preferences.options.decoder)
            onActivated: page.preferences.setOption("decoder", values[currentIndex])
        }
        Label {
            text: qsTr("Untertitel")
            color: page.theme.text
        }
        RowLayout {
            Label {
                text: qsTr("Schriftgröße")
                color: page.theme.muted
            }
            SpinBox {
                from: 12
                to: 100
                value: page.preferences.options.sub_size
                onValueModified: page.preferences.setOption("sub_size", value)
            }
        }
        RowLayout {
            Label {
                text: qsTr("Vertikale Position (%)")
                color: page.theme.muted
            }
            SpinBox {
                from: 0
                to: 100
                value: page.preferences.options.sub_position
                onValueModified: page.preferences.setOption("sub_position", value)
            }
        }
        RowLayout {
            Label {
                text: qsTr("Zeitversatz (Zehntelsekunden)")
                color: page.theme.muted
            }
            SpinBox {
                from: -1200
                to: 1200
                value: Math.round(page.preferences.options.sub_delay * 10)
                editable: true
                onValueModified: page.preferences.setOption("sub_delay", value / 10)
            }
        }
        CheckBox {
            text: qsTr("Untertitel mit Hintergrund")
            checked: page.preferences.options.sub_background
            onToggled: page.preferences.setOption("sub_background", checked)
        }
        Label {
            text: qsTr("Tastaturkürzel")
            color: page.theme.text
            font.pixelSize: 20
        }
        Label {
            text: qsTr("Tastenbezeichnungen wie Space, Left, F oder Ctrl+P. Doppelte Belegungen und Strg+O werden abgewiesen.")
            color: page.theme.muted
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Repeater {
            model: [
                {
                    key: "pause",
                    label: "Pause"
                },
                {
                    key: "back",
                    label: "10 Sekunden zurück"
                },
                {
                    key: "forward",
                    label: "10 Sekunden vor"
                },
                {
                    key: "volume_up",
                    label: "Lauter"
                },
                {
                    key: "volume_down",
                    label: "Leiser"
                },
                {
                    key: "fullscreen",
                    label: "Vollbild"
                },
                {
                    key: "exit_fullscreen",
                    label: "Vollbild verlassen"
                },
                {
                    key: "mute",
                    label: "Stumm"
                },
                {
                    key: "subtitles",
                    label: "Untertitelspur"
                },
                {
                    key: "audio",
                    label: "Audiospur"
                },
                {
                    key: "next",
                    label: "Nächste Folge"
                }
            ]
            RowLayout {
                required property var modelData
                Label {
                    text: modelData.label
                    color: page.theme.text
                    Layout.preferredWidth: 190
                }
                TextField {
                    id: sequence
                    text: page.preferences.shortcuts[modelData.key]
                    Layout.preferredWidth: 170
                }
                Button {
                    text: qsTr("Übernehmen")
                    onClicked: page.preferences.setShortcut(modelData.key, sequence.text)
                }
            }
        }
        Button {
            text: qsTr("Standardbelegung wiederherstellen")
            onClicked: page.preferences.resetShortcuts()
        }
        Label {
            text: page.preferences.error
            color: page.theme.accent
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
