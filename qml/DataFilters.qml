import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property var controller: backend.dataManager
    function columnTitle(c){return c.parent==="data"?c.title:qsTranslate("ProjectPage",c.id==="avg_lead_time"?"Lead time":c.title)}
    property string error: ""
    property bool loading: false
    signal closeRequested()
    color: Theme.surface; border.color: Theme.border; radius: 4
    implicitHeight: 330
    function reload() {
        loading=true; draft.clear()
        const f=controller.filters
        conjunction.currentIndex=f.conjunction==="or"?1:0
        for (const row of (f.items || [])) {
            const key=String(row.filter).replace("filter:tasks:", "")
            let op=row.operator, value=row.value, end=""
            if (op==="empty") { op=value?"empty":"not_empty"; value="" }
            else if(op==="in"||op==="not_in"){end=String(value.max);value=String(value.min)}
            else if (typeof value==="object") value=JSON.stringify(value)
            const column=controller.columns.find(c=>(c.parent==="data"?"data."+c.id:c.id)===key)||{}
            draft.append({fieldKey:key, operation:op, valueType:row.type||column.type||"String", valueText:String(value), endText:end, children:JSON.stringify(row.child_filters||[])})
        }
        loading=false;error=""
    }
    function addFilter(){const columns=controller.columns.filter(c=>c.id!=="data");if(!columns.length)return;const c=columns[0];const key=c.parent==="data"?"data."+c.id:c.id;draft.append({fieldKey:key,operation:controller.operators(key)[0].key,valueType:c.type||"String",valueText:"",endText:"",children:"[]"})}
    function draftFilters(){let rows=[];for(let i=0;i<draft.count;i++){const row=draft.get(i);rows.push(controller.makeFilter(row.fieldKey,row.operation,row.valueText,row.endText,row.valueType))}return rows}
    function apply() {
        let rows=[]
        for(let i=0;i<draft.count;i++){
            const row=draft.get(i), item=controller.makeFilter(row.fieldKey,row.operation,row.valueText,row.endText,row.valueType)
            if(!item.filter){error=qsTr("Check filter values. Use ISO dates, numeric values, or JSON lists where required.");return}
            const child=JSON.parse(row.children);if(child.length)item.child_filters=child
            rows.push(item)
        }
        if(controller.applyQuery({conjunction:conjunction.currentIndex===0?"and":"or",items:rows},controller.ordering)){error="";if(!controller.pinned)closeRequested()}
    }
    Component.onCompleted: reload()
    onVisibleChanged:if(visible)Qt.callLater(reload)
    Connections{target:root.controller;function onQueryRestored(){root.reload()}function onChanged(){if(!root.visible)root.reload()}}
    ListModel{id:draft;dynamicRoles:true}
    ColumnLayout {
        anchors.fill:parent;anchors.margins:16;spacing:8
        RowLayout {
            Label{text:qsTr("Filters");font.pixelSize:16;font.bold:true;Layout.fillWidth:true}
            StudioButton{text:root.controller.pinned?qsTr("Unpin filters"):qsTr("Pin filters");quiet:true;onClicked:{root.controller.pinned=!root.controller.pinned;root.apply()}}
            StudioButton{text:"×";quiet:true;onClicked:root.closeRequested()}
        }
        RowLayout {
            Label{text:qsTr("Where")}
            ComboBox{id:conjunction;model:[qsTr("All conditions (AND)"),qsTr("Any condition (OR)")];Layout.fillWidth:true;implicitHeight:34}
        }
        ListView {
            Layout.fillWidth:true;Layout.fillHeight:true;model:draft;spacing:8;clip:true
            ScrollBar.vertical:ScrollBar{}
            delegate:ColumnLayout {
                id:line
                required property int index
                required property string fieldKey
                required property string operation
                required property string valueType
                required property string valueText
                required property string endText
                width:ListView.view.width;spacing:4
                readonly property var fields:root.controller.columns.filter(c=>c.id!=="data").map(c=>Object.assign({},c,{title:root.columnTitle(c)}))
                readonly property var field:fields.find(c=>(c.parent==="data"?"data."+c.id:c.id)===fieldKey)||{}
                readonly property var operators:root.controller.operators(fieldKey,valueType)
                RowLayout {
                    Layout.fillWidth:true;spacing:6
                    ComboBox {
                        Layout.fillWidth:true;Layout.minimumWidth:120;implicitHeight:34;model:line.fields;textRole:"title"
                        currentIndex:Math.max(0,line.fields.findIndex(c=>(c.parent==="data"?"data."+c.id:c.id)===line.fieldKey))
                        onActivated:{const c=line.fields[currentIndex],key=c.parent==="data"?"data."+c.id:c.id;draft.setProperty(line.index,"fieldKey",key);draft.setProperty(line.index,"valueType",c.type||"String");const ops=root.controller.operators(key);draft.setProperty(line.index,"operation",ops[0].key);draft.setProperty(line.index,"valueText","");draft.setProperty(line.index,"endText","");draft.setProperty(line.index,"children","[]")}
                    }
                    ComboBox {
                        Layout.preferredWidth:150;implicitHeight:34;model:line.operators;textRole:"title"
                        currentIndex:Math.max(0,line.operators.findIndex(o=>o.key===line.operation))
                        onActivated:draft.setProperty(line.index,"operation",line.operators[currentIndex].key)
                    }
                    StudioButton{text:"×";quiet:true;onClicked:draft.remove(line.index)}
                }
                ComboBox {
                    objectName:"filterValueType"
                    visible:line.field.type==="Unknown"
                    Layout.fillWidth:true;implicitHeight:32
                    readonly property var types:[{key:"String",title:qsTr("Text")},{key:"Number",title:qsTr("Number")},{key:"Datetime",title:qsTr("Date and time")},{key:"Boolean",title:qsTr("Boolean")},{key:"List",title:qsTr("List")}]
                    model:types;textRole:"title"
                    currentIndex:Math.max(0,types.findIndex(t=>t.key===line.valueType))
                    onActivated:{const type=types[currentIndex].key;draft.setProperty(line.index,"valueType",type);draft.setProperty(line.index,"operation",root.controller.operators(line.fieldKey,type)[0].key);draft.setProperty(line.index,"valueText","");draft.setProperty(line.index,"endText","")}
                }
                RowLayout {
                    Layout.fillWidth:true;visible:line.operation!=="empty"&&line.operation!=="not_empty"
                    StudioField {
                        Layout.fillWidth:true;implicitHeight:34;font.pixelSize:13;text:line.valueText
                        placeholderText:line.operation==="in_list"||line.operation==="not_in_list"?qsTr("JSON list, e.g. [1, 2]"):line.valueType==="Datetime"?qsTr("YYYY-MM-DDTHH:mm:ssZ"):line.valueType==="List"?qsTr("Values separated by commas"):line.valueType==="Boolean"?qsTr("true / false"):qsTr("Value")
                        onTextEdited:draft.setProperty(line.index,"valueText",text)
                    }
                    StudioField {
                        Layout.fillWidth:true;visible:line.operation==="in"||line.operation==="not_in";implicitHeight:34;font.pixelSize:13;text:line.endText;placeholderText:qsTr("Maximum / end date")
                        onTextEdited:draft.setProperty(line.index,"endText",text)
                    }
                    ComboBox {
                        visible:["annotators","updated_by","skipped_by"].indexOf(line.fieldKey)>=0
                        implicitHeight:34;Layout.preferredWidth:150;model:root.controller.actors;textRole:"email";displayText:qsTr("Add person…")
                        onActivated:{const id=String(model[currentIndex].id);let values=line.valueText.split(",").filter(x=>x);if(values.indexOf(id)<0)values.push(id);draft.setProperty(line.index,"valueText",values.join(","))}
                    }
                }
            }
            Label{anchors.centerIn:parent;visible:draft.count===0;text:qsTr("No filters applied");color:Theme.muted}
        }
        Label{Layout.fillWidth:true;text:root.error;visible:text.length>0;color:Theme.danger;wrapMode:Text.WordWrap;font.pixelSize:12}
        RowLayout {
            StudioButton{text:qsTr("Add filter");onClicked:root.addFilter()}
            Item{Layout.fillWidth:true}
            StudioButton{text:qsTr("Copy");quiet:true;onClicked:root.controller.copyFilters()}
            StudioButton{text:qsTr("Paste");quiet:true;onClicked:{root.controller.pasteFilters();root.reload()}}
            StudioButton{text:qsTr("JSON");quiet:true;onClicked:{jsonText.text=root.controller.filtersJson();jsonDialog.open()}}
            StudioButton{text:qsTr("Apply");primary:true;enabled:!backend.busy;onClicked:root.apply()}
        }
    }
    Dialog {
        id:jsonDialog;parent:Overlay.overlay;anchors.centerIn:parent;modal:true;width:Math.min(650,parent.width-40);height:450;title:qsTr("Filter JSON");standardButtons:Dialog.Apply|Dialog.Cancel
        ScrollView{anchors.fill:parent;TextArea{id:jsonText;wrapMode:TextEdit.Wrap;font.family:"Consolas";selectByMouse:true}}
        onApplied:{if(root.controller.applyFiltersJson(jsonText.text)){root.reload();close()}}
    }
}
