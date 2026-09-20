import XCTest

final class KrebbUITests: XCTestCase {
    override func setUpWithError() throws { continueAfterFailure = false }

    @MainActor
    func testNativeNavigationAndSensorSheet() {
        let app = XCUIApplication()
        app.launch()
        let bar = app.navigationBars["Today"]
        XCTAssertTrue(bar.waitForExistence(timeout: 10))
        XCTAssertTrue(app.tabBars.buttons["Today"].exists)
        let expandedHeight = bar.frame.height
        capture("Today — expanded title", app: app)
        app.scrollViews.firstMatch.swipeUp()
        XCTAssertTrue(app.buttons["responseExplainer"].waitForExistence(timeout: 3))
        XCTAssertLessThan(bar.frame.height, expandedHeight, "System large title should collapse with scrolling")
        capture("Today — collapsed title", app: app)
        app.buttons["responseExplainer"].tap()
        XCTAssertTrue(app.navigationBars["About the response"].waitForExistence(timeout: 3))
        app.navigationBars.buttons.firstMatch.tap()
        app.navigationBars.buttons["Sensors"].tap()
        XCTAssertTrue(app.staticTexts["You’re exploring sample data"].waitForExistence(timeout: 3))
        capture("Sensors — native sheet", app: app)
        app.buttons["Done"].tap()
        XCTAssertTrue(app.tabBars.buttons["Today"].isHittable)
    }

    @MainActor
    func testCheckPreservesProgressAcrossTabs() {
        let app = XCUIApplication()
        app.launch()
        app.buttons["startCheck"].tap()
        XCTAssertTrue(app.staticTexts["A moment to settle."].waitForExistence(timeout: 3))
        app.buttons["advanceCheck"].tap()
        XCTAssertTrue(app.staticTexts["Finding your starting point."].waitForExistence(timeout: 3))
        app.tabBars.buttons["Today"].tap()
        app.tabBars.buttons["Check"].tap()
        XCTAssertTrue(app.staticTexts["Finding your starting point."].exists)
        app.buttons["advanceCheck"].tap()
        XCTAssertTrue(app.staticTexts["Follow the change."].waitForExistence(timeout: 3))
        capture("Check — sample observation", app: app)
        app.buttons["advanceCheck"].tap()
        XCTAssertTrue(app.staticTexts["A check, complete."].waitForExistence(timeout: 3))
    }

    @MainActor
    func testJournalDetailAndBackNavigation() {
        let app = XCUIApplication()
        app.launch()
        app.tabBars.buttons["Journal"].tap()
        XCTAssertTrue(app.navigationBars["Journal"].waitForExistence(timeout: 3))
        app.buttons["sampleSession"].tap()
        XCTAssertTrue(app.navigationBars["After a meal"].waitForExistence(timeout: 3))
        capture("Journal — sample detail", app: app)
        app.navigationBars.buttons.firstMatch.tap()
        XCTAssertTrue(app.navigationBars["Journal"].waitForExistence(timeout: 3))
    }

    @MainActor
    private func capture(_ name: String, app: XCUIApplication) {
        let attachment = XCTAttachment(screenshot: app.screenshot())
        attachment.name = name
        attachment.lifetime = .keepAlways
        add(attachment)
    }
}
