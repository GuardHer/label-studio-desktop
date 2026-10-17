import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property var node: ({})
    property var document
    property var taskData: ({})
    readonly property string tag: node.tag || ""
    readonly property var attributes: node.attributes || ({})
    readonly property var control: document ? document.controls.find(c=>c.name===attributes.name) || ({}) : ({})
    readonly property var values: document ? document.controlValues : ({})
    readonly property var textEntries: tag==="TextArea" ? (values[attributes.name] || []) : []
    readonly property string rawStyle: attributes.style || ""
    readonly property bool hiddenTag: ["Image","Style","Label","Choice","RectangleLabels","PolygonLabels","Labels"].indexOf(tag)>=0
    readonly property real paddingSize: Math.max(0,parseFloat(styleValue("padding")) || 0)
    implicitHeight: hiddenTag ? 0 : (renderer.item ? renderer.item.implicitHeight : 0)+paddingSize*2
    function styleValue(key) { const match=rawStyle.match(new RegExp("(?:^|;)\\s*"+key+"\\s*:\\s*([^;]+)"));return match?match[1].trim():"" }
    function resolve(value) { if(typeof value!=="string")return String(value || "");if(value.startsWith("$")){let result=taskData;for(const key of value.slice(1).split('.'))result=result&&result[key];return result===undefined||result===null?"":String(result)}return value }
    function setTextEntry(index,text) { const entries=textEntries.slice();entries[index]=text;document.setControlValue(attributes.name,entries) }
    Rectangle { anchors.fill: parent; visible: root.tag==="View"; color: root.styleValue("background-color") || "transparent"; radius: parseFloat(root.styleValue("border-radius")) || 0; border.width: root.styleValue("border")?1:0; border.color: Theme.border }
    Loader { id: renderer; x: root.paddingSize; y: root.paddingSize; width: Math.max(0,parent.width-root.paddingSize*2); sourceComponent: root.hiddenTag?null:root.tag==="View"?viewComponent:(root.tag==="Header"||root.tag==="Text")?textComponent:["Choices","TextArea","Rating","Number"].indexOf(root.tag)>=0?controlComponent:unsupportedComponent }
    Component {
        id: viewComponent
        GridLayout {
            columns: /display\s*:\s*flex/.test(root.rawStyle)&&!/flex-direction\s*:\s*column/.test(root.rawStyle)?Math.max(1,(root.node.children || []).length):1
            columnSpacing: 12; rowSpacing: 10
            Repeater { model: root.node.children || []; Loader { required property var modelData; Layout.fillWidth: true; Layout.preferredHeight: item?item.implicitHeight:0; source: "ConfigNode.qml"; onLoaded: {item.node=modelData;item.document=root.document;item.taskData=root.taskData} Connections { target: root; function onTaskDataChanged(){if(item)item.taskData=root.taskData} } } }
        }
    }
    Component { id: textComponent; Label { text: root.resolve(root.attributes.value || root.node.text); wrapMode: Text.WordWrap; textFormat: Text.PlainText; color: root.styleValue("color") || Theme.text; font.pixelSize: parseFloat(root.styleValue("font-size")) || (root.tag==="Header"?20:15); font.weight: root.tag==="Header"?Font.DemiBold:Font.Normal } }
    Component {
        id: controlComponent
        ColumnLayout {
            spacing: 6
            Label { visible: root.attributes.required==="true"; text: qsTr("Required"); color: Theme.danger; font.pixelSize: 12 }
            Flow {
                Layout.fillWidth: true; spacing: 8; visible: root.tag==="Choices"
                Repeater { model: root.tag==="Choices" ? root.control.labels || [] : []; CheckBox { required property var modelData; readonly property string resultValue: modelData.resultValue === undefined || modelData.resultValue === null ? modelData.name : modelData.resultValue; text: modelData.display || modelData.name; checked: (root.values[root.attributes.name] || []).indexOf(resultValue)>=0; enabled: root.document&&root.document.editable
                    onClicked: {let selected=(root.values[root.attributes.name] || []).slice();if((root.attributes.choice || "single")!=="multiple")selected=checked?[resultValue]:[];else if(checked)selected.push(resultValue);else selected=selected.filter(v=>v!==resultValue);root.document.setControlValue(root.attributes.name,selected)}
                } }
            }
            Repeater {
                model: root.tag==="TextArea" ? Math.max(1,root.textEntries.length) : 0
                RowLayout {
                    id: textEntry
                    required property int index; Layout.fillWidth: true
                    ScrollView { Layout.fillWidth: true; Layout.preferredHeight: Math.max(70,Math.min(220,Number(root.attributes.rows || 3)*25+25)); StudioArea { objectName: "runtimeText"+root.attributes.name+"_"+textEntry.index; text: root.textEntries[textEntry.index] || ""; placeholderText: root.attributes.placeholder || ""; wrapMode: TextEdit.Wrap; enabled: root.document&&root.document.editable; onTextChanged: if(activeFocus)root.setTextEntry(textEntry.index,text) } }
                    StudioButton { visible: root.textEntries.length>1; text: "×"; quiet: true; enabled: root.document&&root.document.editable; onClicked: {const entries=root.textEntries.slice();entries.splice(textEntry.index,1);root.document.setControlValue(root.attributes.name,entries)} }
                }
            }
            StudioButton { visible: root.tag==="TextArea"&&Number(root.attributes.maxSubmissions || 1)>Math.max(1,root.textEntries.length); text: qsTr("Add text entry"); enabled: root.document&&root.document.editable; onClicked: {const entries=root.textEntries.slice();if(!entries.length)entries.push("");entries.push("");root.document.setControlValue(root.attributes.name,entries)} }
            SpinBox { visible: root.tag==="Rating"; from: 0; to: Number(root.attributes.maxRating || 5); value: root.tag==="Rating" ? Number(root.values[root.attributes.name] || 0) : 0; enabled: root.document&&root.document.editable; onValueModified: root.document.setControlValue(root.attributes.name,value) }
            StudioField { visible: root.tag==="Number"; Layout.fillWidth: true; text: root.tag!=="Number"||root.values[root.attributes.name]===undefined?"":String(root.values[root.attributes.name]); validator: DoubleValidator {} enabled: root.document&&root.document.editable; onEditingFinished: root.document.setControlValue(root.attributes.name,text.length?Number(text):null) }
        }
    }
    Component { id: unsupportedComponent; Label { text: qsTr("Native preview is unavailable for <%1>.").arg(root.tag); color: Theme.muted; wrapMode: Text.WordWrap; font.pixelSize: 13 } }
}
