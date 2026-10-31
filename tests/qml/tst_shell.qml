import QtQuick
import QtQuick.Controls
import QtTest
import "../../qml" as App
import "../../qml/components" as Components
import "../../qml/themes" as Themes

TestCase {
    id: testCase
    name: "ApplicationShell"
    when: windowShown
    Component {
        id: appComponent
        App.Main {}
    }
    Themes.Theme {
        id: fixtureColors
    }
    Component {
        id: shelfFixture
        Rectangle {
            width: 940
            height: 400
            color: fixtureColors.background
            property alias rows: fixtureModel
            property alias shelf: fixtureShelf
            property int detailsId: 0
            property int playId: 0
            ListModel {
                id: fixtureModel
            }
            Components.PosterShelf {
                id: fixtureShelf
                anchors.fill: parent
                anchors.margins: 12
                theme: fixtureColors
                model: fixtureModel
                heading: "Meine Filme"
                onDetailsRequested: function (id) {
                    detailsId = id;
                }
                onPlayRequested: function (id) {
                    playId = id;
                }
            }
        }
    }
    property var shell
    function init() {
        testInput.resetSetupForTests();
        appSettings.theme = "midnight";
        shell = appComponent.createObject(null);
        verify(shell !== null);
        tryCompare(shell, "visible", true);
        wait(30);
    }
    function cleanup() {
        shell.close();
        shell.destroy();
        shell = null;
    }
    function test_catalogImportDetailsSearchAndAssignment() {
        library.addSource(testInput.libraryFolder, "movie");
        tryCompare(library, "movieCount", 1, 10000);
        tryCompare(library, "busy", false, 10000);
        compare(library.unassignedCount, 1);
        let matches = library.search("Example");
        compare(matches.length, 1);
        shell.currentPage = "Filme";
        let grid = findChild(shell, "movieGrid");
        tryCompare(grid, "count", 1);
        wait(100);
        grid.forceLayout();
        let card = grid.itemAtIndex(0);
        if (screenshotDirectory.length > 0)
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/library-debug.png");
        verify(card !== null);
        mouseClick(card, 20, 20);
        let detail = findChild(shell, "titleDetails");
        tryCompare(detail, "opened", true);
        compare(library.selectedTitle.title, "Example Movie");
        detail.close();
        let search = findChild(shell, "globalSearch");
        search.forceActiveFocus();
        search.text = "Example";
        compare(search.matches.length, 1);
        let results = findChild(shell, "searchResults");
        let popup = findChild(shell, "searchPopup");
        tryCompare(popup, "opened", true);
        results.forceLayout();
        let result = results.itemAtIndex(0);
        verify(result !== null);
        verify(result.text.indexOf("Example Movie") >= 0);
        mouseMove(result, 20, 20);
        mouseClick(result);
        tryCompare(detail, "opened", true);
        detail.close();
        search.text = "";
        library.playTitle(matches[0].id);
        tryCompare(shell, "playerVisible", true);
        tryCompare(playback, "loading", false, 10000);
        verify(playback.active);
        shell.closePlayer();
        mouseClick(findChild(shell, "unassignedToggle"));
        wait(100);
        let unknownList = findChild(shell, "unassignedList");
        unknownList.forceLayout();
        let assign = findChild(unknownList.itemAtIndex(0), "assign-file-" + testInput.unknownFileId);
        verify(assign !== null);
        mouseClick(assign);
        let assignment = findChild(shell, "assignmentDialog");
        tryCompare(assignment, "opened", true);
        let title = findChild(shell, "assignmentTitle");
        compare(title.text, "");
        title.text = "Own Movie";
        mouseClick(assignment.standardButton(Dialog.Ok));
        tryCompare(library, "unassignedCount", 0);
        compare(library.movieCount, 2);
        library.setOptions("movie", "title", 0);
        tryCompare(grid, "count", 0);
        compare(findChild(shell, "availabilityFilter").currentIndex, 2);
        library.setOptions("movie", "title", -1);
        tryCompare(grid, "count", 2);
        if (screenshotDirectory.length > 0) {
            mouseClick(findChild(shell, "unassignedToggle"));
            waitForRendering(shell.contentItem);
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/library.png");
        }
    }
    function test_wizardRequiresMetadataDecision() {
        testInput.restartSetupForTests();
        let wizard = findChild(shell, "setupWizard");
        tryCompare(wizard, "opened", true);
        let next = findChild(shell, "wizardNext");
        compare(library.wizardStep, 0);
        mouseClick(next);
        compare(library.wizardStep, 1);
        mouseClick(next);
        compare(library.wizardStep, 2);
        waitForRendering(shell.contentItem);
        mouseClick(findChild(wizard, "addSmbButton"));
        let smbDialog = findChild(wizard, "smbSourceDialog");
        tryCompare(smbDialog, "opened", true);
        let smbAddress = findChild(wizard, "smbAddress");
        smbAddress.text = "smb://NAS/Filme";
        verify(findChild(wizard, "confirmSmbSource").enabled);
        smbDialog.close();
        mouseClick(findChild(wizard, "addFolderButton"));
        let folderPicker = findChild(wizard, "mediaFolderPicker");
        verify(folderPicker !== null);
        tryCompare(folderPicker, "visible", true);
        wait(100);
        folderPicker.reject();
        tryCompare(folderPicker, "visible", false);
        mouseClick(next);
        compare(library.wizardStep, 3);
        verify(!next.enabled);
        waitForRendering(shell.contentItem);
        let offline = findChild(shell, "offlineChoice");
        if (screenshotDirectory.length > 0)
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/wizard-debug.png");
        mouseClick(offline);
        compare(library.onlineConsent, "denied");
        verify(next.enabled);
        mouseClick(next);
        compare(library.wizardStep, 4);
        mouseClick(next);
        compare(library.wizardStep, 5);
        mouseClick(next);
        tryCompare(library, "setupComplete", true);
        tryCompare(wizard, "opened", false);
    }
    function test_languageAndNewSettingsPages() {
        shell.currentPage = "Einstellungen";
        let page = findChild(shell, "settingsPage");
        verify(page !== null);
        for (let index = 2; index <= 6; ++index) {
            page.section = index;
            waitForRendering(shell.contentItem);
            verify(shell.visible);
            if (index === 2) {
                let device = findChild(shell, "audioDeviceSelector");
                verify(device.count >= 1);
                verify(device.displayText.length > 0);
            }
            if (screenshotDirectory.length > 0)
                grabImage(shell.contentItem.parent).save(screenshotDirectory + "/settings-" + index + ".png");
        }
        localization.language = "en";
        tryCompare(localization, "language", "en");
        tryCompare(findChild(shell, "settingsSectionTitle"), "text", "About VELUNO");
        waitForRendering(shell.contentItem);
        localization.language = "de";
        tryCompare(localization, "language", "de");
        tryCompare(findChild(shell, "settingsSectionTitle"), "text", "Über VELUNO");
        page.section = 3;
        waitForRendering(shell.contentItem);
        mouseClick(findChild(shell, "newProfile"));
        let editor = findChild(shell, "profileEditorName");
        verify(editor !== null);
        editor.text = "Testprofil";
        verify(editor.width <= findChild(shell, "profileEditor").availableWidth + 1);
        // The backend suite verifies creation/deletion and strict profile isolation.
        if (screenshotDirectory.length > 0)
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/profiles.png");
    }
    function test_metadataSwitchAndTokenAreIndependent() {
        shell.currentPage = "Einstellungen";
        let settings = findChild(shell, "settingsPage");
        settings.section = 1;
        waitForRendering(shell.contentItem);
        let toggle = findChild(shell, "tmdbEnabledSwitch");
        let refresh = findChild(shell, "tmdbRefresh");
        let token = findChild(shell, "tmdbToken");
        let save = findChild(shell, "tmdbSaveToken");
        verify(toggle !== null);
        compare(toggle.checked, false);
        verify(!refresh.enabled);
        // Saving a token does not grant network consent.
        token.text = "qml-test-token";
        mouseClick(save);
        compare(token.text, "");
        verify(metadata.configured);
        compare(toggle.checked, false);
        mouseClick(toggle);
        tryCompare(metadata, "enabled", true);
        compare(library.onlineConsent, "granted");
        verify(refresh.enabled);
        mouseClick(toggle);
        tryCompare(metadata, "enabled", false);
        verify(!refresh.enabled);
        verify(metadata.configured);
        // Reopening settings retains the off state and leaves the token hidden.
        shell.currentPage = "Startseite";
        shell.currentPage = "Einstellungen";
        compare(toggle.checked, false);
        compare(token.text, "");
        appSettings.theme = "light";
        waitForRendering(shell.contentItem);
        if (screenshotDirectory.length > 0)
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/tmdb-settings-light.png");
        appSettings.theme = "midnight";
        waitForRendering(shell.contentItem);
        if (screenshotDirectory.length > 0)
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/tmdb-settings.png");
        metadata.setToken("");
    }
    function test_navigation() {
        let buttons = [];
        function collect(item) {
            if (item.text !== undefined && shell.navigation.indexOf(item.text) >= 0 && item.clicked !== undefined)
                buttons.push(item);
            for (let i = 0; i < item.children.length; ++i)
                collect(item.children[i]);
        }
        collect(shell.header);
        compare(buttons.length, 5);
        for (let button of buttons) {
            mouseClick(button);
            compare(shell.currentPage, button.text);
        }
    }
    function test_playerControlsAndOverlay() {
        shell.playerVisible = true;
        playback.openFile(testInput.videoFixture);
        tryCompare(playback, "loading", false, 10000);
        verify(playback.active);
        compare(playback.error, "");
        let page = findChild(shell, "playerPage");
        mouseClick(findChild(shell, "playbackDiagnosticsButton"));
        let diagnostics = findChild(shell, "playbackDiagnostics");
        tryCompare(diagnostics, "opened", true);
        verify(playback.diagnostics.rendering.length > 0);
        if (screenshotDirectory.length > 0) {
            waitForRendering(shell.contentItem);
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/playback-diagnostics.png");
        }
        mouseClick(findChild(shell, "playbackDiagnosticsClose"));
        tryCompare(diagnostics, "opened", false);
        mouseClick(findChild(shell, "trackMenuButton"));
        let tracks = findChild(shell, "playerTracks");
        tryCompare(tracks, "opened", true);
        playback.openFile(testInput.videoFixture);
        tryCompare(playback, "loading", false, 10000);
        for (let size of [[980, 600], [1240, 800], [1050, 650], [1240, 800]]) {
            shell.width = size[0];
            shell.height = size[1];
            waitForRendering(shell.contentItem);
        }
        verify(tracks.opened);
        mouseClick(findChild(shell, "tracksCloseButton"));
        tryCompare(tracks, "opened", false);
        tryCompare(page, "overlayVisible", false, 4500);
        mouseMove(page, page.width / 2, page.height / 2);
        tryCompare(page, "overlayVisible", true);
        mouseClick(findChild(shell, "pauseButton"));
        tryCompare(playback, "paused", true);
        wait(2100);
        verify(page.overlayVisible);
        let timeline = findChild(shell, "timeline");
        waitForRendering(shell.contentItem);
        mouseClick(timeline, timeline.width / 2, timeline.height / 2);
        tryVerify(function () {
            return playback.position > 5 && playback.position < 9;
        });
        testInput.pressKey(shell, Qt.Key_M);
        tryCompare(playback, "muted", true);
        testInput.pressKey(shell, Qt.Key_F);
        tryCompare(shell, "visibility", Window.FullScreen);
        testInput.pressKey(shell, Qt.Key_Escape);
        tryCompare(shell, "visibility", Window.Windowed);
        shell.closePlayer();
        compare(playback.active, false);
        compare(shell.playerVisible, false);
    }
    function test_timelineHandleKeepsPositionOnPress() {
        shell.playerVisible = true;
        playback.openFile(testInput.videoFixture);
        tryCompare(playback, "loading", false, 10000);
        tryVerify(function () {
            return playback.duration > 13;
        });
        playback.togglePause();
        tryCompare(playback, "paused", true);
        playback.seek(9);
        tryVerify(function () {
            return playback.position > 8.8;
        });
        let timeline = findChild(shell, "timeline");
        waitForRendering(shell.contentItem);
        let before = timeline.value;
        mousePress(timeline, timeline.handle.x + timeline.handle.width / 2, timeline.handle.y + timeline.handle.height / 2);
        let held = timeline.value;
        mouseRelease(timeline, timeline.handle.x + timeline.handle.width / 2, timeline.handle.y + timeline.handle.height / 2);
        verify(Math.abs(held - before) < 0.3, "Pressing the seek handle must not reset its position");
        shell.closePlayer();
    }
    function test_profileSettingsAndTheme() {
        let profile = findChild(shell, "profileButton");
        verify(profile !== null);
        mouseClick(profile);
        let menu = findChild(shell, "profileMenu");
        tryCompare(menu, "opened", true);
        let item = findChild(shell, "settingsMenuItem");
        verify(item !== null);
        mouseClick(item);
        compare(shell.currentPage, "Einstellungen");
        compare(findChild(shell, "settingsNavigation").width, 180);
        let selector = findChild(shell, "themeSelector");
        selector.forceActiveFocus();
        testInput.pressKey(shell, Qt.Key_Space);
        tryCompare(selector.popup, "opened", true);
        testInput.pressKey(shell, Qt.Key_Down);
        testInput.pressKey(shell, Qt.Key_Return);
        tryCompare(appSettings, "theme", "light");
        compare(shell.color, "#f4f6fa");
        if (screenshotDirectory.length > 0) {
            waitForRendering(shell.contentItem);
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/light.png");
        }
        testInput.resetSetupForTests();
        appSettings.theme = "midnight";
        compare(shell.color, "#080f1c");
        if (screenshotDirectory.length > 0) {
            waitForRendering(shell.contentItem);
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/midnight.png");
        }
    }
    function test_streamingShelfArtworkFallbackAndNavigation() {
        let fixture = createTemporaryObject(shelfFixture, shell.contentItem);
        verify(fixture !== null);
        let titles = ["Orbit", "Nachtzug", "Gezeiten", "Fernweh", "Licht", "Morgen"];
        for (let i = 0; i < 12; ++i)
            fixture.rows.append({
                entry: {
                    id: i + 1,
                    title: titles[i % 6],
                    year: 2020 + i % 6,
                    kind: "movie",
                    available: true,
                    poster: i === 11 ? "images/nonexistent.png" : testInput.artworkFixture(i % 6)
                }
            });
        let row = findChild(fixture, "posterRow");
        row.forceLayout();
        tryCompare(row, "count", 12);
        let next = findChild(fixture, "shelfNext");
        let previous = findChild(fixture, "shelfPrevious");
        verify(next.enabled);
        verify(!previous.enabled);
        let card = row.itemAtIndex(0);
        verify(card !== null);
        tryCompare(findChild(card, "posterArtwork"), "status", Image.Ready);
        mouseClick(card, 20, 20);
        compare(fixture.detailsId, 1);
        mouseMove(card, card.width / 2, 70);
        tryCompare(card, "hovered", true);
        let play = findChild(card, "posterPlay");
        mouseClick(play);
        compare(fixture.playId, 1);
        compare(fixture.detailsId, 1);
        mouseMove(next);
        if (screenshotDirectory.length > 0) {
            waitForRendering(fixture);
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/streaming-shelf-fixture.png");
        }
        fixtureColors.light = true;
        waitForRendering(fixture);
        if (screenshotDirectory.length > 0)
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/streaming-shelf-light-fixture.png");
        fixtureColors.light = false;
        mouseClick(next);
        tryVerify(function () {
            return row.contentX > 100;
        });
        wait(260);
        verify(previous.enabled);
        row.forceActiveFocus();
        keyClick(Qt.Key_Left);
        tryVerify(function () {
            return row.contentX < 1;
        });
        wait(260);
        row.forceActiveFocus();
        keyClick(Qt.Key_End);
        wait(40);
        row.forceLayout();
        tryCompare(next, "enabled", false);
        wait(300);
        tryVerify(function () {
            return row.itemAtIndex(11) !== null;
        });
        let fallback = row.itemAtIndex(11);
        verify(fallback !== null);
        tryCompare(fallback, "artworkStatus", Image.Error);
        mouseClick(fallback, 20, 20);
        compare(fixture.detailsId, 12);
        row.forceActiveFocus();
        keyClick(Qt.Key_Home);
        tryCompare(previous, "enabled", false);
        fixture.rows.clear();
        tryCompare(fixture.shelf, "visible", false);
        wait(30);
    }
    function revealDetailControl(control) {
        let list = findChild(shell, "titleDetailsScroll");
        list.contentY = control.mapToItem(list.contentItem, 0, 0).y - 24;
        list.returnToBounds();
        wait(300);
    }
    function test_detailMoviePresentationAndPlayback() {
        let id = library.search("Example Movie")[0].id;
        metadata.edit(id, "Example Movie", "Ein Film für den lokalen Oberflächentest. Die Beschreibung bleibt auch ohne Internet verfügbar.", 2020);
        metadata.selectLocalImage(id, "backdrop", testInput.backdropFixtureUrl());
        testInput.seedDetailProgress(id, 0, 0, 6);
        library.openDetails(id);
        let detail = findChild(shell, "titleDetails");
        tryCompare(detail, "opened", true);
        compare(findChild(shell, "titleHeading").text, "Example Movie");
        verify(findChild(shell, "titleOverview").text.indexOf("Oberflächentest") >= 0);
        let artwork = findChild(shell, "titleBackdrop");
        tryCompare(artwork, "status", Image.Ready);
        let list = findChild(shell, "titleDetailsScroll");
        compare(list.count, 0); // File paths are not the default presentation.
        compare(findChild(shell, "titlePrimaryPlay").text, "Fortsetzen");
        verify(findChild(shell, "titleResumePosition").text.indexOf("0:06") >= 0);
        let favorite = findChild(shell, "titleFavorite");
        let wasFavorite = personal.isFavorite(id);
        mouseClick(favorite);
        compare(personal.isFavorite(id), !wasFavorite);
        favorite.forceActiveFocus();
        testInput.pressKey(shell, Qt.Key_Space);
        compare(personal.isFavorite(id), wasFavorite);
        if (screenshotDirectory.length > 0) {
            waitForRendering(shell.contentItem);
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/title-details-midnight.png");
        }
        appSettings.theme = "light";
        tryCompare(findChild(shell, "titlePrimaryPlayLabel"), "color", "#101722");
        wait(40);
        if (screenshotDirectory.length > 0) {
            waitForRendering(shell.contentItem);
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/title-details-light.png");
        }
        appSettings.theme = "midnight";
        list.positionViewAtEnd(); // Scroll to the content actions.
        waitForRendering(shell.contentItem);
        revealDetailControl(findChild(shell, "titleFilesToggle"));
        mouseClick(findChild(shell, "titleFilesToggle"));
        tryCompare(detail, "showFiles", true);
        tryCompare(list, "count", 1);
        list.positionViewAtIndex(0, ListView.Beginning);
        list.forceLayout();
        tryVerify(function () {
            return list.itemAtIndex(0) !== null;
        });
        compare(list.itemAtIndex(0).modelData.path.indexOf("Example.Movie.2020.mp4") >= 0, true);
        let fileRow = list.itemAtIndex(0);
        mouseClick(findChild(fileRow, "titleFileAssign-" + fileRow.modelData.id));
        let assignment = findChild(shell, "assignmentDialog");
        tryCompare(assignment, "opened", true);
        assignment.reject();
        // The header's primary action uses the same player and progress tracking.
        list.positionViewAtBeginning();
        waitForRendering(shell.contentItem);
        mouseClick(findChild(shell, "titlePrimaryPlay"));
        tryCompare(shell, "playerVisible", true);
        tryCompare(detail, "opened", false);
        tryCompare(playback, "loading", false, 10000);
        verify(playback.active);
        shell.closePlayer();
        library.openDetails(id);
        tryCompare(detail, "opened", true);
        compare(detail.showFiles, false);
        mouseClick(findChild(shell, "titleDetailsClose"));
        tryCompare(detail, "opened", false);
    }
    function test_detailSeriesSeasonsAndEditing() {
        let id = testInput.detailSeriesFixture();
        library.openDetails(id);
        let detail = findChild(shell, "titleDetails");
        tryCompare(detail, "opened", true);
        let list = findChild(shell, "titleDetailsScroll");
        tryCompare(list, "count", 2);
        compare(detail.seasons.length, 2);
        // No artwork is required for offline title and episode controls.
        compare(findChild(shell, "titleHeading").text, "Orbit");
        mouseClick(findChild(shell, "titlePrimaryPlay"));
        let selector = findChild(shell, "titleSeasonSelector");
        revealDetailControl(selector);
        selector.forceActiveFocus();
        testInput.pressKey(shell, Qt.Key_Space);
        tryCompare(selector.popup, "opened", true);
        testInput.pressKey(shell, Qt.Key_Down);
        testInput.pressKey(shell, Qt.Key_Return);
        tryCompare(detail, "season", 2);
        tryCompare(list, "count", 1);
        list.positionViewAtBeginning();
        revealDetailControl(findChild(shell, "titleEdit"));
        mouseClick(findChild(shell, "titleEdit"));
        let editor = findChild(shell, "titleInfoEditor");
        tryCompare(editor, "opened", true);
        editor.close();
        revealDetailControl(findChild(shell, "titleCollectionsButton"));
        mouseClick(findChild(shell, "titleCollectionsButton"));
        let collections = findChild(shell, "titleCollections");
        tryCompare(collections, "opened", true);
        collections.close();
        // Play the explicitly selected season's episode.
        list.positionViewAtIndex(0, ListView.Beginning);
        list.forceLayout();
        let row = list.itemAtIndex(0);
        verify(row !== null);
        compare(row.modelData.season, 2);
        mouseClick(findChild(row, "titleFilePlay-" + row.modelData.id));
        tryCompare(shell, "playerVisible", true);
        tryCompare(detail, "opened", false);
        tryCompare(playback, "loading", false, 10000);
        compare(playback.error, "");
        verify(playback.active);
        shell.closePlayer();
        shell.width = 980;
        shell.height = 600;
        library.openDetails(id);
        tryCompare(detail, "opened", true);
        tryVerify(function () {
            return detail.width <= shell.width - 48;
        });
        tryVerify(function () {
            return detail.height <= shell.height - 48;
        });
        if (screenshotDirectory.length > 0) {
            waitForRendering(shell.contentItem);
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/title-details-series-compact.png");
        }
        testInput.pressKey(shell, Qt.Key_Escape);
        tryCompare(detail, "opened", false);
    }
    function test_homeArtworkSetupAndScanErrorsReachSettings() {
        let configure = findChild(shell, "configureArtwork");
        verify(configure.visible);
        mouseClick(configure);
        compare(shell.currentPage, "Einstellungen");
        compare(findChild(shell, "settingsPage").section, 1);
        shell.currentPage = "Startseite";
        mouseClick(findChild(shell, "scanStatusButton"));
        compare(shell.currentPage, "Einstellungen");
        compare(findChild(shell, "settingsPage").section, 1);
    }
    function test_resumeSeriesFromTitleDetails() {
        let id = testInput.detailSeriesFixture();
        testInput.seedDetailProgress(id, 2, 1, 6);
        library.openDetails(id);
        let detail = findChild(shell, "titleDetails");
        tryCompare(detail, "opened", true);
        compare(findChild(shell, "titlePrimaryPlay").text, "Fortsetzen");
        revealDetailControl(findChild(shell, "titlePrimaryPlay"));
        mouseClick(findChild(shell, "titlePrimaryPlay"));
        tryCompare(shell, "playerVisible", true);
        tryCompare(playback, "loading", false, 10000);
        tryVerify(function () {
            return playback.position > 5.5;
        });
        verify(playback.title.indexOf("S2E1") >= 0);
        shell.closePlayer();
    }
    function test_regularWindow() {
        compare(shell.title, "VELUNO");
        if (screenshotDirectory.length > 0) {
            waitForRendering(shell.contentItem);
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/startseite.png");
        }
        shell.width = 1050;
        shell.height = 650;
        compare(shell.width, 1050);
        compare(shell.height, 650);
        verify(shell.visible);
        shell.close();
        compare(shell.visible, false);
    }
}
