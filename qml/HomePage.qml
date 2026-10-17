import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: root
    property bool allProjects: false
    signal createRequested()
    signal openRequested(int id)
    signal inviteRequested()
    contentWidth: availableWidth
    clip: true
    ColumnLayout {
        width: root.availableWidth; spacing: 24
        RowLayout {
            Layout.fillWidth: true; Layout.margins: 24; spacing: 24
            ColumnLayout {
                Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.alignment: Qt.AlignTop; spacing: 24
                ColumnLayout {
                    spacing: 4
                    Label { text: root.allProjects ? qsTr("Projects") : qsTr("Welcome 👋"); font.pixelSize: 28; font.weight: Font.Medium }
                    Label { text: root.allProjects ? qsTr("Manage your data labeling projects.") : qsTr("Let's get you started."); color: Theme.muted; font.pixelSize: 14 }
                }
                RowLayout {
                    spacing: 16
                    StudioButton { objectName: "homeCreate"; text: qsTr("Create Project"); glyph: "folder-add"; Layout.preferredWidth: 250; Layout.preferredHeight: 40; onClicked: root.createRequested() }
                    StudioButton { text: qsTr("Invite Members"); glyph: "user-add"; Layout.preferredWidth: 200; Layout.preferredHeight: 40; onClicked: root.inviteRequested() }
                }
                Rectangle {
                    visible: backend.projects.length === 0
                    Layout.fillWidth: true; Layout.preferredHeight: 280
                    color: Theme.selected; radius: 8; border.color: Theme.border
                    ColumnLayout {
                        anchors.centerIn: parent; width: parent.width - 32; spacing: 12
                        Rectangle { Layout.alignment: Qt.AlignHCenter; width: 48; height: 48; radius: 24; color: Theme.dark ? "#45486a" : "#dcdffa"; Label { anchors.centerIn: parent; text: "▱"; font.pixelSize: 30; color: Theme.accent } }
                        Label { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: qsTr("Create your first project"); font.pixelSize: 28; font.weight: Font.Medium; wrapMode: Text.WordWrap }
                        Label { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: qsTr("Import your data and set up the labeling interface to start annotating"); color: Theme.muted; wrapMode: Text.WordWrap; font.pixelSize: 14 }
                        StudioButton { text: qsTr("Create Project"); primary: true; Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 8; onClicked: root.createRequested() }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true; implicitHeight: projectsColumn.implicitHeight + 40
                    visible: backend.projects.length > 0; color: Theme.surface; radius: 8; border.color: Theme.border
                    ColumnLayout {
                        id: projectsColumn; anchors.fill: parent; anchors.margins: 20; spacing: 16
                        RowLayout {
                            Label { text: root.allProjects ? qsTr("All Projects") : qsTr("Recent Projects"); font.pixelSize: 20; font.weight: Font.DemiBold; Layout.fillWidth: true }
                            StudioButton { text: "↻"; quiet: true; enabled: !backend.busy && !backend.demo; onClicked: backend.refreshProjects() }
                        }
                        Repeater {
                            model: root.allProjects ? backend.projects : backend.catalog.recent
                            ItemDelegate {
                                required property var modelData
                                Layout.fillWidth: true; implicitHeight: 84; enabled: !backend.busy
                                background: Rectangle { color: parent.hovered ? Theme.selected : Theme.subtle; radius: 4; Rectangle { width: 3; height: parent.height; color: modelData.color || Theme.border } }
                                contentItem: RowLayout {
                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 8
                                        Label { text: modelData.title; font.pixelSize: 17; elide: Text.ElideRight; Layout.fillWidth: true }
                                        Label { text: qsTr("%1 of %2 tasks").arg(modelData.finished_task_number || 0).arg(modelData.task_number || 0); color: Theme.muted; font.pixelSize: 13 }
                                    }
                                    ProgressBar { Layout.preferredWidth: 140; value: modelData.task_number > 0 ? (modelData.finished_task_number || 0) / modelData.task_number : 0 }
                                    Label { text: "→"; color: Theme.accent; font.pixelSize: 22 }
                                }
                                onClicked: root.openRequested(modelData.id)
                            }
                        }
                    }
                }
                Label { text: qsTr("Label Studio Version: Community"); color: Theme.muted; font.pixelSize: 12 }
            }
            ColumnLayout {
                Layout.preferredWidth: 450; Layout.maximumWidth: 450; Layout.minimumWidth: 280; Layout.alignment: Qt.AlignTop; spacing: 24
                Rectangle {
                    Layout.fillWidth: true; implicitHeight: 168; radius: 8; border.color: Theme.border; color: Theme.subtle
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 20; spacing: 10
                        Label { text: qsTr("Did you know?"); font.pixelSize: 16; font.weight: Font.DemiBold }
                        Label { Layout.fillWidth: true; text: qsTr("Try Label Studio Starter Cloud, optimized for small teams and projects."); wrapMode: Text.WordWrap; color: Theme.muted; font.pixelSize: 14 }
                        Label { text: qsTr("Learn more  ↗"); color: Theme.accent; font.pixelSize: 14; MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Qt.openUrlExternally("https://humansignal.com/pricing/") } }
                        Item { Layout.fillHeight: true }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true; implicitHeight: resources.implicitHeight + 40; radius: 8; border.color: Theme.border; color: Theme.surface
                    ColumnLayout {
                        id: resources; anchors.fill: parent; anchors.margins: 20; spacing: 6
                        Label { text: qsTr("Resources"); font.pixelSize: 20; font.weight: Font.DemiBold }
                        Label { text: qsTr("Learn, explore and get help"); color: Theme.muted; font.pixelSize: 14; Layout.bottomMargin: 16 }
                        Repeater {
                            model: [{name:qsTr("Documentation"),url:"https://labelstud.io/guide/"},{name:qsTr("API Documentation"),url:"https://api.labelstud.io/"},{name:qsTr("Release Notes"),url:"https://labelstud.io/learn/categories/release-notes/"},{name:qsTr("LabelStud.io Blog"),url:"https://labelstud.io/blog/"},{name:qsTr("Slack Community"),url:"https://slack.labelstud.io"}]
                            StudioButton { required property var modelData; Layout.fillWidth: true; text: modelData.name + "    ↗"; quiet: true; alignLeft: true; onClicked: Qt.openUrlExternally(modelData.url) }
                        }
                    }
                }
            }
        }
        Item { Layout.fillHeight: true }
    }
}
