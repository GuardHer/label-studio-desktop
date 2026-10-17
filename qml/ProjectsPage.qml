import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    objectName: "projectsPage"
    signal openRequested(int id)
    signal createRequested()
    signal settingsRequested(int id)
    property var pendingDelete: ({})
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 32; spacing: 20
        RowLayout {
            Layout.fillWidth: true; spacing: 16
            Label { text: qsTr("Projects"); font.pixelSize: 28; font.weight: Font.DemiBold }
            StudioField { objectName: "projectSearch"; Layout.fillWidth: true; placeholderText: qsTr("Search projects by name, description or ID…"); text: backend.catalog.search; onTextEdited: backend.catalog.search=text }
            ComboBox {
                objectName: "projectSort"; Layout.preferredWidth: 190
                model: [{name:qsTr("Newest first"),value:"newest"},{name:qsTr("Oldest first"),value:"oldest"},{name:qsTr("Name A–Z"),value:"title"},{name:qsTr("Name Z–A"),value:"title_desc"},{name:qsTr("Recently opened"),value:"recent"}]
                textRole: "name"; valueRole: "value"; currentIndex: model.findIndex(function(row){return row.value===backend.catalog.order})
                onActivated: backend.catalog.order=currentValue
            }
            StudioButton { glyph: "refresh"; enabled: !backend.busy && !backend.demo; onClicked: backend.refreshProjects(); ToolTip.visible: hovered; ToolTip.text: qsTr("Refresh projects") }
        }
    GridView {
        id: projects; Layout.fillWidth: true; Layout.fillHeight: true
        model: backend.catalog.items; cellWidth: 284; cellHeight: 212; clip: true
        ScrollBar.vertical: ScrollBar {}
        delegate: ItemDelegate {
            required property var modelData
            width: 252; height: 180; padding: 0; enabled: !backend.busy
            background: Rectangle { color: parent.hovered ? Theme.selected : Theme.surface; radius: 8; border.color: Theme.border; Rectangle { x: 1; y: 8; width: 4; height: parent.height-16; color: modelData.color || Theme.border; radius: 2 } }
            contentItem: ColumnLayout {
                spacing: 0
                ColumnLayout {
                    Layout.fillWidth: true; Layout.margins: 16; spacing: 12
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: modelData.title; font.pixelSize: 16; font.weight: Font.DemiBold; elide: Text.ElideRight; Layout.fillWidth: true }
                        ToolButton { objectName: "projectMenu"+modelData.id; implicitWidth: 30; implicitHeight: 28; onClicked: actions.open()
                            contentItem: Item { Column { anchors.centerIn: parent; spacing: 2; Repeater { model: 3; Rectangle { width: 3; height: 3; radius: 1.5; color: Theme.text } } } }
                            Accessible.name: qsTr("Project actions"); ToolTip.visible: hovered; ToolTip.text: qsTr("Project actions")
                            Menu { id: actions
                                MenuItem { text: qsTr("Open project"); onTriggered: root.openRequested(modelData.id) }
                                MenuItem { text: qsTr("Settings"); enabled: !backend.demo; onTriggered: root.settingsRequested(modelData.id) }
                                MenuSeparator {}
                                MenuItem { text: qsTr("Delete Project"); enabled: !backend.demo; onTriggered: { root.pendingDelete=modelData;confirmDelete.open() } }
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 16
                        Label { text: (modelData.finished_task_number || 0) + " / " + (modelData.task_number || 0); color: Theme.muted; font.pixelSize: 14; Layout.fillWidth: true }
                        Label { text: "✓ " + (modelData.total_annotations_number || 0); color: Theme.positive; font.pixelSize: 14 }
                        Label { text: "— " + (modelData.skipped_annotations_number || 0); color: Theme.danger; font.pixelSize: 14 }
                        Label { text: "✧ " + (modelData.total_predictions_number || 0); color: Theme.accent; font.pixelSize: 14 }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
                Label { Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16; Layout.topMargin: 12; text: modelData.description || ""; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight; font.pixelSize: 14; color: Theme.muted }
                Item { Layout.fillHeight: true }
                RowLayout {
                    Layout.fillWidth: true; Layout.margins: 16
                    Label { Layout.fillWidth: true; text: modelData.created_at ? new Date(modelData.created_at).toLocaleString(Qt.locale(languageManager.localeName), Locale.ShortFormat) : ""; color: Theme.muted; font.pixelSize: 12 }
                    Rectangle { width: 28; height: 28; radius: 14; color: "#e7b8b7"; Label { anchors.centerIn: parent; text: backend.user.email ? String(backend.user.email).slice(0,2) : "LS"; color: "#5c3d3b"; font.pixelSize: 11 } }
                }
            }
            onClicked: root.openRequested(modelData.id)
        }
    }
    }
    ColumnLayout {
        anchors.centerIn: parent; spacing: 20; visible: backend.catalog.items.length === 0
        Label { text: backend.projects.length ? qsTr("No projects match your search") : qsTr("Create your first project"); font.pixelSize: 28 }
        StudioButton { text: qsTr("Create Project"); visible: backend.projects.length===0; primary: true; Layout.alignment: Qt.AlignHCenter; onClicked: root.createRequested() }
    }
    Dialog { id: confirmDelete; objectName: "deleteCardProjectConfirm"; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 480; title: qsTr("Delete Project?"); standardButtons: Dialog.Yes | Dialog.Cancel
        Label { width: parent.width; wrapMode: Text.WordWrap; text: qsTr("Permanently delete “%1” and all its tasks and annotations?").arg(root.pendingDelete.title || "") }
        onAccepted: backend.deleteProjectById(root.pendingDelete.id)
    }
}
