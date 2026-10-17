import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id:root
    objectName:"projectPage"
    signal importRequested()
    signal settingsRequested()
    signal taskRequested(int id)
    signal labelingRequested()
    signal taskActionRequested(string actionId)
    property bool grid:false
    property bool descending:false
    property int statusFilter:0
    property bool showFilename:false
    property bool showCreated:false
    property bool filtersOpen:false
    property bool renameMode:false
    property int pendingView:0
    function switchView(id){if(dm.viewDirty){pendingView=id;unsavedView.open()}else dm.activateView(id)}
    readonly property var dm:backend.dataManager
    readonly property var rows:backend.tasks
    readonly property var columns:dm.visibleColumns
    readonly property int tableWidth:Math.max(dataArea.width,70+columns.reduce((s,c)=>s+Number(c.width),0))
    function queueQuery(){queryTimer.restart()}
    function resetQuery(){queryTimer.stop();search.text=backend.taskSearch;statusFilter=backend.taskStatus;descending=backend.taskDescending;queryTimer.stop()}
    function columnTitle(c){const titles={id:qsTr("ID"),inner_id:qsTr("Inner ID"),completed_at:qsTr("Completed"),total_annotations:qsTr("Annotations"),cancelled_annotations:qsTr("Cancelled"),total_predictions:qsTr("Predictions"),annotators:qsTr("Annotated by"),annotations_results:qsTr("Annotation results"),annotations_ids:qsTr("Annotation IDs"),predictions_score:qsTr("Prediction score"),predictions_model_versions:qsTr("Prediction model versions"),predictions_results:qsTr("Prediction results"),file_upload:qsTr("Upload filename"),storage_filename:qsTr("Storage filename"),created_at:qsTr("Created at"),updated_at:qsTr("Updated at"),updated_by:qsTr("Updated by"),avg_lead_time:qsTr("Lead time"),draft_exists:qsTr("Drafts")};return c.parent==="data"?c.title:(titles[c.id]||c.title)}
    function fieldKey(c){return c.parent==="data"?"data."+c.id:c.id}
    function orderBy(key){dm.applyQuery(dm.filters,[(descending?"-":"")+"tasks:"+key])}
    onStatusFilterChanged:queueQuery()
    onDescendingChanged:queueQuery()
    Timer{id:queryTimer;interval:350;onTriggered:{if(!root.visible||backend.demo)return;if(backend.busy){restart();return}backend.setTaskQuery(search.text,root.statusFilter,root.descending)}}
    Connections{target:backend;function onTaskQueryFailed(){root.resetQuery()}function onProjectOpened(){root.resetQuery()}function onProjectCreated(id){root.resetQuery()}function onProjectTabsReset(){root.resetQuery()}}
    ColumnLayout {
        anchors.fill:parent;spacing:0
        Rectangle {
            Layout.fillWidth:true;Layout.preferredHeight:43;color:Theme.subtle
            RowLayout {
                anchors.fill:parent;anchors.leftMargin:4;spacing:0
                StudioButton{text:qsTr("Default");quiet:dm.activeView!==0;primary:dm.activeView===0;Layout.preferredHeight:36;Layout.preferredWidth:130;enabled:!backend.busy;onClicked:root.switchView(0)}
                Repeater{model:dm.views;delegate:StudioButton{required property var modelData;text:(modelData.data||{}).title||qsTr("Untitled");quiet:dm.activeView!==modelData.id;primary:dm.activeView===modelData.id;Layout.preferredHeight:36;enabled:!backend.busy;onClicked:root.switchView(modelData.id)}}
                StudioButton{text:"+";quiet:true;enabled:!backend.busy&&!backend.demo;onClicked:{root.renameMode=false;viewName.text=qsTr("New tab");viewDialog.open()}}
                StudioButton{text:"⋯";quiet:true;enabled:!backend.busy&&!backend.demo;onClicked:viewMenu.open();Menu{id:viewMenu;MenuItem{text:qsTr("Save tab");enabled:dm.activeView>0;onTriggered:dm.saveView()}MenuItem{text:qsTr("Rename tab…");enabled:dm.activeView>0;onTriggered:{root.renameMode=true;viewName.text=(dm.views.find(v=>v.id===dm.activeView).data||{}).title||"";viewDialog.open()}}MenuItem{text:qsTr("Delete tab…");enabled:dm.activeView>0;onTriggered:deleteViewDialog.open()}}}
                Label{text:dm.viewDirty?qsTr("Unsaved tab changes"):"";color:Theme.muted;font.pixelSize:12}
                Item{Layout.fillWidth:true}
            }
            Rectangle{anchors.bottom:parent.bottom;width:parent.width;height:1;color:Theme.border}
        }
        Rectangle {
            Layout.fillWidth:true;Layout.preferredHeight:39;color:Theme.surface
            RowLayout{anchors.fill:parent;anchors.leftMargin:18;spacing:28
                Label{text:qsTr("Tasks: %1 / %2").arg(backend.totalTasks).arg(backend.project.task_number||backend.totalTasks);font.pixelSize:13}
                Label{text:qsTr("Submitted annotations: %1").arg(dm.totalAnnotations);font.pixelSize:13}
                Label{text:qsTr("Predictions: %1").arg(dm.totalPredictions);font.pixelSize:13}
                Item{Layout.fillWidth:true}
            }
        }
        Rectangle {
            Layout.fillWidth:true;Layout.preferredHeight:56;color:Theme.surface
            Flickable {
                anchors.fill:parent;anchors.margins:10;contentWidth:toolbar.implicitWidth;clip:true;flickableDirection:Flickable.HorizontalFlick
                RowLayout {
                    id:toolbar;height:parent.height;spacing:8
                    StudioButton{text:qsTr("Actions  ⌄");objectName:"taskActions";enabled:backend.selectedTaskCount>0&&!backend.busy;onClicked:actionsMenu.open();Menu{id:actionsMenu;Instantiator{model:dm.actions;delegate:MenuItem{required property var modelData;text:root.actionTitle(modelData.id,modelData.title);onTriggered:root.taskActionRequested(modelData.id)}onObjectAdded:(index,obj)=>actionsMenu.insertItem(index,obj);onObjectRemoved:(index,obj)=>actionsMenu.removeItem(obj)}}}
                    StudioButton{text:qsTr("Columns  ⌄");enabled:!backend.demo;onClicked:columnsPopup.open();DataColumns{id:columnsPopup;y:parent.height+4}}
                    StudioButton{text:qsTr("Filters")+(dm.filters.items.length?" ("+dm.filters.items.length+")":"")+"  ⌄";enabled:!backend.busy&&!backend.demo;onClicked:{root.filtersOpen=!root.filtersOpen;if(root.filtersOpen)filterPanel.reload()}}
                    ComboBox{id:order;implicitHeight:36;Layout.preferredWidth:160;model:dm.columns.filter(c=>c.id!=="data"&&c.type!=="List").map(c=>Object.assign({},c,{title:root.columnTitle(c)}));textRole:"title";displayText:qsTr("Order by: %1").arg(currentText);currentIndex:Math.max(0,model.findIndex(c=>"tasks:"+root.fieldKey(c)===String(dm.ordering[0]||"tasks:id").replace(/^-/ ,"")));enabled:!backend.busy;onActivated:root.orderBy(root.fieldKey(model[currentIndex]))}
                    StudioButton{text:String(dm.ordering[0]||"").startsWith("-")?"↓":"↑";enabled:!backend.busy;onClicked:{const current=String(dm.ordering[0]||"tasks:id");dm.applyQuery(dm.filters,[current.startsWith("-")?current.slice(1):"-"+current])}}
                    StudioButton{objectName:"startLabeling";text:backend.selectedTaskCount>0?qsTr("Label %1 Tasks").arg(backend.selectedTaskCount):qsTr("Label All Tasks");primary:true;enabled:backend.totalTasks>0&&!backend.busy&&!backend.demo&&!queryTimer.running&&search.text===backend.taskSearch&&root.statusFilter===backend.taskStatus&&root.descending===backend.taskDescending;onClicked:if(enabled)root.labelingRequested()}
                    StudioButton{glyph:"refresh";enabled:!backend.busy&&!backend.demo;onClicked:backend.loadPage(backend.page);ToolTip.visible:hovered;ToolTip.text:qsTr("Refresh data")}
                    StudioButton{text:qsTr("Import");enabled:!backend.busy&&!backend.demo;onClicked:root.importRequested()}
                    StudioButton{text:qsTr("Export");enabled:!backend.busy&&!backend.demo;onClicked:exportFile.open()}
                    StudioButton{glyph:root.grid?"list":"grid";onClicked:root.grid=!root.grid}
                    StudioButton{text:dm.compact?qsTr("Compact"):qsTr("Comfortable");onClicked:dm.compact=!dm.compact}
                }
            }
            Rectangle{anchors.bottom:parent.bottom;width:parent.width;height:1;color:Theme.border}
        }
        Rectangle {
            Layout.fillWidth:true;Layout.preferredHeight:42;color:Theme.surface
            RowLayout{anchors.fill:parent;anchors.leftMargin:18;anchors.rightMargin:18
                StudioField{id:search;objectName:"taskSearch";Layout.preferredWidth:260;implicitHeight:32;font.pixelSize:13;placeholderText:qsTr("Search filename or task ID…");enabled:!backend.busy&&!backend.demo;onTextChanged:root.queueQuery()}
                StudioButton{text:qsTr("Labeled");quiet:root.statusFilter!==1;onClicked:root.statusFilter=root.statusFilter===1?0:1;enabled:!backend.busy&&!backend.demo}
                StudioButton{text:qsTr("Unlabeled");quiet:root.statusFilter!==2;onClicked:root.statusFilter=root.statusFilter===2?0:2;enabled:!backend.busy&&!backend.demo}
                Item{Layout.fillWidth:true}
                Label{text:qsTr("%1 selected").arg(backend.selectedTaskCount);visible:backend.selectedTaskCount>0;color:Theme.accent}
                StudioButton{text:qsTr("Select all %1 results").arg(backend.totalTasks);visible:backend.totalTasks>0&&!backend.allResultsSelected;quiet:true;enabled:!backend.busy;onClicked:backend.selectAllResults()}
                StudioButton{text:qsTr("Clear selection");visible:backend.selectedTaskCount>0;quiet:true;enabled:!backend.busy;onClicked:backend.clearSelection()}
            }
        }
        RowLayout {
            id:dataArea;Layout.fillWidth:true;Layout.fillHeight:true;spacing:0
            Item {
                Layout.fillWidth:true;Layout.fillHeight:true
                Flickable {
                    id:table;anchors.fill:parent;visible:!root.grid;clip:true;contentWidth:root.tableWidth;contentHeight:height;flickableDirection:Flickable.HorizontalFlick
                    ScrollBar.horizontal:ScrollBar{}
                    Column {
                        width:root.tableWidth;height:table.height
                        Rectangle {
                            width:parent.width;height:42;color:Theme.subtle
                            Row {
                                anchors.fill:parent
                                Item{width:60;height:42;CheckBox{objectName:"selectPageTasks";anchors.centerIn:parent;enabled:!backend.busy&&!backend.demo;checked:backend.tasks.length>0&&backend.tasks.every(t=>{backend.selectedTaskCount;return backend.isTaskSelected(t.id)});onClicked:backend.selectPage(checked)}}
                                Repeater{model:root.columns;delegate:Rectangle{
                                    id:header
                                    required property var modelData
                                    width:modelData.width;height:42;color:"transparent"
                                    Label{anchors.fill:parent;anchors.leftMargin:12;anchors.rightMargin:10;verticalAlignment:Text.AlignVCenter;text:root.columnTitle(header.modelData);font.bold:true;font.pixelSize:13;elide:Text.ElideRight}
                                    MouseArea{anchors.fill:parent;anchors.rightMargin:8;onClicked:root.orderBy(header.modelData.key)}
                                    Rectangle{anchors.right:parent.right;width:1;height:parent.height;color:Theme.border}
                                    MouseArea{anchors.right:parent.right;width:8;height:parent.height;cursorShape:Qt.SplitHCursor;property real startX;property int startWidth;onPressed:mouse=>{startX=mapToItem(table,mouse.x,0).x;startWidth=header.width} onPositionChanged:mouse=>{if(pressed)dm.setColumnWidth(header.modelData.key,startWidth+mapToItem(table,mouse.x,0).x-startX)}}
                                }}
                            }
                        }
                        ListView {
                            id:taskList;width:parent.width;height:parent.height-42;clip:true;model:root.rows
                            ScrollBar.vertical:ScrollBar{}
                            delegate:ItemDelegate {
                                id:taskRow
                                required property var modelData
                                required property int index
                                width:taskList.width;height:dm.compact?42:76;enabled:!backend.busy;padding:0
                                Component.onCompleted:backend.requestThumbnail(modelData.id)
                                background:Rectangle{color:taskRow.hovered?Theme.selected:taskRow.index%2?Theme.subtle:Theme.surface;Rectangle{anchors.bottom:parent.bottom;width:parent.width;height:1;color:Theme.border}}
                                contentItem:Row {
                                    Item{width:60;height:taskRow.height;CheckBox{objectName:"selectTask"+taskRow.modelData.id;anchors.centerIn:parent;enabled:!backend.demo;checked:{backend.selectedTaskCount;return backend.isTaskSelected(taskRow.modelData.id)} onClicked:backend.selectTask(taskRow.modelData.id,checked)}}
                                    Repeater{model:root.columns;delegate:Item{
                                        id:cell
                                        required property var modelData
                                        width:modelData.width;height:taskRow.height
                                        Component.onCompleted:if(modelData.type==="Image")backend.requestThumbnail(taskRow.modelData.id,modelData.key)
                                        Image{anchors.fill:parent;anchors.margins:5;visible:cell.modelData.type==="Image";fillMode:Image.PreserveAspectFit;source:backend.thumbnails[String(taskRow.modelData.id)+":"+cell.modelData.key]||""}
                                        Label{anchors.fill:parent;anchors.leftMargin:12;anchors.rightMargin:8;verticalAlignment:Text.AlignVCenter;visible:cell.modelData.type!=="Image";text:dm.cellText(taskRow.modelData,cell.modelData);elide:Text.ElideRight;font.pixelSize:13;color:cell.modelData.key==="id"?Theme.accent:Theme.text;ToolTip.visible:hoverArea.containsMouse&&truncated;ToolTip.text:text;MouseArea{id:hoverArea;anchors.fill:parent;hoverEnabled:true;acceptedButtons:Qt.NoButton}}
                                    }}
                                }
                                onClicked:root.taskRequested(modelData.id)
                            }
                        }
                    }
                }
                GridView {
                    id:taskGrid;anchors.fill:parent;anchors.margins:14;visible:root.grid;clip:true;model:root.rows;cellWidth:250;cellHeight:dm.compact?190:244
                    ScrollBar.vertical:ScrollBar{}
                    delegate:ItemDelegate{
                        id:tile;required property var modelData;width:238;height:taskGrid.cellHeight-12;enabled:!backend.busy
                        Component.onCompleted:backend.requestThumbnail(modelData.id)
                        background:Rectangle{color:tile.hovered?Theme.selected:Theme.subtle;border.color:Theme.border;radius:4}
                        contentItem:ColumnLayout{
                            Image{source:backend.thumbnails[String(tile.modelData.id)]||"";Layout.fillWidth:true;Layout.fillHeight:true;fillMode:Image.PreserveAspectFit}
                            RowLayout{CheckBox{enabled:!backend.demo;checked:{backend.selectedTaskCount;return backend.isTaskSelected(tile.modelData.id)} onClicked:backend.selectTask(tile.modelData.id,checked)}Label{text:"#"+tile.modelData.id;Layout.fillWidth:true;font.bold:true}Label{text:tile.modelData.is_labeled?qsTr("Done"):qsTr("Unlabeled");color:Theme.muted}}
                            Label{text:JSON.stringify(tile.modelData.data||{});Layout.fillWidth:true;elide:Text.ElideMiddle;font.pixelSize:12;color:Theme.muted}
                        }
                        onClicked:root.taskRequested(modelData.id)
                    }
                }
                ColumnLayout{
                    anchors.centerIn:parent;width:Math.min(650,parent.width-50);visible:backend.totalTasks===0
                    Label{Layout.fillWidth:true;text:backend.taskSearch||backend.taskStatus||dm.filters.items.length?qsTr("No tasks match these filters."):qsTr("Import data to get your project started");font.pixelSize:24;horizontalAlignment:Text.AlignHCenter;wrapMode:Text.WordWrap}
                    StudioButton{text:qsTr("Import");Layout.alignment:Qt.AlignHCenter;primary:true;enabled:!backend.demo;onClicked:root.importRequested()}
                }
            }
            DataFilters{id:filterPanel;objectName:"pinnedDataFilters";Layout.preferredWidth:Math.min(490,root.width*.48);Layout.fillHeight:true;visible:root.filtersOpen&&dm.pinned;onCloseRequested:root.filtersOpen=false}
        }
        Rectangle{
            Layout.fillWidth:true;Layout.preferredHeight:48;color:Theme.surface
            Rectangle{width:parent.width;height:1;color:Theme.border}
            RowLayout{anchors.fill:parent;anchors.leftMargin:18;anchors.rightMargin:18
                Label{text:qsTr("Tasks: %1 / %2").arg(root.rows.length).arg(backend.totalTasks);color:Theme.muted;font.pixelSize:12}
                Item{Layout.fillWidth:true}
                Label{text:qsTr("Per page");font.pixelSize:12;color:Theme.muted}
                ComboBox{objectName:"taskPageSize";implicitHeight:34;Layout.preferredWidth:85;model:[10,25,50,100,250,500,1000];currentIndex:model.indexOf(dm.pageSize);enabled:!backend.busy&&!backend.demo;onActivated:dm.setPageSize(Number(currentText))}
                StudioButton{text:"‹";quiet:true;enabled:backend.page>1&&!backend.busy&&!backend.demo;onClicked:backend.loadPage(backend.page-1)}
                Label{text:qsTr("Page %1 of %2").arg(backend.page).arg(Math.max(1,Math.ceil(backend.totalTasks/dm.pageSize)));color:Theme.muted;font.pixelSize:12}
                StudioButton{text:"›";quiet:true;enabled:backend.page*dm.pageSize<backend.totalTasks&&!backend.busy&&!backend.demo;onClicked:backend.loadPage(backend.page+1)}
            }
        }
    }
    function actionTitle(id,fallback){const names={delete_tasks:qsTr("Delete Tasks"),delete_tasks_annotations:qsTr("Delete Annotations"),delete_tasks_predictions:qsTr("Delete Predictions"),retrieve_tasks_predictions:qsTr("Retrieve Predictions"),predictions_to_annotations:qsTr("Create Annotations From Predictions"),remove_duplicates:qsTr("Remove Duplicated Tasks")};return names[id]||fallback}
    Popup{id:filtersPopup;x:Math.min(170,root.width-width);y:100;width:Math.min(680,root.width-20);height:410;padding:0;visible:root.filtersOpen&&!dm.pinned;closePolicy:Popup.CloseOnEscape|Popup.CloseOnPressOutside;onClosed:if(!dm.pinned)root.filtersOpen=false;DataFilters{id:popupFilterPanel;objectName:"popupDataFilters";anchors.fill:parent;onCloseRequested:root.filtersOpen=false}onOpened:popupFilterPanel.reload()}
    Dialog{id:unsavedView;parent:Overlay.overlay;anchors.centerIn:parent;modal:true;width:440;title:qsTr("Unsaved tab changes");standardButtons:Dialog.Discard|Dialog.Cancel;Label{text:qsTr("Discard changes to filters and columns and switch tabs?");wrapMode:Text.WordWrap}onDiscarded:{dm.activateView(root.pendingView);close()}}
    DataActionDialog{id:actionDialog;objectName:"taskActionConfirm"}
    Dialog{id:viewDialog;parent:Overlay.overlay;anchors.centerIn:parent;modal:true;width:420;title:root.renameMode?qsTr("Rename tab"):qsTr("New tab");standardButtons:Dialog.Save|Dialog.Cancel;StudioField{id:viewName;width:parent.width;placeholderText:qsTr("Tab name")} onAccepted:root.renameMode?dm.renameView(viewName.text):dm.createView(viewName.text)}
    Dialog{id:deleteViewDialog;parent:Overlay.overlay;anchors.centerIn:parent;modal:true;width:420;title:qsTr("Delete tab");standardButtons:Dialog.Yes|Dialog.Cancel;Label{text:qsTr("Delete this saved tab? Tasks will be kept.")} onAccepted:dm.deleteView()}
    FileDialog{id:exportFile;title:qsTr("Export annotations");fileMode:FileDialog.SaveFile;defaultSuffix:"json";nameFilters:[qsTr("Label Studio JSON (*.json)")];onAccepted:backend.exportProject(selectedFile)}
}
