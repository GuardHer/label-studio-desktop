import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root
    property var files: []
    property alias datasetUrl: dataset.text
    function clear() { files = []; dataset.clear() }
    function appendFiles(urls) { files = files.concat(Array.from(urls)) }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 24; spacing: 24
        RowLayout {
            Layout.fillWidth: true; spacing: 16
            StudioField { id: dataset; Layout.fillWidth: true; placeholderText: qsTr("Dataset URL") }
            Label { text: qsTr("or"); color: Theme.muted }
            StudioButton { text: qsTr("↥   Upload Files"); Layout.preferredHeight: 40; onClicked: picker.open() }
        }
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true; radius: 8; color: drop.containsDrag ? Theme.selected : Theme.subtle
            border.color: drop.containsDrag ? Theme.accent : Theme.border
            DropArea { id: drop; anchors.fill: parent; onDropped: event => { if (event.hasUrls) { root.appendFiles(event.urls); event.acceptProposedAction() } } }
            ColumnLayout {
                anchors.centerIn: parent; width: Math.min(parent.width - 64, 640); spacing: 16
                Label { text: "↥"; font.pixelSize: 60; color: Theme.accent; Layout.alignment: Qt.AlignHCenter }
                Label { text: qsTr("Drag & drop files here"); font.pixelSize: 28; font.weight: Font.Medium; Layout.alignment: Qt.AlignHCenter }
                StudioButton { text: qsTr("or click to browse"); quiet: true; Layout.alignment: Qt.AlignHCenter; onClicked: picker.open() }
                Label { text: qsTr("Images"); font.weight: Font.DemiBold; font.pixelSize: 16; Layout.topMargin: 12 }
                Label { text: qsTr("bmp, gif, jpg, jpeg, png, svg, webp"); color: Theme.muted; font.pixelSize: 14 }
                Label { text: qsTr("Structured data"); font.weight: Font.DemiBold; font.pixelSize: 16 }
                Label { text: qsTr("csv, tsv, json"); color: Theme.muted; font.pixelSize: 14 }
                Label { Layout.fillWidth: true; text: qsTr("Image files are uploaded to Django. JSON and CSV tasks must contain the image field used by your labeling configuration."); color: Theme.muted; wrapMode: Text.WordWrap; font.pixelSize: 13; Layout.topMargin: 8 }
                Label { visible: root.files.length > 0; text: qsTr("%1 file(s) ready to import").arg(root.files.length); color: Theme.positive; font.weight: Font.DemiBold; font.pixelSize: 15; Layout.topMargin: 8 }
                Label { visible: root.files.length > 0; Layout.fillWidth: true; text: root.files.map(f => decodeURIComponent(String(f).split('/').pop())).join("\n"); wrapMode: Text.WrapAnywhere; maximumLineCount: 5; elide: Text.ElideRight; color: Theme.muted }
                StudioButton { visible: root.files.length > 0; text: qsTr("Clear selection"); quiet: true; onClicked: root.files = [] }
            }
        }
        Label { Layout.fillWidth: true; text: qsTr("For large datasets, import smaller batches. Native annotation supports one image with RectangleLabels or PolygonLabels."); color: Theme.muted; wrapMode: Text.WordWrap; font.pixelSize: 13 }
    }
    FileDialog { id: picker; title: qsTr("Import data"); fileMode: FileDialog.OpenFiles; nameFilters: [qsTr("Supported data (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.svg *.json *.csv *.tsv)"), qsTr("All files (*)")]; onAccepted: root.appendFiles(selectedFiles) }
}
