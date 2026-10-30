import QtQuick
import QtQuick.Controls
import QtTest
import "../../qml" as App
TestCase {
    id: testCase
    name: "ApplicationShell"
    when: windowShown
    Component { id: appComponent; App.Main {} }
    property var shell
    function init() {
        testInput.resetSetupForTests()
        appSettings.theme = "midnight"
        shell = appComponent.createObject(null)
        verify(shell !== null)
        tryCompare(shell, "visible", true)
        wait(30)
    }
    function cleanup() { shell.close(); shell.destroy(); shell = null }
    function test_catalogImportDetailsSearchAndAssignment() {
        library.addSource(testInput.libraryFolder, "movie")
        tryCompare(library, "movieCount", 1, 10000)
        tryCompare(library, "busy", false, 10000)
        compare(library.unassignedCount, 1)
        let matches = library.search("Example")
        compare(matches.length, 1)
        shell.currentPage = "Filme"
        let grid = findChild(shell, "movieGrid")
        tryCompare(grid, "count", 1)
        wait(100)
        grid.forceLayout()
        let card = grid.itemAtIndex(0)
        if (screenshotDirectory.length > 0) grabImage(shell.contentItem.parent).save(screenshotDirectory + "/library-debug.png")
        verify(card !== null)
        mouseClick(card, 20, 20)
        let detail = findChild(shell, "titleDetails")
        tryCompare(detail, "opened", true)
        compare(library.selectedTitle.title, "Example Movie")
        detail.close()
        let search = findChild(shell, "globalSearch")
        search.forceActiveFocus()
        search.text = "Example"
        compare(search.matches.length, 1)
        let results = findChild(shell, "searchResults")
        let popup = findChild(shell, "searchPopup")
        tryCompare(popup, "opened", true)
        results.forceLayout()
        let result = results.itemAtIndex(0)
        verify(result !== null)
        mouseMove(result, 20, 20)
        mouseClick(result)
        tryCompare(detail, "opened", true)
        detail.close()
        search.text = ""
        library.playTitle(matches[0].id)
        tryCompare(shell, "playerVisible", true)
        tryCompare(playback, "loading", false, 10000)
        verify(playback.active)
        shell.closePlayer()
        mouseClick(findChild(shell, "unassignedToggle"))
        wait(100)
        let unknownList = findChild(shell, "unassignedList")
        unknownList.forceLayout()
        let assign = findChild(unknownList.itemAtIndex(0), "assign-file-" + testInput.unknownFileId)
        verify(assign !== null)
        mouseClick(assign)
        let assignment = findChild(shell, "assignmentDialog")
        tryCompare(assignment, "opened", true)
        let title = findChild(shell, "assignmentTitle")
        compare(title.text, "")
        title.text = "Own Movie"
        mouseClick(assignment.standardButton(Dialog.Ok))
        tryCompare(library, "unassignedCount", 0)
        compare(library.movieCount, 2)
        library.setOptions("movie", "title", 0)
        tryCompare(grid, "count", 0)
        compare(findChild(shell, "availabilityFilter").currentIndex, 2)
        library.setOptions("movie", "title", -1)
        tryCompare(grid, "count", 2)
        if (screenshotDirectory.length > 0) {
            mouseClick(findChild(shell, "unassignedToggle"))
            waitForRendering(shell.contentItem)
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/library.png")
        }
    }
    function test_wizardRequiresMetadataDecision() {
        testInput.restartSetupForTests()
        let wizard = findChild(shell, "setupWizard")
        tryCompare(wizard, "opened", true)
        let next = findChild(shell, "wizardNext")
        compare(library.wizardStep, 0)
        mouseClick(next)
        compare(library.wizardStep, 1)
        mouseClick(next)
        compare(library.wizardStep, 2)
        waitForRendering(shell.contentItem)
        mouseClick(findChild(wizard, "addSmbButton"))
        let smbDialog = findChild(wizard, "smbSourceDialog")
        tryCompare(smbDialog, "opened", true)
        let smbAddress = findChild(wizard, "smbAddress")
        smbAddress.text = "smb://NAS/Filme"
        verify(findChild(wizard, "confirmSmbSource").enabled)
        smbDialog.close()
        mouseClick(findChild(wizard, "addFolderButton"))
        let folderPicker = findChild(wizard, "mediaFolderPicker")
        verify(folderPicker !== null)
        tryCompare(folderPicker, "visible", true)
        wait(100)
        folderPicker.reject()
        tryCompare(folderPicker, "visible", false)
        mouseClick(next)
        compare(library.wizardStep, 3)
        verify(!next.enabled)
        waitForRendering(shell.contentItem)
        let offline = findChild(shell, "offlineChoice")
        if (screenshotDirectory.length > 0) grabImage(shell.contentItem.parent).save(screenshotDirectory + "/wizard-debug.png")
        mouseClick(offline)
        compare(library.onlineConsent, "denied")
        verify(next.enabled)
        mouseClick(next)
        compare(library.wizardStep, 4)
        mouseClick(next)
        compare(library.wizardStep, 5)
        mouseClick(next)
        tryCompare(library, "setupComplete", true)
        tryCompare(wizard, "opened", false)
    }
    function test_navigation() {
        let buttons = []
        function collect(item) {
            if (item.text !== undefined && shell.navigation.indexOf(item.text) >= 0 && item.clicked !== undefined) buttons.push(item)
            for (let i = 0; i < item.children.length; ++i) collect(item.children[i])
        }
        collect(shell.header)
        compare(buttons.length, 5)
        for (let button of buttons) {
            mouseClick(button)
            compare(shell.currentPage, button.text)
        }
    }
    function test_playerControlsAndOverlay() {
        shell.playerVisible = true
        playback.openFile(testInput.videoFixture)
        tryCompare(playback, "loading", false, 10000)
        verify(playback.active)
        compare(playback.error, "")
        let page = findChild(shell, "playerPage")
        tryCompare(page, "overlayVisible", false, 4500)
        mouseMove(page, page.width / 2, page.height / 2)
        tryCompare(page, "overlayVisible", true)
        mouseClick(findChild(shell, "pauseButton"))
        tryCompare(playback, "paused", true)
        wait(2100)
        verify(page.overlayVisible)
        let timeline = findChild(shell, "timeline")
        mouseClick(timeline, timeline.width / 2, timeline.height / 2)
        tryVerify(function() { return playback.position > 5 && playback.position < 9 })
        testInput.pressKey(shell, Qt.Key_M)
        tryCompare(playback, "muted", true)
        testInput.pressKey(shell, Qt.Key_F)
        tryCompare(shell, "visibility", Window.FullScreen)
        testInput.pressKey(shell, Qt.Key_Escape)
        tryCompare(shell, "visibility", Window.Windowed)
        shell.closePlayer()
        compare(playback.active, false)
        compare(shell.playerVisible, false)
    }
    function test_profileSettingsAndTheme() {
        let profile = findChild(shell, "profileButton")
        verify(profile !== null)
        mouseClick(profile)
        let menu = findChild(shell, "profileMenu")
        tryCompare(menu, "opened", true)
        let item = findChild(shell, "settingsMenuItem")
        verify(item !== null)
        mouseClick(item)
        compare(shell.currentPage, "Einstellungen")
        compare(findChild(shell, "settingsNavigation").width, 180)
        let selector = findChild(shell, "themeSelector")
        selector.forceActiveFocus()
        testInput.pressKey(shell, Qt.Key_Space)
        tryCompare(selector.popup, "opened", true)
        testInput.pressKey(shell, Qt.Key_Down)
        testInput.pressKey(shell, Qt.Key_Return)
        tryCompare(appSettings, "theme", "light")
        compare(shell.color, "#f4f6fa")
        if (screenshotDirectory.length > 0) {
            waitForRendering(shell.contentItem)
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/light.png")
        }
        testInput.resetSetupForTests()
        appSettings.theme = "midnight"
        compare(shell.color, "#0b1426")
        if (screenshotDirectory.length > 0) {
            waitForRendering(shell.contentItem)
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/midnight.png")
        }
    }
    function test_regularWindow() {
        compare(shell.title, "VELUNO")
        if (screenshotDirectory.length > 0) {
            waitForRendering(shell.contentItem)
            grabImage(shell.contentItem.parent).save(screenshotDirectory + "/startseite.png")
        }
        shell.width = 1050; shell.height = 650
        compare(shell.width, 1050); compare(shell.height, 650)
        verify(shell.visible)
        shell.close()
        compare(shell.visible, false)
    }
}
