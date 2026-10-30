import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
Item {
    id: page
    required property QtObject theme
    required property QtObject library
    property string pageTitle: "Startseite"
    property string description: "Deine Filme und Serien. An einem Ort."
    property int optionsVersion: 0
    Connections { target: library; function onOptionsChanged() { page.optionsVersion++ } }
    property bool showUnassigned: false
    readonly property bool catalogPage: pageTitle === "Filme" || pageTitle === "Serien"
    readonly property string kind: pageTitle === "Serien" ? "series" : "movie"
    signal openFileRequested()
    signal foldersRequested()
    signal assignRequested(var entry)
    ColumnLayout {
        anchors.fill: parent; spacing: 14
        RowLayout {
            Layout.fillWidth: true
            Label { text: pageTitle; color: theme.text; font.pixelSize: 30; font.weight: Font.DemiBold; Layout.fillWidth: true }
            Button { text: "Medienordner"; onClicked: page.foldersRequested() }
            Button { text: "Datei abspielen"; onClicked: page.openFileRequested() }
        }
        Label { text: description; color: theme.muted; font.pixelSize: 15 }
        RowLayout {
            visible: page.catalogPage
            Label { text: "Sortierung"; color: theme.muted }
            ComboBox {
                model: ["Titel", "Zuletzt hinzugefügt", "Jahr"]
                readonly property var orders: ["title","recent","year"]
                currentIndex: { page.optionsVersion; return orders.indexOf(library.sortOrder(page.kind)) }
                onActivated: library.setOptions(page.kind,orders[currentIndex],library.availability(page.kind))
                Accessible.name: "Bibliothek sortieren"
            }
            ComboBox {
                objectName: "availabilityFilter"
                model: ["Alle Titel", "Verfügbar", "Nicht verfügbar"]
                readonly property var filters: [-1,1,0]
                currentIndex: { page.optionsVersion; return filters.indexOf(library.availability(page.kind)) }
                onActivated: library.setOptions(page.kind,library.sortOrder(page.kind),filters[currentIndex])
                Accessible.name: "Nach Verfügbarkeit filtern"
            }
            Item { Layout.fillWidth: true }
            Label { text: "Kachelgröße"; color: theme.muted }
            Slider { from: 130; to: 230; value: library.tileSize; Layout.preferredWidth: 120; onMoved: library.tileSize = Math.round(value); Accessible.name: "Kachelgröße" }
            Button { objectName: "unassignedToggle"; text: "Nicht zugeordnet (" + library.unassignedCount + ")"; checkable: true; checked: page.showUnassigned; onClicked: page.showUnassigned = !page.showUnassigned }
        }
        Label { visible: library.busy; text: library.paused ? "Bibliotheksscan pausiert" : "Bibliothek wird im Hintergrund ergänzt · " + library.checked + " Dateien geprüft"; color: theme.accent }
        ScrollView {
            visible: pageTitle === "Startseite"; Layout.fillWidth: true; Layout.fillHeight: true
            contentWidth: availableWidth
            ColumnLayout {
                width: parent.width; spacing: 24
                PosterShelf { theme: page.theme; model: library.recent; heading: "Zuletzt hinzugefügt"; onDetailsRequested: library.openDetails(id); onPlayRequested: library.playTitle(id) }
                PosterShelf { theme: page.theme; model: library.randomMovies; heading: "Meine Filme"; onDetailsRequested: library.openDetails(id); onPlayRequested: library.playTitle(id) }
                PosterShelf { theme: page.theme; model: library.randomSeries; heading: "Meine Serien"; onDetailsRequested: library.openDetails(id); onPlayRequested: library.playTitle(id) }
                Label { visible: library.movieCount + library.seriesCount === 0; text: "Deine Mediathek beginnt hier. Wähle Medienordner, um deine Filme und Serien einzulesen."; wrapMode: Text.WordWrap; color: page.theme.muted; Layout.fillWidth: true; Layout.topMargin: 48 }
            }
        }
        GridView {
            id: grid
            objectName: "movieGrid"
            visible: page.catalogPage && !page.showUnassigned
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            cellWidth: library.tileSize + 18; cellHeight: library.tileSize * 1.68 + 18
            model: page.kind === "movie" ? library.movies : library.series
            ScrollBar.vertical: ScrollBar {}
            delegate: PosterCard {
                theme: page.theme
                width: page.library.tileSize; height: width * 1.68
                onDetailsRequested: page.library.openDetails(entry.id)
                onPlayRequested: page.library.playTitle(entry.id)
            }
            Label { anchors.centerIn: parent; visible: grid.count === 0; text: "Keine passenden Titel vorhanden."; color: page.theme.muted }
        }
        ListView {
            id: unassignedList
            objectName: "unassignedList"
            visible: page.catalogPage && page.showUnassigned
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 10
            model: library.unassigned
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                required property var entry
                width: ListView.view.width; height: 100; radius: 8; color: page.theme.surface
                RowLayout {
                    anchors.fill: parent; anchors.margins: 14
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { text: entry.path; textFormat: Text.PlainText; color: page.theme.text; elide: Text.ElideMiddle; Layout.fillWidth: true }
                        Label { text: entry.diagnostic || "Aus dem Dateinamen nicht eindeutig erkannt"; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: page.theme.muted; Layout.fillWidth: true }
                    }
                    Button { objectName: "assign-file-" + entry.id; text: "Zuordnen"; onClicked: page.assignRequested(entry) }
                }
            }
        }
        Item {
            visible: !page.catalogPage && pageTitle !== "Startseite"
            Layout.fillWidth: true; Layout.fillHeight: true
            Label { anchors.centerIn: parent; text: "Noch keine Einträge"; color: page.theme.muted }
        }
    }
}
