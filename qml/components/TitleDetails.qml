import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: detail
    objectName: "titleDetails"
    required property QtObject theme
    required property QtObject library
    required property QtObject personal
    readonly property var entry: library.selectedTitle
    readonly property bool isSeries: entry.kind === "series"
    readonly property var files: library.detailFiles
    readonly property var progressRows: {
        personal.revision;
        return personal.progressForTitle(entry.id || 0);
    }
    readonly property var resumeProgress: progressRows.find(function (p) {
        return !p.seen && p.position > 0;
    }) || ({})
    readonly property bool canResume: resumeProgress.position > 0 && (!isSeries || !!resumeProgress.preferred_file_id)
    function progressFor(file) {
        return progressRows.find(function (p) {
            return p.season === file.season && p.episode === file.episode;
        }) || ({});
    }
    function quality(file) {
        return (file.height >= 2160 ? "4K UHD" : file.height >= 1080 ? "Full HD" : file.height >= 720 ? "HD" : qsTr("Video")) + " · " + file.width + " × " + file.height;
    }
    property bool showFiles: false
    property int season: -1
    readonly property var seasons: {
        let result = [];
        for (let file of files) {
            if (result.indexOf(file.season) < 0)
                result.push(file.season);
        }
        return result.sort(function (a, b) {
            return a - b;
        });
    }
    readonly property var visibleFiles: isSeries ? files.filter(function (file) {
        return file.season === (season < 0 ? seasons[0] : season);
    }) : showFiles ? files : []
    signal editRequested
    signal collectionsRequested
    signal assignRequested(var entry)
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 48, 960)
    height: Math.min(parent.height - 48, 850)
    padding: 0
    title: entry.title || qsTr("Details")
    header: null
    footer: null
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onAboutToShow: {
        showFiles = false;
        season = -1;
        contents.positionViewAtBeginning();
    }
    onClosed: {
        if (library.networkBusy)
            library.cancelPendingPlayback();
    }
    background: Rectangle {
        color: detail.theme.surface
        radius: 10
    }
    contentItem: Item {
        ListView {
            id: contents
            objectName: "titleDetailsScroll"
            anchors.fill: parent
            clip: true
            model: detail.visibleFiles
            spacing: 1
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            header: Column {
                id: information
                width: contents.width
                Rectangle {
                    id: hero
                    width: parent.width
                    height: Math.max(310, Math.min(440, width * 0.5))
                    color: detail.theme.raised
                    Image {
                        id: artwork
                        objectName: "titleBackdrop"
                        anchors.fill: parent
                        source: detail.library.assetUrl(detail.entry.backdrop || detail.entry.poster || "")
                        fillMode: Image.PreserveAspectCrop
                        sourceSize.width: 1600
                        asynchronous: true
                    }
                    Rectangle {
                        anchors.fill: parent
                        gradient: Gradient {
                            GradientStop {
                                position: 0
                                color: "#25000000"
                            }
                            GradientStop {
                                position: 0.45
                                color: "#35000000"
                            }
                            GradientStop {
                                position: 1
                                color: detail.theme.light ? "#E6060B14" : detail.theme.surface
                            }
                        }
                    }
                    // The title sits over a consistently dark scrim in both themes.
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: heroTitle.implicitHeight + 130
                        gradient: Gradient {
                            GradientStop {
                                position: 0
                                color: "#00060B14"
                            }
                            GradientStop {
                                position: 0.6
                                color: "#BB060B14"
                            }
                            GradientStop {
                                position: 1
                                color: detail.theme.light ? "#DD060B14" : detail.theme.surface
                            }
                        }
                    }
                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 32
                        spacing: 16
                        Label {
                            text: detail.isSeries ? qsTr("SERIE") : qsTr("FILM")
                            font.pixelSize: 11
                            font.letterSpacing: 3
                            font.weight: Font.DemiBold
                            color: "#C9E4EB"
                        }
                        Label {
                            id: heroTitle
                            objectName: "titleHeading"
                            text: detail.entry.title || ""
                            textFormat: Text.PlainText
                            Layout.fillWidth: true
                            Layout.rightMargin: 24
                            font.pixelSize: detail.width < 700 ? 30 : 42
                            font.weight: Font.Bold
                            color: "#FFFFFF"
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                        Button {
                            id: primaryPlay
                            Connections {
                                target: detail
                                function onOpened() {
                                    primaryPlay.forceActiveFocus();
                                }
                            }
                            objectName: "titlePrimaryPlay"
                            text: detail.library.networkBusy ? qsTr("Verbinden …") : detail.canResume ? qsTr("Fortsetzen") : detail.isSeries ? qsTr("Episoden auswählen") : qsTr("Abspielen")
                            enabled: !detail.library.networkBusy && (detail.isSeries ? detail.files.length > 0 : !!detail.entry.available)
                            opacity: enabled ? 1 : 0.5
                            leftPadding: 22
                            rightPadding: 22
                            topPadding: 12
                            bottomPadding: 12
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            contentItem: Text {
                                objectName: "titlePrimaryPlayLabel"
                                text: "\u25B6\uFE0E  " + primaryPlay.text
                                font: primaryPlay.font
                                color: detail.theme.accentText
                                horizontalAlignment: Text.AlignHCenter
                            }
                            background: Rectangle {
                                radius: 4
                                border.width: primaryPlay.visualFocus ? 2 : 0
                                border.color: detail.theme.accentText
                                color: primaryPlay.hovered ? detail.theme.muted : detail.theme.accent
                            }
                            onClicked: {
                                if (detail.canResume)
                                    detail.personal.resumeTitle(detail.entry.id);
                                else if (detail.isSeries)
                                    contents.positionViewAtIndex(0, ListView.Beginning);
                                else
                                    detail.library.playTitle(detail.entry.id);
                            }
                        }
                    }
                }
                ColumnLayout {
                    width: parent.width
                    spacing: 18
                    // Margins belong to the content, not the full-bleed artwork.
                    Item {
                        Layout.preferredHeight: 6
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 32
                        Layout.rightMargin: 32
                        spacing: 14
                        Label {
                            text: [detail.entry.year > 0 ? detail.entry.year : "", detail.isSeries ? qsTr("Serie") : qsTr("Film"), !detail.isSeries && detail.files.length > 0 && detail.files[0].duration > 0 ? Math.ceil(detail.files[0].duration / 60) + " Min." : ""].filter(function (value) {
                                return value !== "";
                            }).join("  ·  ")
                            color: detail.theme.muted
                        }
                        Label {
                            visible: detail.entry.rating > 0
                            text: qsTr("Bewertung") + "  " + Number(detail.entry.rating || 0).toFixed(1) + " / 10"
                            color: detail.theme.accent
                            font.weight: Font.DemiBold
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                    }
                    ColumnLayout {
                        visible: detail.resumeProgress.position > 0
                        Layout.fillWidth: true
                        Layout.leftMargin: 32
                        Layout.rightMargin: 32
                        Label {
                            objectName: "titleResumePosition"
                            text: (detail.isSeries ? "S" + detail.resumeProgress.season + " E" + detail.resumeProgress.episode + " · " : "") + qsTr("Fortsetzen bei") + " " + Math.floor((detail.resumeProgress.position || 0) / 60) + ":" + ("0" + Math.floor((detail.resumeProgress.position || 0) % 60)).slice(-2)
                            color: detail.theme.accent
                        }
                        ProgressBar {
                            id: titleProgress
                            Layout.fillWidth: true
                            from: 0
                            to: Math.max(1, detail.resumeProgress.duration || 0)
                            value: detail.resumeProgress.position || 0
                            background: null
                            contentItem: Rectangle {
                                implicitHeight: 4
                                color: detail.theme.raised
                                Rectangle {
                                    width: parent.width * titleProgress.position
                                    height: parent.height
                                    color: detail.theme.accent
                                }
                            }
                        }
                    }
                    Flow {
                        Layout.fillWidth: true
                        Layout.leftMargin: 22
                        Layout.rightMargin: 22
                        spacing: 4
                        NavButton {
                            objectName: "titleFavorite"
                            theme: detail.theme
                            text: {
                                detail.personal.revision;
                                return detail.personal.isFavorite(detail.entry.id || 0) ? qsTr("Favorit entfernen") : qsTr("Als Favorit");
                            }
                            onClicked: detail.personal.toggleFavorite(detail.entry.id)
                        }
                        NavButton {
                            objectName: "titleSeen"
                            theme: detail.theme
                            text: {
                                detail.personal.revision;
                                return detail.personal.isSeen(detail.entry.id || 0) ? qsTr("Als ungesehen") : qsTr("Als gesehen");
                            }
                            onClicked: detail.personal.markSeen(detail.entry.id, !detail.personal.isSeen(detail.entry.id))
                        }
                        NavButton {
                            objectName: "titleCollectionsButton"
                            theme: detail.theme
                            text: qsTr("Sammlungen")
                            onClicked: detail.collectionsRequested()
                        }
                    }
                    Label {
                        objectName: "titleOverview"
                        Layout.fillWidth: true
                        Layout.leftMargin: 32
                        Layout.rightMargin: 32
                        text: detail.entry.overview || qsTr("Noch keine Beschreibung vorhanden. Du kannst Informationen über TMDB ergänzen oder selbst eintragen.")
                        textFormat: Text.PlainText
                        wrapMode: Text.WordWrap
                        color: detail.theme.text
                        font.pixelSize: 15
                        lineHeight: 1.35
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: 32
                        Layout.rightMargin: 32
                        visible: !!detail.entry.genres
                        text: detail.entry.genres || ""
                        textFormat: Text.PlainText
                        wrapMode: Text.WordWrap
                        color: detail.theme.muted
                    }
                    Flow {
                        Layout.fillWidth: true
                        Layout.leftMargin: 22
                        Layout.rightMargin: 22
                        spacing: 4
                        NavButton {
                            objectName: "titleEdit"
                            theme: detail.theme
                            text: qsTr("Informationen und Bilder bearbeiten")
                            onClicked: detail.editRequested()
                        }
                        NavButton {
                            objectName: "titleFilesToggle"
                            theme: detail.theme
                            text: (detail.showFiles ? "−  " : "+  ") + qsTr("Dateien und Versionen") + (!detail.isSeries ? " (" + detail.files.length + ")" : "")
                            onClicked: detail.showFiles = !detail.showFiles
                        }
                    }
                    RowLayout {
                        visible: detail.isSeries || detail.showFiles
                        Layout.fillWidth: true
                        Layout.leftMargin: 32
                        Layout.rightMargin: 32
                        Label {
                            text: detail.isSeries ? qsTr("Episoden") : qsTr("Filmversionen")
                            font.pixelSize: 22
                            font.weight: Font.DemiBold
                            color: detail.theme.text
                            Layout.fillWidth: true
                        }
                        ComboBox {
                            objectName: "titleSeasonSelector"
                            visible: detail.isSeries
                            model: detail.seasons.map(function (value) {
                                return qsTr("Staffel") + " " + value;
                            })
                            currentIndex: Math.max(0, detail.seasons.indexOf(detail.season))
                            onActivated: detail.season = detail.seasons[currentIndex]
                            Accessible.name: qsTr("Staffel auswählen")
                        }
                    }
                    Label {
                        visible: detail.showFiles
                        Layout.fillWidth: true
                        Layout.leftMargin: 32
                        Layout.rightMargin: 32
                        text: qsTr("Standard: höchste Auflösung. Andere Versionen kannst du hier gezielt abspielen oder neu zuordnen.")
                        wrapMode: Text.WordWrap
                        color: detail.theme.muted
                    }
                    Label {
                        visible: !detail.entry.available
                        Layout.fillWidth: true
                        Layout.leftMargin: 32
                        Layout.rightMargin: 32
                        text: qsTr("Medien zurzeit nicht verfügbar. Prüfe das Laufwerk oder die Netzwerkverbindung.")
                        wrapMode: Text.WordWrap
                        color: detail.theme.muted
                    }
                    Item {
                        Layout.preferredHeight: 12
                    }
                }
            }
            delegate: ItemDelegate {
                id: fileRow
                required property var modelData
                objectName: "titleFile-" + modelData.id
                width: contents.width
                height: rowContent.implicitHeight + 32
                leftPadding: 32
                rightPadding: 32
                topPadding: 16
                bottomPadding: 16
                Accessible.name: detail.isSeries ? qsTr("Episode") + " " + modelData.episode : modelData.width + " × " + modelData.height
                onClicked: {
                    if (modelData.available)
                        detail.library.playFile(modelData.path);
                }
                background: Rectangle {
                    color: fileRow.hovered ? detail.theme.raised : detail.theme.surface
                }
                contentItem: RowLayout {
                    id: rowContent
                    spacing: 18
                    Label {
                        visible: detail.isSeries
                        text: fileRow.modelData.episode
                        font.pixelSize: 28
                        color: detail.theme.muted
                        Layout.preferredWidth: 40
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Label {
                            Layout.fillWidth: true
                            text: (detail.isSeries ? qsTr("Episode") + " " + fileRow.modelData.episode + "  ·  " : "") + detail.quality(fileRow.modelData) + (fileRow.modelData.available ? "" : "  ·  " + qsTr("Nicht verfügbar"))
                            color: detail.theme.text
                            wrapMode: Text.WordWrap
                        }
                        Label {
                            visible: !!detail.progressFor(fileRow.modelData).seen
                            text: qsTr("Gesehen")
                            color: detail.theme.accent
                        }
                        ProgressBar {
                            id: episodeProgress
                            background: null
                            contentItem: Rectangle {
                                implicitHeight: 4
                                color: detail.theme.raised
                                Rectangle {
                                    width: parent.width * episodeProgress.position
                                    height: parent.height
                                    color: detail.theme.accent
                                }
                            }
                            visible: !detail.progressFor(fileRow.modelData).seen && detail.progressFor(fileRow.modelData).position > 0
                            Layout.fillWidth: true
                            from: 0
                            to: Math.max(1, detail.progressFor(fileRow.modelData).duration || 0)
                            value: detail.progressFor(fileRow.modelData).position || 0
                        }
                        Label {
                            visible: detail.showFiles
                            Layout.fillWidth: true
                            text: fileRow.modelData.path
                            textFormat: Text.PlainText
                            color: detail.theme.muted
                            elide: Text.ElideMiddle
                        }
                        Label {
                            visible: !!fileRow.modelData.diagnostic
                            Layout.fillWidth: true
                            text: fileRow.modelData.diagnostic || ""
                            textFormat: Text.PlainText
                            color: detail.theme.muted
                            wrapMode: Text.WordWrap
                        }
                    }
                    NavButton {
                        visible: detail.showFiles
                        objectName: "titleFileAssign-" + fileRow.modelData.id
                        theme: detail.theme
                        text: qsTr("Zuordnen")
                        onClicked: detail.assignRequested(fileRow.modelData)
                    }
                    NavButton {
                        objectName: "titleFilePlay-" + fileRow.modelData.id
                        theme: detail.theme
                        text: qsTr("Abspielen")
                        enabled: fileRow.modelData.available && !detail.library.networkBusy
                        onClicked: detail.library.playFile(fileRow.modelData.path)
                    }
                }
            }
            footer: Item {
                width: contents.width
                height: 24
            }
        }
        Button {
            id: closeButton
            objectName: "titleDetailsClose"
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 16
            width: 36
            height: 36
            text: "×"
            Accessible.name: qsTr("Schließen")
            contentItem: Label {
                text: closeButton.text
                font.pixelSize: 28
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: 18
                color: closeButton.hovered ? "#E13C4C60" : "#DD101827"
                border.width: closeButton.visualFocus ? 2 : 0
                border.color: detail.theme.accent
            }
            onClicked: detail.close()
        }
    }
}
