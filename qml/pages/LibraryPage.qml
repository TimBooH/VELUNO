import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page
    required property QtObject theme
    required property QtObject personal
    required property QtObject metadata
    required property QtObject library
    property string pageTitle: "Startseite"
    property string description: "Deine Filme und Serien. An einem Ort."
    property int optionsVersion: 0
    Connections {
        target: library
        function onOptionsChanged() {
            page.optionsVersion++;
        }
    }
    property bool showUnassigned: false
    readonly property bool catalogPage: pageTitle === "Filme" || pageTitle === "Serien"
    readonly property string kind: pageTitle === "Serien" ? "series" : "movie"
    signal openFileRequested
    signal foldersRequested
    signal assignRequested(var entry)
    ColumnLayout {
        anchors.fill: parent
        spacing: 14
        RowLayout {
            visible: page.pageTitle !== "Startseite" || library.movieCount + library.seriesCount === 0
            Layout.fillWidth: true
            Label {
                text: qsTr(pageTitle)
                color: theme.text
                font.pixelSize: 30
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Medienordner")
                onClicked: page.foldersRequested()
            }
            Button {
                text: qsTr("Datei abspielen")
                onClicked: page.openFileRequested()
            }
        }
        Label {
            visible: page.pageTitle !== "Startseite" || library.movieCount + library.seriesCount === 0
            text: qsTr(description)
            color: theme.muted
            font.pixelSize: 15
        }
        RowLayout {
            visible: page.catalogPage
            Label {
                text: qsTr("Sortierung")
                color: theme.muted
            }
            ComboBox {
                model: [qsTr("Titel"), qsTr("Zuletzt hinzugefügt"), qsTr("Jahr")]
                readonly property var orders: ["title", "recent", "year"]
                currentIndex: {
                    page.optionsVersion;
                    return orders.indexOf(library.sortOrder(page.kind));
                }
                onActivated: library.setOptions(page.kind, orders[currentIndex], library.availability(page.kind))
                Accessible.name: qsTr("Bibliothek sortieren")
            }
            ComboBox {
                objectName: "availabilityFilter"
                model: [qsTr("Alle Titel"), qsTr("Verfügbar"), qsTr("Nicht verfügbar")]
                readonly property var filters: [-1, 1, 0]
                currentIndex: {
                    page.optionsVersion;
                    return filters.indexOf(library.availability(page.kind));
                }
                onActivated: library.setOptions(page.kind, library.sortOrder(page.kind), filters[currentIndex])
                Accessible.name: qsTr("Nach Verfügbarkeit filtern")
            }
            Item {
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("Kachelgröße")
                color: theme.muted
            }
            Slider {
                from: 130
                to: 230
                value: library.tileSize
                Layout.preferredWidth: 120
                onMoved: library.tileSize = Math.round(value)
                Accessible.name: qsTr("Kachelgröße")
            }
            Button {
                objectName: "unassignedToggle"
                text: qsTr("Nicht zugeordnet (") + library.unassignedCount + ")"
                checkable: true
                checked: page.showUnassigned
                onClicked: page.showUnassigned = !page.showUnassigned
            }
        }
        RowLayout {
            visible: page.pageTitle === "Startseite" && library.movieCount + library.seriesCount > 0 && (!page.metadata.configured || library.onlineConsent !== "granted")
            Layout.fillWidth: true
            spacing: 12
            Label {
                Layout.fillWidth: true
                text: qsTr("Cover fehlen? Online-Metadaten einrichten oder eigene Bilder in den Titeldetails wählen.")
                color: page.theme.muted
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
            ToolButton {
                objectName: "configureArtwork"
                text: qsTr("Cover einrichten") + "  ›"
                onClicked: page.foldersRequested()
            }
        }
        Label {
            visible: library.busy
            text: library.paused ? "Bibliotheksscan pausiert" : "Bibliothek wird im Hintergrund ergänzt · " + library.checked + " Dateien geprüft"
            color: theme.accent
        }
        ScrollView {
            visible: pageTitle === "Startseite"
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            ColumnLayout {
                width: parent.width
                spacing: 30
                ContinueShelf {
                    theme: page.theme
                    personal: page.personal
                }
                PosterShelf {
                    theme: page.theme
                    model: library.recent
                    heading: qsTr("Zuletzt hinzugefügt")
                    onDetailsRequested: library.openDetails(id)
                    onPlayRequested: library.playTitle(id)
                }
                PosterShelf {
                    theme: page.theme
                    model: library.randomMovies
                    heading: qsTr("Meine Filme")
                    onDetailsRequested: library.openDetails(id)
                    onPlayRequested: library.playTitle(id)
                }
                PosterShelf {
                    theme: page.theme
                    model: library.randomSeries
                    heading: qsTr("Meine Serien")
                    onDetailsRequested: library.openDetails(id)
                    onPlayRequested: library.playTitle(id)
                }
                Label {
                    visible: library.movieCount + library.seriesCount === 0
                    text: qsTr("Deine Mediathek beginnt hier. Wähle Medienordner, um deine Filme und Serien einzulesen.")
                    wrapMode: Text.WordWrap
                    color: page.theme.muted
                    Layout.fillWidth: true
                    Layout.topMargin: 48
                }
            }
        }
        GridView {
            id: grid
            objectName: "movieGrid"
            visible: (page.catalogPage && !page.showUnassigned) || pageTitle === "Favoriten"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cellWidth: library.tileSize + 18
            cellHeight: library.tileSize * 1.5 + 56
            model: pageTitle === "Favoriten" ? personal.favorites : page.kind === "movie" ? library.movies : library.series
            ScrollBar.vertical: ScrollBar {}
            delegate: PosterCard {
                theme: page.theme
                width: page.library.tileSize
                height: implicitHeight
                onDetailsRequested: page.library.openDetails(entry.id)
                onPlayRequested: page.library.playTitle(entry.id)
            }
            Label {
                anchors.centerIn: parent
                visible: grid.count === 0
                text: qsTr("Keine passenden Titel vorhanden.")
                color: page.theme.muted
            }
        }
        Label {
            visible: page.catalogPage && page.showUnassigned
            Layout.fillWidth: true
            text: qsTr("Serien werden beispielsweise über S01E02, 1x02 oder Folge 2 im Ordner Staffel 1 erkannt. Mehrdeutige Dateien kannst du einzeln zuordnen; Originaldateien bleiben unverändert.")
            wrapMode: Text.WordWrap
            color: page.theme.muted
        }
        ListView {
            id: unassignedList
            objectName: "unassignedList"
            visible: page.catalogPage && page.showUnassigned
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            model: library.unassigned
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                required property var entry
                width: ListView.view.width
                height: 100
                radius: 8
                color: page.theme.surface
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            text: entry.path
                            textFormat: Text.PlainText
                            color: page.theme.text
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                        Label {
                            text: entry.diagnostic || "Aus dem Dateinamen nicht eindeutig erkannt"
                            textFormat: Text.PlainText
                            wrapMode: Text.WordWrap
                            color: page.theme.muted
                            Layout.fillWidth: true
                        }
                    }
                    Button {
                        objectName: "assign-file-" + entry.id
                        text: qsTr("Zuordnen")
                        onClicked: page.assignRequested(entry)
                    }
                }
            }
        }
        Item {
            visible: !page.catalogPage && pageTitle !== "Startseite" && pageTitle !== "Favoriten"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Label {
                anchors.centerIn: parent
                text: qsTr("Noch keine Einträge")
                color: page.theme.muted
            }
        }
    }
}
