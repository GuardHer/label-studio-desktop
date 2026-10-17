import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Studio.Native 1.0

ColumnLayout {
    id: root
    property string config: ""
    property var sampleData: ({})
    property string activeLabel: ""
    property string tool: "draw"
    property var layoutNodes: []
    readonly property var document: previewDocument
    spacing: 12
    function rebuild() { previewCanvas.cancelDraft();previewDocument.configure(config);previewDocument.setTaskData(sampleData);layoutNodes=previewDocument.layoutTree;activeLabel=previewDocument.labels.length?previewDocument.labels[0].name:"" }
    onConfigChanged: updateTimer.restart()
    onSampleDataChanged: updateTimer.restart()
    Component.onCompleted: updateTimer.restart()
    Timer { id: updateTimer; interval: 250; onTriggered: root.rebuild() }
    AnnotationDocument { id: previewDocument }
    Label { text: qsTr("Preview"); font.pixelSize: 18; font.weight: Font.DemiBold }
    ScrollView {
        id: scroll; Layout.fillWidth: true; Layout.fillHeight: true; contentWidth: availableWidth; clip: true
        ColumnLayout {
            width: scroll.availableWidth; spacing: 12
            Label { Layout.fillWidth: true; visible: previewDocument.error.length>0; text: previewDocument.error; wrapMode: Text.WordWrap; color: Theme.danger; font.pixelSize: 13 }
            Label { Layout.fillWidth: true; visible: root.config.indexOf("<Style")>=0||root.config.indexOf("className=")>=0; text: qsTr("Stylesheets and CSS classes are not rendered in this preview."); wrapMode: Text.WordWrap; color: Theme.muted; font.pixelSize: 12 }
            ComboBox { Layout.fillWidth: true; visible: previewDocument.imageFrames.length>1; model: previewDocument.imageFrames; textRole: "name"; currentIndex: previewDocument.activeImage; onActivated: {previewCanvas.cancelDraft();previewDocument.selectImage(currentIndex)} }
            RowLayout {
                visible: previewDocument.imageFrames.length>0; Layout.fillWidth: true; Layout.preferredHeight: Math.min(380,Math.max(220,width*0.65)); spacing: 4
                AnnotationCanvas { id: previewCanvas; objectName: "configurationPreviewCanvas"; Layout.fillWidth: true; Layout.fillHeight: true; document: previewDocument; tool: root.tool; imageSource: previewDocument.activeImageSource; activeLabel: root.activeLabel; inset: 6; backgroundColor: Theme.subtle; onImageSizeChanged: if(imageSize.width>0&&imageSize.height>0)previewDocument.setImageSize(imageSize.width,imageSize.height) }
                ColumnLayout {
                    Layout.preferredWidth: 38; spacing: 2
                    StudioButton { glyph: "cursor"; quiet: root.tool!=="select"; primary: root.tool==="select"; Layout.fillWidth: true; leftPadding: 3; rightPadding: 3; onClicked: root.tool="select" }
                    StudioButton { glyph: previewCanvas.polygonMode?"polygon-tool":"rectangle-tool"; quiet: root.tool!=="draw"; primary: root.tool==="draw"; Layout.fillWidth: true; leftPadding: 3; rightPadding: 3; onClicked: root.tool="draw" }
                    StudioButton { glyph: "hand-tool"; quiet: root.tool!=="pan"; primary: root.tool==="pan"; Layout.fillWidth: true; leftPadding: 3; rightPadding: 3; onClicked: root.tool="pan" }
                    StudioButton { glyph: "zoom-in"; quiet: true; Layout.fillWidth: true; leftPadding: 3; rightPadding: 3; onClicked: previewCanvas.zoomAt(1.2,previewCanvas.width/2,previewCanvas.height/2) }
                    StudioButton { text: qsTr("Fit"); quiet: true; Layout.fillWidth: true; leftPadding: 3; rightPadding: 3; onClicked: previewCanvas.resetView() }
                    StudioButton { glyph: "zoom-out"; quiet: true; Layout.fillWidth: true; leftPadding: 3; rightPadding: 3; onClicked: previewCanvas.zoomAt(1/1.2,previewCanvas.width/2,previewCanvas.height/2) }
                }
            }
            ComboBox {
                Layout.fillWidth: true; visible: model.length>1
                model: previewDocument.controls.filter(c=>["rectanglelabels","polygonlabels"].indexOf(c.type)>=0)
                textRole: "name"; valueRole: "name"
                currentIndex: {const name=previewDocument.controls[previewDocument.activeControl]?.name;return model.findIndex(c=>c.name===name)}
                onActivated: {previewCanvas.cancelDraft();const index=previewDocument.controls.findIndex(c=>c.name===currentValue);previewDocument.selectControl(index);root.activeLabel=previewDocument.labels.length?previewDocument.labels[0].name:""}
            }
            Flow { Layout.fillWidth: true; spacing: 8; Repeater { model: previewDocument.labels; Button { id: chip; required property var modelData; text: modelData.display || modelData.name; implicitHeight: 28; topInset: 0; bottomInset: 0; leftPadding: 12; rightPadding: 12; contentItem: Text { text: chip.text; color: Theme.text; font.pixelSize: 13; verticalAlignment: Text.AlignVCenter } background: Rectangle { radius: 3; color: Qt.rgba(Qt.color(chip.modelData.color).r,Qt.color(chip.modelData.color).g,Qt.color(chip.modelData.color).b,0.18); border.width: root.activeLabel===chip.modelData.name?1:0; border.color: chip.modelData.color; Rectangle { width: 4; height: parent.height; color: chip.modelData.color } } onClicked: {root.activeLabel=modelData.name;root.tool="draw"} } } }
            Repeater { model: root.layoutNodes; ConfigNode { required property var modelData; Layout.fillWidth: true; node: modelData; document: previewDocument; taskData: root.sampleData } }
            Label { visible: previewDocument.imageFrames.length>0; text: qsTr("Regions (%1)").arg(previewDocument.rectangles.length); font.weight: Font.DemiBold; Layout.topMargin: 20 }
            Repeater { model: previewDocument.rectangles; Label { required property var modelData; text: modelData.displayLabel || modelData.label; color: Theme.muted } }
            StudioButton { text: qsTr("Reset preview"); enabled: previewDocument.editable; onClicked: root.rebuild() }
        }
    }
}
