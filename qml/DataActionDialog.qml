import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Dialog {
    id:root
    property var controller:backend.dataManager
    property var action:({})
    property var values:({})
    parent:Overlay.overlay;anchors.centerIn:parent;modal:true
    function actionTitle(){const titles={delete_tasks:qsTr("Delete Tasks"),delete_tasks_annotations:qsTr("Delete Annotations"),delete_tasks_predictions:qsTr("Delete Predictions"),retrieve_tasks_predictions:qsTr("Retrieve Predictions"),predictions_to_annotations:qsTr("Create Annotations From Predictions"),remove_duplicates:qsTr("Remove Duplicated Tasks")};return titles[action.id]||action.title||qsTr("Confirm bulk action")}
    function actionDescription(){const texts={delete_tasks:qsTr("Delete the selected tasks and their annotations?"),delete_tasks_annotations:qsTr("Delete annotations and drafts from the selected tasks. Choose a person to delete only their work, or All to delete everyone's work."),delete_tasks_predictions:qsTr("Delete all predictions from the selected tasks?"),retrieve_tasks_predictions:qsTr("Send selected tasks to all ML backends connected to this project. The request can time out; refresh after processing finishes."),predictions_to_annotations:qsTr("Create annotations from the selected prediction model version. Your account will own the new annotations."),remove_duplicates:qsTr("Remove tasks with identical data. The server merges annotations into the retained task. Refresh when processing finishes.")};return texts[action.id]||(action.dialog||{}).text||qsTr("Please confirm your action.")}
    width:Math.min(560,parent.width-40);title:actionTitle()
    standardButtons:Dialog.Yes|Dialog.Cancel
    function openAction(){action=controller.actions.find(a=>a.id===controller.actionId)||{};values={};for(const f of controller.actionFields)if(f.value!==undefined&&f.value!==null&&f.value!=="")values[f.name]=f.value;open()}
    Connections{target:root.controller;function onActionReady(){root.openAction()}}
    contentItem:ColumnLayout{
        spacing:14
        Label{Layout.fillWidth:true;text:qsTr("%1 selected tasks").arg(backend.selectedTaskCount);font.bold:true}
        Label{Layout.fillWidth:true;text:root.actionDescription();wrapMode:Text.WordWrap}
        Repeater{
            model:root.controller.actionFields
            delegate:ColumnLayout{
                required property var modelData
                Layout.fillWidth:true
                Label{text:modelData.name==="annotator"?qsTr("Annotator"):modelData.name==="model_version"?qsTr("Choose predictions"):modelData.label||modelData.name}
                ComboBox{
                    Layout.fillWidth:true
                    readonly property var options:[{value:"",label:qsTr("All")}].concat((modelData.options||[]).map(v=>typeof v==="object"?v:{value:v,label:String(v)}))
                    model:options;textRole:"label"
                    currentIndex:Math.max(0,options.findIndex(v=>String(v.value)===String(modelData.value||"")))
                    onActivated:{let copy=Object.assign({},root.values);const value=options[currentIndex].value;if(value==="")delete copy[modelData.name];else copy[modelData.name]=value;root.values=copy}
                }
            }
        }
        Label{Layout.fillWidth:true;visible:String(root.action.id).startsWith("delete")||root.action.id==="remove_duplicates";text:qsTr("This cannot be undone.");color:Theme.danger}
    }
    onAccepted:controller.executeAction(action.id,values)
}
