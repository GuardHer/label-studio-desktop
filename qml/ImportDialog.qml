import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    parent: Overlay.overlay; anchors.centerIn: parent
    width: Math.min(1100, parent.width - 100); height: parent.height - 120
    modal: true; padding: 0
    onOpened: { importer.clear(); backend.clearMessage() }
    background: Rectangle { color: Theme.surface; radius: 8; border.color: Theme.border }
    header: Rectangle {
        implicitHeight: 80; color: "transparent"
        RowLayout { anchors.fill: parent; anchors.margins: 24; Label { text: qsTr("Import Data"); font.pixelSize: 24; Layout.fillWidth: true } StudioButton { text: qsTr("Cancel"); enabled: !backend.busy; onClicked: root.close() } StudioButton { text: backend.busy ? qsTr("Importing…") : qsTr("Import"); primary: true; enabled: !backend.busy && (importer.files.length > 0 || importer.datasetUrl.trim().length > 0) && !backend.demo; onClicked: importer.files.length ? backend.importFiles(importer.files) : backend.importUrl(importer.datasetUrl) } }
        Rectangle { width: parent.width; height: 1; anchors.bottom: parent.bottom; color: Theme.border }
    }
    ColumnLayout {
        anchors.fill: parent
        Label { visible: backend.messageIsError; Layout.fillWidth: true; Layout.margins: 16; text: backend.message; color: Theme.danger; wrapMode: Text.WordWrap }
        ImportPane { id: importer; Layout.fillWidth: true; Layout.fillHeight: true }
    }
    Connections { target: backend; function onImported() { root.close() } }
}
