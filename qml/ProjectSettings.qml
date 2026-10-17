import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root
    objectName: "projectSettings"
    enabled: !backend.busy
    property int section: 0
    property string originalName: ""
    property string originalDescription: ""
    property string originalConfig: ""
    property string originalColor: "#FFFFFF"
    property string sampling: "Sequential sampling"
    property string originalSampling: "Sequential sampling"
    property bool droppingTabs: false
    readonly property bool dirty: name.text !== originalName || description.text !== originalDescription || setup.config !== originalConfig || projectColor.text!==originalColor || sampling!==originalSampling
    function markSaved() { originalName = name.text; originalDescription = description.text; originalConfig = setup.config;originalColor=projectColor.text;originalSampling=sampling }
    function save() { backend.updateProjectDetails(name.text,description.text,setup.config,projectColor.text,sampling) }
    Connections { target: backend; function onProjectUpdated() { const previousSection = root.section; root.load(); root.section = previousSection } }
    function load() { name.text = backend.project.title || ""; description.text = backend.project.description || ""; setup.setConfig(backend.project.label_config || "");projectColor.text=backend.project.color || "#FFFFFF";sampling=backend.project.sampling || "Sequential sampling"; section = 0; markSaved() }
    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.preferredWidth: 240; Layout.fillHeight: true; color: Theme.subtle
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 8
                Repeater { model: [qsTr("General"), qsTr("Labeling Interface"), qsTr("Danger Zone")]; StudioButton { required property string modelData; required property int index; Layout.fillWidth: true; text: modelData; primary: root.section === index; quiet: root.section !== index; onClicked: root.section = index } }
                Item { Layout.fillHeight: true }
                Label { Layout.fillWidth: true; text: qsTr("Additional integrations remain available in the web application."); font.pixelSize: 12; color: Theme.muted; wrapMode: Text.WordWrap }
                StudioButton { text: qsTr("Open in browser  ↗"); quiet: true; onClicked: Qt.openUrlExternally(backend.serverUrl + "projects/" + backend.project.id + "/settings/") }
            }
        }
        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; currentIndex: root.section
            ScrollView {
                id: generalScroll; objectName: "settingsGeneralScroll"; contentWidth: availableWidth; contentHeight: generalContent.y+generalContent.implicitHeight; clip: true
                ColumnLayout {
                    id: generalContent
                    x: 40; y: 32; width: Math.min(640, generalScroll.availableWidth - 80); spacing: 16
                    Label { text: qsTr("General Settings"); font.pixelSize: 28; font.weight: Font.Medium; Layout.bottomMargin: 16 }
                    Label { text: qsTr("Project Name"); font.pixelSize: 14 }
                    StudioField { id: name; objectName: "settingsName"; Layout.fillWidth: true }
                    Label { text: qsTr("Description"); font.pixelSize: 14; Layout.topMargin: 16 }
                    ScrollView { Layout.fillWidth: true; Layout.preferredHeight: 130; StudioArea { id: description; objectName: "settingsDescription"; color: Theme.text; font.pixelSize: 16; wrapMode: TextEdit.WordWrap; selectByMouse: true; padding: 12; background: Rectangle { radius: 4; color: Theme.surface; border.color: Theme.border } } }
                    Label { text: qsTr("Color") }
                    RowLayout {
                        Rectangle { width: 32; height: 32; radius: 4; color: /^#[0-9a-fA-F]{6}$/.test(projectColor.text)?projectColor.text:Theme.border; border.color: Theme.border }
                        StudioField { id: projectColor; objectName: "settingsColor"; Layout.preferredWidth: 160; maximumLength: 7; placeholderText: "#RRGGBB" }
                        StudioButton { text: qsTr("Choose color"); onClicked: { colorPicker.selectedColor=/^#[0-9a-fA-F]{6}$/.test(projectColor.text)?projectColor.text:"#FFFFFF";colorPicker.open() } }
                    }
                    Label { text: qsTr("Task Sampling") }
                    ComboBox { objectName: "settingsSampling"; Layout.fillWidth: true; model: [qsTr("Sequential sampling"),qsTr("Random sampling")]; currentIndex: root.sampling==="Sequential sampling"?0:root.sampling==="Uniform sampling"?1:-1; displayText: currentIndex>=0?currentText:root.sampling; onActivated: root.sampling=currentIndex===0?"Sequential sampling":"Uniform sampling" }
                    Label { Layout.fillWidth: true; text: qsTr("Start labeling to use the project's sampling queue. Sequential sampling follows the current task order; random sampling chooses tasks on the server."); color: Theme.muted; wrapMode: Text.WordWrap }
                    Label { text: qsTr("Workspace     ✧  Enterprise"); font.pixelSize: 14; Layout.topMargin: 16 }
                    StudioField { Layout.fillWidth: true; placeholderText: qsTr("Select an option"); enabled: false }
                    Label { Layout.fillWidth: true; text: qsTr("Simplify project management by organizing projects into workspaces."); color: Theme.muted; wrapMode: Text.WordWrap; font.pixelSize: 14 }
                    StudioButton { objectName: "saveProjectSettings"; text: qsTr("Save"); primary: true; Layout.preferredWidth: 120; Layout.topMargin: 24; enabled: !backend.busy && !backend.demo; onClicked: root.save() }
                    Item { Layout.preferredHeight: 64 }
                }
            }
            ColumnLayout {
                LabelingSetup { id: setup; objectName: "settingsLabelingSetup"; existingProject: true; Layout.fillWidth: true; Layout.fillHeight: true }
                StudioButton { text: qsTr("Save Labeling Interface"); primary: true; Layout.alignment: Qt.AlignRight; Layout.margins: 24; enabled: !backend.busy && !backend.demo; onClicked: root.save() }
            }
            Item {
                ColumnLayout {
                    x: 48; y: 40; width: Math.min(640, parent.width - 96); spacing: 24
                    Label { text: qsTr("Danger Zone"); font.pixelSize: 28; font.weight: Font.Medium }
                    Label { Layout.fillWidth: true; text: qsTr("Recalculate cached annotation labels if an outdated cache blocks labeling configuration changes."); wrapMode: Text.WordWrap; color: Theme.muted }
                    StudioButton { objectName: "resetProjectCache"; text: qsTr("Reset Cache"); destructive: true; enabled: !backend.demo && !root.dirty; onClicked: { root.droppingTabs=false;confirmMaintenance.open() } }
                    Label { Layout.fillWidth: true; text: qsTr("Delete all saved Data Manager tabs for this project. Tasks and annotations are retained; the desktop filter will be reset."); wrapMode: Text.WordWrap; color: Theme.muted }
                    StudioButton { objectName: "dropProjectTabs"; text: qsTr("Drop All Tabs"); destructive: true; enabled: !backend.demo && !root.dirty; onClicked: { root.droppingTabs=true;confirmMaintenance.open() } }
                    Label { Layout.fillWidth: true; text: qsTr("Delete this project and all of its tasks and annotations. This action cannot be undone."); wrapMode: Text.WordWrap; font.pixelSize: 16; color: Theme.muted }
                    StudioButton { text: qsTr("Delete Project"); destructive: true; enabled: !backend.busy && !backend.demo; onClicked: confirmDelete.open() }
                }
            }
        }
    }
    ColorDialog { id: colorPicker; title: qsTr("Choose project color"); onAccepted: projectColor.text=selectedColor.toString() }
    Dialog { id: confirmMaintenance; objectName: "projectMaintenanceConfirm"; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 480; title: root.droppingTabs?qsTr("Drop All Tabs?"):qsTr("Reset Cache?"); standardButtons: Dialog.Yes | Dialog.Cancel
        Label { width: parent.width; wrapMode: Text.WordWrap; text: root.droppingTabs?qsTr("Delete all saved Data Manager tabs for “%1”? Their filters and column settings will be lost.").arg(backend.project.title || ""):qsTr("Recalculate the cached annotation labels for “%1”?").arg(backend.project.title || "") }
        onAccepted: { if(root.droppingTabs)backend.dropProjectTabs();else backend.resetProjectCache() }
    }
    Dialog {
        id: confirmDelete; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 460; title: qsTr("Delete Project?")
        standardButtons: Dialog.Yes | Dialog.Cancel
        onOpened: standardButton(Dialog.Yes).text = Qt.binding(function() { return qsTr("Delete permanently") })
        Label { width: parent.width; text: qsTr("Permanently delete “%1” and all its tasks and annotations?").arg(backend.project.title || ""); wrapMode: Text.WordWrap }
        onAccepted: backend.deleteProject()
    }
}
