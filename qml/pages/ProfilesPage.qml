import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ColumnLayout {
    id: page
    required property QtObject theme
    required property QtObject personal
    Label {
        text: qsTr("Die Mediathek ist gemeinsam. Favoriten, Sammlungen, Fortschritt und Ansichten gehören zum jeweiligen Profil.")
        color: theme.muted
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    Button {
        objectName: "newProfile"
        text: qsTr("Profil hinzufügen")
        onClicked: edit(0, "", "initials", "#36BBD1")
    }
    function edit(id, name, avatar, color) {
        editor.profileId = id;
        profileName.text = name;
        editor.avatar = avatar;
        editor.color = color;
        editor.open();
    }
    ListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: 10
        model: personal.profiles
        ScrollBar.vertical: ScrollBar {}
        delegate: Rectangle {
            required property var entry
            width: ListView.view.width
            height: 76
            color: page.theme.surface
            radius: 8
            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                Rectangle {
                    width: 42
                    height: 42
                    radius: 21
                    color: entry.color
                    Image {
                        anchors.fill: parent
                        source: entry.avatar_url || ""
                        fillMode: Image.PreserveAspectCrop
                        visible: !!entry.avatar_url
                    }
                    Label {
                        visible: !entry.avatar_url
                        anchors.centerIn: parent
                        text: entry.avatar === "cinema" ? "▣" : entry.avatar === "rocket" ? "↑" : entry.avatar === "planet" ? "◉" : entry.name.substring(0, 1).toUpperCase()
                        color: "#081424"
                        font.pixelSize: 24
                    }
                }
                Label {
                    text: entry.name + (entry.id === personal.activeProfile.id ? " · Aktiv" : "")
                    color: page.theme.text
                    Layout.fillWidth: true
                    textFormat: Text.PlainText
                }
                Button {
                    text: qsTr("Wechseln")
                    enabled: entry.id !== personal.activeProfile.id
                    onClicked: personal.switchProfile(entry.id)
                }
                Button {
                    text: qsTr("Bearbeiten")
                    onClicked: page.edit(entry.id, entry.name, entry.avatar, entry.color)
                }
                Button {
                    text: qsTr("Löschen")
                    onClicked: {
                        removal.profileId = entry.id;
                        removal.open();
                    }
                }
            }
        }
    }
    Label {
        visible: personal.error.length > 0
        text: personal.error
        wrapMode: Text.WordWrap
        color: theme.accent
        Layout.fillWidth: true
    }
    Dialog {
        id: editor
        objectName: "profileEditor"
        property var profileId: 0
        property string avatar: "initials"
        property string color: "#36BBD1"
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 520)
        modal: true
        title: qsTr("Profil bearbeiten")
        ColumnLayout {
            anchors.fill: parent
            TextField {
                id: profileName
                objectName: "profileEditorName"
                placeholderText: qsTr("Profilname")
                Layout.fillWidth: true
            }
            ComboBox {
                Layout.preferredWidth: 280
                model: [qsTr("Farbige Initialen"), qsTr("Kino"), qsTr("Rakete"), qsTr("Planet")]
                readonly property var avatars: ["initials", "cinema", "rocket", "planet"]
                currentIndex: Math.max(0, avatars.indexOf(editor.avatar))
                onActivated: editor.avatar = avatars[currentIndex]
            }
            RowLayout {
                Repeater {
                    model: [qsTr("#36BBD1"), qsTr("#F1BD61"), qsTr("#B995E4"), qsTr("#81CF9F"), qsTr("#F495A0")]
                    Button {
                        Layout.minimumWidth: 44
                        Layout.maximumWidth: 44
                        Layout.preferredWidth: 44
                        required property string modelData
                        text: editor.color === modelData ? "●" : "○"
                        palette.buttonText: modelData
                        onClicked: editor.color = modelData
                    }
                }
            }
            Button {
                text: qsTr("Eigenes Bild wählen")
                onClicked: avatarPicker.open()
            }
            Label {
                text: editor.avatar.indexOf("file:") === 0 || editor.avatar.indexOf("images/") === 0 ? "Eigenes Bild ausgewählt" : "Standardavatar"
                color: page.theme.muted
            }
        }
        footer: DialogButtonBox {
            Button {
                text: qsTr("Abbrechen")
                onClicked: editor.close()
            }
            Button {
                text: qsTr("Speichern")
                enabled: profileName.text.trim().length > 0
                onClicked: {
                    personal.clearError();
                    personal.saveProfile(editor.profileId, profileName.text, editor.avatar, editor.color);
                    if (!personal.error.length)
                        editor.close();
                }
            }
        }
    }
    FileDialog {
        id: avatarPicker
        nameFilters: ["Bilder (*.png *.jpg *.jpeg *.webp)"]
        onAccepted: editor.avatar = selectedFile.toString()
    }
    Dialog {
        id: removal
        property var profileId: 0
        parent: Overlay.overlay
        anchors.centerIn: parent
        title: qsTr("Profil und persönliche Daten löschen?")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: personal.deleteProfile(profileId)
        Label {
            text: qsTr("Die gemeinsame Mediathek und Originalvideos bleiben erhalten.")
            color: page.theme.text
        }
    }
}
