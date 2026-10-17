import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    objectName: "createProjectDialog"
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(1280, parent.width - 80); height: parent.height - 80
    modal: true; padding: 0
    closePolicy: Popup.CloseOnEscape
    property int step: 0
    property alias stagedFiles: importer.files
    property alias stagedUrl: importer.datasetUrl
    onAboutToShow: { step = 0; name.text = qsTr("New Project"); description.clear(); importer.clear(); setup.setConfig(""); setup.browsing = true; backend.clearMessage() }
    background: Rectangle { radius: 8; color: Theme.surface; border.color: Theme.border }
    header: Rectangle {
        implicitHeight: 90; color: "transparent"
        Rectangle { width: parent.width; height: 1; anchors.bottom: parent.bottom; color: Theme.border }
        RowLayout {
            anchors.fill: parent; anchors.margins: 24; spacing: 24
            Label { text: qsTr("Create Project"); font.pixelSize: 24; font.weight: Font.Medium; Layout.preferredWidth: 190 }
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 52; color: Theme.subtle; radius: 6; border.color: Theme.border
                RowLayout {
                    anchors.fill: parent; anchors.margins: 4; spacing: 4
                    Repeater { model: [qsTr("Project Name"), qsTr("Data Import"), qsTr("Labeling Setup")]; StudioButton { required property string modelData; required property int index; Layout.fillWidth: true; Layout.fillHeight: true; text: modelData; quiet: root.step !== index; onClicked: root.step = index } }
                }
            }
            StudioButton { text: qsTr("Cancel"); destructive: true; Layout.preferredWidth: 100; Layout.preferredHeight: 40; enabled: !backend.busy; onClicked: root.close() }
            StudioButton { objectName: "createProjectSave"; text: backend.busy ? qsTr("Saving…") : qsTr("Save"); primary: true; Layout.preferredWidth: 100; Layout.preferredHeight: 40; enabled: name.text.trim().length > 0 && !backend.busy && backend.connected && !backend.demo; onClicked: backend.createProject(name.text, description.text, setup.config) }
        }
    }
    ColumnLayout {
        anchors.fill: parent; spacing: 0
        Label { Layout.fillWidth: true; Layout.margins: 16; text: backend.message; visible: backend.messageIsError; color: Theme.danger; wrapMode: Text.WordWrap }
        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; currentIndex: root.step
            Item {
                ColumnLayout {
                    width: Math.min(560, parent.width - 80); anchors.horizontalCenter: parent.horizontalCenter; y: 40; spacing: 16
                    Label { text: qsTr("Project Name"); font.pixelSize: 14; font.weight: Font.DemiBold }
                    StudioField { id: name; objectName: "projectTitle"; Layout.fillWidth: true; placeholderText: qsTr("Enter a project name") }
                    Label { text: qsTr("Description"); font.pixelSize: 14; font.weight: Font.DemiBold; Layout.topMargin: 16 }
                    ScrollView { Layout.fillWidth: true; Layout.preferredHeight: 110; StudioArea { id: description; placeholderText: qsTr("Optional description of your project"); color: Theme.text; font.pixelSize: 16; selectByMouse: true; wrapMode: TextEdit.WordWrap; padding: 14; background: Rectangle { radius: 4; color: Theme.surface; border.color: Theme.border } } }
                    Label { text: qsTr("Workspace     ✧  Enterprise"); font.pixelSize: 14; Layout.topMargin: 20 }
                    StudioField { Layout.fillWidth: true; placeholderText: qsTr("Select an option"); enabled: false }
                    Label { Layout.fillWidth: true; text: qsTr("Simplify project management by organizing projects into workspaces."); color: Theme.muted; wrapMode: Text.WordWrap; font.pixelSize: 14 }
                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 106; Layout.topMargin: 24; color: Theme.subtle; radius: 8; border.color: Theme.border
                        ColumnLayout { anchors.fill: parent; anchors.margins: 16; Label { text: qsTr("Did you know?"); font.pixelSize: 16; font.weight: Font.DemiBold } Label { Layout.fillWidth: true; text: qsTr("Try Label Studio Starter Cloud, optimized for small teams and projects."); color: Theme.muted; wrapMode: Text.WordWrap; font.pixelSize: 14 } Label { text: qsTr("Learn more  ↗"); color: Theme.accent } }
                    }
                    Image { source: "qrc:/assets/opossum_looking.png"; Layout.preferredWidth: 110; Layout.preferredHeight: 80; fillMode: Image.PreserveAspectFit }
                }
            }
            ImportPane { id: importer }
            LabelingSetup { id: setup; objectName: "creationLabelingSetup" }
        }
    }
}
