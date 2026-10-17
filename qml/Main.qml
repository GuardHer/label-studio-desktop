import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtCore

ApplicationWindow {
    id: window
    width: 1440; height: 900
    minimumWidth: 1060; minimumHeight: 700
    visible: true
    title: (annotationDocument.dirty ? "● " : "") + qsTr("Label Studio")
    color: Theme.background
    font.family: languageManager.code === "zh_CN" ? "Microsoft YaHei UI" : "Figtree"; font.pixelSize: 14
    Material.theme: Theme.dark ? Material.Dark : Material.Light
    Material.accent: Theme.accent; Material.primary: Theme.surface
    Material.background: Theme.surface; Material.foreground: Theme.text
    property string page: "login"
    property bool demoShown: false
    property alias activeLabel: workspace.activeLabel
    property var pendingAction: null
    property var pendingFiles: []
    property string pendingUrl: ""
    property bool settingsAfterOpen: false
    property bool quickView: false
    property bool pendingQuickView: false
    onPageChanged: if (page !== "annotate") { quickView = false; pendingQuickView = false }
    Settings { id: preferences; property bool webDarkMode: false }
    Component.onCompleted: Theme.dark = preferences.webDarkMode
    function guard(action) {
        if (backend.busy) return
        if (annotationDocument.dirty || workspace.hasDraft || (window.page === "settings" && settingsPage.dirty) || (window.page === "account" && accountPage.dirty)) { pendingAction = action; discardDialog.open() }
        else action()
    }
    function navigate(destination) { guard(function() { window.page = destination }) }
    function openAccount(section) { guard(function() { window.page = "account"; accountPage.open(section) }) }
    function openProject(id) { guard(function() { window.settingsAfterOpen=false; if (!backend.demo) backend.openProject(id); else window.page = "project" }) }
    function openProjectSettings(id) { guard(function() { window.settingsAfterOpen=true;backend.openProject(id) }) }
    function openTask(id) { guard(function() { window.pendingQuickView = true; if (!backend.demo) backend.openTask(id); else { window.quickView = true; window.pendingQuickView = false; window.page = "annotate" } }) }
    onClosing: event => {
        if (backend.busy) { event.accepted = false; return }
        if (annotationDocument.dirty || workspace.hasDraft || (window.page === "settings" && settingsPage.dirty) || (window.page === "account" && accountPage.dirty)) { event.accepted = false; guard(function() { window.close() }) }
    }
    Connections {
        target: backend
        function onChanged() {
            if (backend.demo && !window.demoShown) { window.demoShown = true; window.page = "annotate" }
            if (backend.connected && window.page === "login") window.page = "home"
            if (!backend.connected && window.page !== "login") window.page = "login"
            if (!backend.busy && window.pendingFiles.length) {
                const files = window.pendingFiles; window.pendingFiles = []
                Qt.callLater(function() { backend.importFiles(files) })
            } else if (!backend.busy && window.pendingUrl.length) {
                const address = window.pendingUrl; window.pendingUrl = ""
                Qt.callLater(function() { backend.importUrl(address) })
            }
        }
        function onProjectCreated(id) {
            window.pendingFiles = creation.stagedFiles.slice()
            window.pendingUrl = creation.stagedFiles.length ? "" : creation.stagedUrl
            creation.close(); window.page = "project"
        }
        function onProjectOpened() { if(window.settingsAfterOpen){window.settingsAfterOpen=false;settingsPage.load();window.page="settings"}else window.page = "project" }
        function onTaskOpened() { if (window.pendingQuickView) window.quickView = true; else if (window.page !== "annotate") window.quickView = false; window.pendingQuickView = false; window.page = "annotate" }
        function onProjectDeleted() { window.page = "projects" }
    }
    header: ToolBar {
        visible: window.page !== "login"
        height: visible ? 48 : 0
        background: Rectangle { color: Theme.surface; Rectangle { width: parent.width; height: 1; anchors.bottom: parent.bottom; color: Theme.border } }
        RowLayout {
            anchors.fill: parent; spacing: 0
            Item {
                Layout.preferredWidth: 240; Layout.fillHeight: true
                Image { source: Theme.dark ? "qrc:/assets/logo-dark.svg" : "qrc:/assets/logo.svg"; x: 20; anchors.verticalCenter: parent.verticalCenter; width: 145; height: 24; MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: window.navigate("home") } }
                StudioButton { x: 188; anchors.verticalCenter: parent.verticalCenter; glyph: "hamburger"; quiet: true; width: 40; onClicked: navigation.open() }
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.border }
            }
            StudioButton { text: window.page === "home" ? qsTr("Home") : qsTr("Projects"); quiet: true; Layout.leftMargin: 10; onClicked: window.navigate(window.page === "home" ? "home" : "projects") }
            Label { text: "/"; color: Theme.muted; visible: ["project","settings","annotate"].indexOf(window.page) >= 0; Layout.leftMargin: 8; Layout.rightMargin: 8 }
            StudioButton { text: backend.project.title || qsTr("Project"); visible: ["project","settings","annotate"].indexOf(window.page) >= 0; quiet: true; elide: true; Layout.maximumWidth: Math.max(100, Math.min(350, window.width - 860)); onClicked: window.navigate("project") }
            Label { text: "/  " + (window.page === "settings" ? qsTr("Settings") : qsTr("Task #") + backend.taskId); visible: window.page === "settings" || window.page === "annotate"; color: Theme.muted }
            Item { Layout.fillWidth: true }
            StudioButton { text: qsTr("Create"); primary: true; visible: window.page === "projects"; onClicked: creation.open() }
            StudioButton { text: qsTr("Settings"); visible: window.page === "project" || window.page === "annotate"; quiet: true; onClicked: window.guard(function() { settingsPage.load(); window.page = "settings" }) }
            StudioButton { text: "⌨"; quiet: true; onClicked: shortcuts.open(); ToolTip.visible: hovered; ToolTip.text: qsTr("Keyboard shortcuts") }
            LanguageSelector { objectName: "headerLanguageSelector"; Layout.leftMargin: 8 }
            StudioButton { objectName: "themeButton"; text: Theme.dark ? qsTr("☾  Dark") : qsTr("☀  Light"); quiet: true; Layout.leftMargin: 8; onClicked: { Theme.dark = !Theme.dark; preferences.webDarkMode = Theme.dark } }
            RoundButton { text: backend.user.email ? String(backend.user.email).slice(0,2).toUpperCase() : backend.demo ? "DE" : "LS"; font.pixelSize: 11; implicitWidth: 28; implicitHeight: 28; flat: true; background: Rectangle { radius: 14; color: Theme.dark ? "#72524f" : "#e7b8b7" } Layout.leftMargin: 8; Layout.rightMargin: 12; onClicked: accountMenu.open(); ToolTip.visible: hovered; ToolTip.text: backend.user.email || qsTr("Account") }
        }
    }
    Item {
        anchors.fill: parent
        LoginPage { anchors.fill: parent; visible: window.page === "login"; onTokenRequested: workspace.openConnection() }
        HomePage { anchors.fill: parent; visible: window.page === "home"; onCreateRequested: creation.open(); onOpenRequested: id => window.openProject(id); onInviteRequested: window.openAccount(2) }
        ProjectsPage { anchors.fill: parent; visible: window.page === "projects"; onCreateRequested: creation.open(); onOpenRequested: id => window.openProject(id); onSettingsRequested: id => window.openProjectSettings(id) }
        SplitView {
            id: taskSplit
            objectName: "taskQuickViewSplit"
            anchors.fill: parent
            orientation: Qt.Horizontal
            visible: window.page === "project" || window.page === "annotate"
            ProjectPage {
                objectName: "projectPage"
                visible: window.page === "project" || (window.page === "annotate" && window.quickView)
                SplitView.minimumWidth: window.quickView ? 200 : 0
                SplitView.preferredWidth: window.quickView ? Math.max(200, window.width * 0.32) : taskSplit.width
                SplitView.fillWidth: !window.quickView
                onImportRequested: importer.open()
                onSettingsRequested: window.guard(function() { settingsPage.load(); window.page = "settings" })
                onTaskRequested: id => window.openTask(id)
                onTaskActionRequested: actionId => window.guard(function() { backend.dataManager.prepareAction(actionId) })
                onLabelingRequested: window.guard(function() { window.quickView = false; window.pendingQuickView = false; backend.startLabeling() })
            }
            Workspace {
                id: workspace
                visible: window.page === "annotate"
                SplitView.minimumWidth: 760
                SplitView.fillWidth: true
                quickView: window.quickView
                onExpandRequested: window.quickView = false
                onCloseRequested: window.guard(function() { window.quickView = false; window.page = "project" })
            }
        }
        ProjectSettings { id: settingsPage; anchors.fill: parent; visible: window.page === "settings" }
        AccountPage { id: accountPage; anchors.fill: parent; visible: window.page === "account" }
        ProgressBar { anchors.top: parent.top; width: parent.width; height: 3; visible: backend.busy; indeterminate: true }
        Rectangle {
            anchors.bottom: parent.bottom; anchors.bottomMargin: 20; anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(parent.width - 64, 760); height: notification.implicitHeight + 28; radius: 6
            visible: backend.message.length > 0 && window.page !== "login" && window.page !== "annotate" && !creation.visible && !importer.visible
            color: backend.messageIsError ? (Theme.dark ? "#492d26" : "#fff0eb") : Theme.selected; border.color: Theme.border
            RowLayout { anchors.fill: parent; anchors.margins: 12; Label { id: notification; text: backend.message; Layout.fillWidth: true; wrapMode: Text.WordWrap; font.pixelSize: 14; color: backend.messageIsError ? Theme.danger : Theme.text } StudioButton { text: "×"; quiet: true; onClicked: backend.clearMessage() } }
        }
    }
    Menu {
        id: navigation; x: 170; y: 46
        MenuItem { text: qsTr("Home"); onTriggered: window.navigate("home") }
        MenuItem { text: qsTr("Projects"); onTriggered: window.navigate("projects") }
        MenuSeparator {}
        MenuItem { text: qsTr("Account & Settings"); onTriggered: window.openAccount(0) }
        MenuItem { text: qsTr("Members & Invitations"); onTriggered: window.openAccount(2) }
        MenuItem { text: qsTr("Open example"); onTriggered: window.guard(function() { window.demoShown = false; backend.loadDemo() }) }
    }
    Menu {
        id: accountMenu; x: window.width - width - 12; y: 46
        MenuItem { text: backend.user.email || qsTr("Account"); enabled: false }
        MenuItem { text: qsTr("Account & Settings"); onTriggered: window.openAccount(0) }
        MenuItem { text: qsTr("Members & Invitations"); onTriggered: window.openAccount(2) }
        MenuSeparator {}
        MenuItem { text: qsTr("Log out"); onTriggered: window.guard(function() { backend.disconnectServer(); window.demoShown = false }) }
    }
    CreateProjectDialog { id: creation }
    ImportDialog { id: importer; objectName: "importDialog" }
    Dialog {
        id: discardDialog; objectName: "navigationDiscardDialog"; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 420
        title: qsTr("Unsaved changes"); standardButtons: Dialog.Discard | Dialog.Cancel
        Label { width: parent.width; text: qsTr("Discard the unsaved changes and continue?"); wrapMode: Text.WordWrap }
        onDiscarded: { if (window.page === "settings") settingsPage.load(); if (window.page === "account") accountPage.discard(); workspace.cancelDraft(); backend.discardChanges(); const action = window.pendingAction; window.pendingAction = null; if (action) action() }
        onRejected: window.pendingAction = null
    }
    Dialog {
        id: shortcuts; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 440; title: qsTr("Keyboard shortcuts"); standardButtons: Dialog.Ok
        Label { text: qsTr("Draw shape          R\nSelect / move       V\nPan image            H / middle mouse\nZoom                    Mouse wheel\nSave annotation    Ctrl+S\nUndo                     Ctrl+Z\nRedo                      Ctrl+Shift+Z\nDelete region         Delete\nFinish polygon       Enter\nCancel polygon      Esc"); lineHeight: 1.8; font.pixelSize: 16 }
    }
}
