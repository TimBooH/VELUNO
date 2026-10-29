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
        appSettings.theme = "midnight"
        shell = appComponent.createObject(null)
        verify(shell !== null)
        tryCompare(shell, "visible", true)
        wait(30)
    }
    function cleanup() { shell.close(); shell.destroy(); shell = null }
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
