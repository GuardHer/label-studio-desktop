import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Popup {
    id:root
    property var controller:backend.dataManager
    function columnTitle(c){return c.parent==="data"?c.title:qsTranslate("ProjectPage",c.id==="avg_lead_time"?"Lead time":c.title)}
    width:380;height:Math.min(580,Overlay.overlay.height-100);padding:14
    closePolicy:Popup.CloseOnEscape|Popup.CloseOnPressOutside
    background:Rectangle{color:Theme.surface;border.color:Theme.border;radius:5}
    ColumnLayout{
        anchors.fill:parent;spacing:6
        Label{text:qsTr("Columns");font.bold:true;font.pixelSize:16}
        Label{text:qsTr("Choose columns and change their order.");color:Theme.muted;font.pixelSize:12}
        ListView{
            Layout.fillHeight:true;Layout.fillWidth:true;clip:true;spacing:2
            model:root.controller.columnOrder.map(key=>root.controller.columns.find(c=>(c.parent==="data"?"data."+c.id:c.id)===key)).filter(c=>c&&c.id!=="data")
            ScrollBar.vertical:ScrollBar{}
            delegate:RowLayout{
                required property var modelData
                readonly property string key:modelData.parent==="data"?"data."+modelData.id:modelData.id
                width:ListView.view.width
                CheckBox{Layout.fillWidth:true;text:root.columnTitle(modelData);checked:root.controller.visibleColumns.some(c=>c.key===parent.key);onClicked:root.controller.setColumnVisible(parent.key,checked)}
                StudioButton{text:"↑";quiet:true;onClicked:root.controller.moveColumn(parent.key,-1)}
                StudioButton{text:"↓";quiet:true;onClicked:root.controller.moveColumn(parent.key,1)}
            }
        }
        CheckBox{text:qsTr("Compact rows");checked:root.controller.compact;onClicked:root.controller.compact=checked}
    }
}
