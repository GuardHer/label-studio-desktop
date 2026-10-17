import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Studio.Native 1.0

Item {
    id: root
    property bool browsing: true
    property bool codeMode: false
    property string config: ""
    property bool existingProject: false
    property int category: 0
    property string templateSearch: ""
    property int colorIndex: -1
    property var previewData: { const source=editor.xml; return editor.sampleData() }
    readonly property bool polygon: config.indexOf("<PolygonLabels")>=0
    readonly property var categoryTitles: [qsTr("Computer Vision"),qsTr("Natural Language Processing"),qsTr("Audio/Speech Processing"),qsTr("Conversational AI"),qsTr("Chat"),qsTr("Ranking & Scoring"),qsTr("Structured Data Parsing"),qsTr("Time Series Analysis"),qsTr("Videos"),qsTr("Generative AI"),qsTr("Community Contributions")]
    function setConfig(value) { config=value;editor.xml=value;browsing=false;codeMode=false }
    function templateConfig(usePolygon) { const row=editor.templates.find(t=>t.slug===(usePolygon?"computer-vision/semantic-segmentation-with-polygons":"computer-vision/object-detection-with-bounding-boxes"));return row?row.config:"<View></View>" }
    function openCompletion() {
        const before=xml.text.slice(0,xml.cursorPosition)
        completion.prefix=(before.match(/[A-Za-z]*$/)||[""])[0]
        const opening=before.slice(before.lastIndexOf("<"))
        const tag=opening.match(/^<([A-Za-z]+)\s/)
        completion.rows=tag&&opening.indexOf(">")<0?editor.attributeCompletions(tag[1],completion.prefix):editor.completions(completion.prefix)
        completion.open()
    }
    onConfigChanged: if(editor.xml!==config)editor.xml=config
    LabelingConfig { id: editor; objectName: "configEditorModel"; onChanged: if(root.config!==editor.xml)root.config=editor.xml }
    ColorDialog { id: labelColor; title: qsTr("Choose label color"); onAccepted: {const label=editor.labels[root.colorIndex];if(label)editor.setLabel(root.colorIndex,label.value,selectedColor.toString(),label.alias,label.hotkey)} }
    RowLayout {
        anchors.fill: parent; spacing: 0; visible: root.browsing
        Rectangle {
            Layout.preferredWidth: 210; Layout.fillHeight: true; color: Theme.subtle
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 12; spacing: 2
                Repeater { model: root.categoryTitles; StudioButton { required property string modelData; required property int index; text: modelData; Layout.fillWidth: true; alignLeft: true; primary: root.category===index; quiet: root.category!==index; onClicked: root.category=index; font.pixelSize: 12 } }
                Item { Layout.fillHeight: true }
                StudioButton { text: qsTr("Custom template"); Layout.fillWidth: true; onClicked: {root.browsing=false;root.codeMode=true;if(!root.config.length)root.config="<View>\n</View>"} }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 20; spacing: 16
            Label { text: root.categoryTitles[root.category] || ""; font.pixelSize: 24; font.weight: Font.DemiBold }
            StudioField { Layout.fillWidth: true; placeholderText: qsTr("Search templates…"); text: root.templateSearch; onTextEdited: root.templateSearch=text }
            GridView {
                id: templatesGrid; objectName: "templateCatalogue"; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; cellWidth: width<580?width:Math.floor(width/2); cellHeight: 245
                model: editor.templates.filter(t=>t.group===editor.groups[root.category]&&(!root.templateSearch.length||editor.templateTitle(t.title).toLowerCase().indexOf(root.templateSearch.toLowerCase())>=0||t.title.toLowerCase().indexOf(root.templateSearch.toLowerCase())>=0))
                ScrollBar.vertical: ScrollBar {}
                delegate: ItemDelegate {
                    required property var modelData; width: templatesGrid.cellWidth-14; height: 230; padding: 10
                    background: Rectangle { radius: 6; color: parent.hovered?Theme.selected:Theme.surface; border.color: Theme.border }
                    contentItem: ColumnLayout {
                        spacing: 8
                        Image { source: modelData.image || ""; Layout.fillWidth: true; Layout.fillHeight: true; fillMode: Image.PreserveAspectFit; asynchronous: true }
                        Label { Layout.fillWidth: true; text: {const revision=languageManager.code;return editor.templateTitle(modelData.title)} font.pixelSize: 14; font.weight: Font.DemiBold; wrapMode: Text.WordWrap; maximumLineCount: 2 }
                        Label { visible: modelData.type!=="community"; text: qsTr("Enterprise"); color: Theme.muted; font.pixelSize: 12 }
                        StudioButton { text: qsTr("Use Template"); primary: true; enabled: modelData.type==="community"; onClicked: root.setConfig(modelData.config) }
                    }
                    onClicked: if(modelData.type==="community")root.setConfig(modelData.config)
                }
            }
        }
    }
    SplitView {
        anchors.fill: parent; visible: !root.browsing; orientation: Qt.Horizontal
        ScrollView {
            id: editScroll; objectName: "configurationEditorScroll"; SplitView.preferredWidth: Math.min(440,root.width*0.44); SplitView.minimumWidth: 300; contentWidth: availableWidth; clip: true
            ColumnLayout {
                width: editScroll.availableWidth; spacing: 16
                Label { text: qsTr("Labeling Interface"); Layout.margins: 20; Layout.bottomMargin: 0; font.pixelSize: 24; font.weight: Font.DemiBold }
                RowLayout { Layout.fillWidth: true; Layout.leftMargin: 20; Layout.rightMargin: 20; spacing: 6; StudioButton { text: qsTr("Browse Templates"); font.pixelSize: 12; onClicked: root.browsing=true } Item { Layout.fillWidth: true } StudioButton { text: qsTr("Code"); primary: root.codeMode; onClicked: root.codeMode=true } StudioButton { text: qsTr("Visual"); primary: !root.codeMode; onClicked: root.codeMode=false } }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
                ColumnLayout {
                    visible: root.codeMode; Layout.fillWidth: true; Layout.leftMargin: 20; Layout.rightMargin: 20; spacing: 10
                    ScrollView {
                        Layout.fillWidth: true; Layout.preferredHeight: Math.max(320,root.height-210)
                        StudioArea { id: xml; objectName: "labelConfig"; text: root.config; font.family: "Consolas"; font.pixelSize: 14; wrapMode: TextEdit.NoWrap; selectByMouse: true; onTextChanged: if(activeFocus&&root.config!==text)root.config=text; Keys.onPressed: event=>{if(event.key===Qt.Key_Space&&(event.modifiers&Qt.ControlModifier)){root.openCompletion();event.accepted=true}} XmlSyntax { document: xml.textDocument } }
                    }
                    RowLayout { StudioButton { text: qsTr("Format XML"); enabled: !editor.error.length; onClicked: editor.formatXml() } StudioButton { text: qsTr("Complete tag"); onClicked: root.openCompletion() } }
                }
                ColumnLayout {
                    visible: !root.codeMode; Layout.fillWidth: true; Layout.leftMargin: 20; Layout.rightMargin: 20; spacing: 14
                    Label { text: qsTr("Configure data"); font.weight: Font.DemiBold }
                    Repeater { model: editor.objects; ColumnLayout { required property var modelData; required property int index; Layout.fillWidth: true; spacing: 6; Label { text: modelData.tag+" · "+modelData.name; color: Theme.muted } StudioField { objectName: "configDataField"+index; Layout.fillWidth: true; text: modelData.value; placeholderText: "$field"; onEditingFinished: editor.setObjectValue(index,text) } } }
                    Label { text: qsTr("Control"); font.weight: Font.DemiBold }
                    ComboBox { Layout.fillWidth: true; model: editor.controls; textRole: "name"; currentIndex: editor.selectedControl; onActivated: editor.selectedControl=currentIndex }
                    Label { text: qsTr("Add label names"); font.weight: Font.DemiBold }
                    Label { Layout.fillWidth: true; text: qsTr("Use a new line to separate multiple labels"); wrapMode: Text.WordWrap; color: Theme.muted; font.pixelSize: 13 }
                    ScrollView { Layout.fillWidth: true; Layout.preferredHeight: 80; StudioArea { id: labelNames; objectName: "configLabelNames"; wrapMode: TextEdit.Wrap } }
                    StudioButton { text: qsTr("Add labels"); onClicked: {editor.addLabels(labelNames.text);labelNames.clear()} }
                    Label { text: qsTr("Labels (%1)").arg(editor.labels.length); font.weight: Font.DemiBold }
                    Repeater {
                        model: editor.labels
                        ColumnLayout {
                            required property var modelData; required property int index; Layout.fillWidth: true; spacing: 4
                            RowLayout { Layout.fillWidth: true
                                Button { implicitWidth: 32; implicitHeight: 32; background: Rectangle { color: modelData.color; radius: 3; border.color: Theme.border } onClicked: {root.colorIndex=index;labelColor.selectedColor=modelData.color;labelColor.open()} }
                                StudioField { Layout.fillWidth: true; text: modelData.value; onEditingFinished: editor.setLabel(index,text,modelData.color,modelData.alias,modelData.hotkey) }
                                StudioButton { text: "×"; quiet: true; onClicked: editor.removeLabel(index); ToolTip.visible: hovered; ToolTip.text: qsTr("Remove label") }
                            }
                            RowLayout { Layout.fillWidth: true; StudioField { Layout.fillWidth: true; text: modelData.alias; placeholderText: qsTr("Alias"); font.pixelSize: 12; onEditingFinished: editor.setLabel(index,modelData.value,modelData.color,text,modelData.hotkey) } StudioField { Layout.preferredWidth: 80; text: modelData.hotkey; placeholderText: qsTr("Hotkey"); font.pixelSize: 12; onEditingFinished: editor.setLabel(index,modelData.value,modelData.color,modelData.alias,text) } }
                        }
                    }
                    Label { visible: !editor.controls.length; Layout.fillWidth: true; text: qsTr("Choose a template or enter XML to configure the labeling interface."); wrapMode: Text.WordWrap; color: Theme.muted }
                    CheckBox { visible: editor.controls.length>0; text: qsTr("Required"); checked: ((editor.controls[editor.selectedControl] || {}).attributes || {}).required==="true"; onClicked: editor.setControlAttribute("required",checked?"true":"false") }
                    Label { visible: editor.controls.length>0; text: qsTr("Configure settings"); font.weight: Font.DemiBold }
                    Repeater {
                        model: { const c=editor.controls[editor.selectedControl] || {};return c.tag==="Number"?["min","max","step"]:c.tag==="Rating"?["maxRating"]:c.tag==="TextArea"?["placeholder","rows","maxSubmissions"]:(c.tag || "").endsWith("Labels")?["maxUsages"]:[] }
                        RowLayout {
                            required property string modelData; Layout.fillWidth: true
                            Label { text: modelData; Layout.preferredWidth: 130; color: Theme.muted }
                            StudioField { Layout.fillWidth: true; text: String(((editor.controls[editor.selectedControl] || {}).attributes || {})[parent.modelData] || ""); onEditingFinished: editor.setControlAttribute(parent.modelData,text) }
                        }
                    }
                    ComboBox { visible: (editor.controls[editor.selectedControl] || {}).tag==="Choices"; Layout.fillWidth: true; model: [qsTr("Single choice"),qsTr("Multiple choices"),qsTr("Radio buttons")]; currentIndex: ["single","multiple","single-radio"].indexOf(((editor.controls[editor.selectedControl] || {}).attributes || {}).choice || "single"); onActivated: editor.setControlAttribute("choice",["single","multiple","single-radio"][currentIndex]) }
                }
                Label { Layout.fillWidth: true; Layout.leftMargin: 20; Layout.rightMargin: 20; visible: (editor.error+editor.message+editor.serverStatus).length>0; text: editor.error || editor.message || editor.serverStatus; color: editor.error.length?Theme.danger:Theme.muted; wrapMode: Text.WordWrap; font.pixelSize: 13 }
                RowLayout { Layout.leftMargin: 20; Layout.rightMargin: 20; Layout.bottomMargin: 20; StudioButton { objectName: "validateConfiguration"; text: qsTr("Validate with Django"); enabled: backend.connected&&!backend.demo&&!backend.busy&&!editor.error.length&&root.config.length>0; onClicked: editor.validateOnServer(backend,root.existingProject) } StudioButton { text: qsTr("Preview data"); onClicked: {sampleJson.text=JSON.stringify(root.previewData,null,2);sampleDialog.open()} } }
            }
        }
        Rectangle { SplitView.fillWidth: true; SplitView.preferredWidth: root.width*0.5; SplitView.minimumWidth: 250; color: Theme.subtle; LabelingPreview { anchors.fill: parent; anchors.margins: 18; config: root.config; sampleData: root.previewData } }
    }
    Popup {
        id: completion; parent: Overlay.overlay; anchors.centerIn: parent; width: 300; height: Math.min(350,list.contentHeight+24); modal: true; property string prefix: ""; property var rows: []
        ListView { id: list; anchors.fill: parent; model: completion.rows; clip: true; delegate: ItemDelegate { required property string modelData; width: list.width; text: modelData; onClicked: {const position=xml.cursorPosition;xml.remove(position-completion.prefix.length,position);xml.insert(position-completion.prefix.length,modelData);root.config=xml.text;completion.close();xml.forceActiveFocus()} } }
    }
    Dialog {
        id: sampleDialog; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 540; height: 400; title: qsTr("Preview data"); standardButtons: Dialog.Apply | Dialog.Cancel; property string error: ""
        ColumnLayout { anchors.fill: parent; ScrollView { Layout.fillWidth: true; Layout.fillHeight: true; StudioArea { id: sampleJson; font.family: "Consolas"; wrapMode: TextEdit.Wrap } } Label { Layout.fillWidth: true; text: sampleDialog.error; color: Theme.danger; wrapMode: Text.WordWrap } }
        onApplied: {try{const data=JSON.parse(sampleJson.text);if(!data||Array.isArray(data)||typeof data!=="object")throw new Error(qsTr("Enter a JSON object."));root.previewData=data;error="";close()}catch(e){error=String(e)}}
    }
}
