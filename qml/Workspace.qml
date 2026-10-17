import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window

Item {
    id: root
    objectName: "annotationWorkspace"
    property bool quickView: false
    property int sourceSection: 0
    signal expandRequested()
    signal closeRequested()
    function sourceObject() {
        const task = backend.taskSnapshot
        if (sourceSection === 1) return task.data || ({})
        if (sourceSection === 2) return task.annotations || []
        if (sourceSection === 3) return task.predictions || []
        return task
    }
    function taskMetadata() {
        const task = backend.taskSnapshot
        const format = value => value === undefined || value === null ? "—" : (typeof value === "object" ? JSON.stringify(value) : String(value))
        return [
            {title: qsTr("Task ID"), value: format(task.id || backend.taskId)},
            {title: qsTr("Project"), value: format(task.project || backend.project.id)},
            {title: qsTr("Created"), value: format(task.created_at)},
            {title: qsTr("Updated"), value: format(task.updated_at)},
            {title: qsTr("Updated by"), value: format(task.updated_by)},
            {title: qsTr("Annotated by"), value: format((task.annotations || []).map(a=>a.completed_by))},
            {title: qsTr("Prediction scores"), value: format((task.predictions || []).map(p=>p.score))},
            {title: qsTr("Prediction model versions"), value: format((task.predictions || []).map(p=>p.model_version))},
            {title: qsTr("Labeled"), value: task.is_labeled === undefined ? "—" : task.is_labeled ? qsTr("Yes") : qsTr("No")},
            {title: qsTr("Annotations"), value: format(task.total_annotations === undefined ? (task.annotations || []).length : task.total_annotations)},
            {title: qsTr("Predictions"), value: format(task.total_predictions === undefined ? (task.predictions || []).length : task.total_predictions)},
            {title: qsTr("Cancelled annotations"), value: format(task.cancelled_annotations)},
            {title: qsTr("Overlap"), value: format(task.overlap)},
            {title: qsTr("Data fields"), value: Object.keys(task.data || {}).join(", ") || "—"},
            {title: qsTr("Metadata"), value: format(task.meta)}
        ]
    }
    property var layoutNodes: []
    readonly property bool hasImages: annotationDocument.imageFrames.length > 0
    readonly property bool hasShapes: annotationDocument.controls.some(c => c.type === "rectanglelabels" || c.type === "polygonlabels")
    readonly property bool hasRuntimeLayout: containsRuntimeNode(layoutNodes)
    function containsRuntimeNode(nodes) {
        for (const node of nodes) {
            if (["Header", "Text", "Choices", "TextArea", "Rating", "Number"].indexOf(node.tag) >= 0) return true
            if (containsRuntimeNode(node.children || [])) return true
        }
        return false
    }
    function loadRuntimeLayout() {
        canvas.cancelDraft()
        canvas.selectedIndex = -1
        layoutNodes = annotationDocument.layoutTree
    }
    Component.onCompleted: loadRuntimeLayout()
    property string activeLabel: ""
    property bool submitAndContinue: true
    property string tool: "draw"
    property var pendingAction: null
    property int keyRevision: 0
    readonly property bool textEditing: {
        let item = root.Window.window ? root.Window.window.activeFocusItem : null
        while (item) {
            if (typeof item.cursorPosition === "number" && typeof item.text === "string") return true
            item = item.parent
        }
        return false
    }
    function key(action,fallback) { const revision=keyRevision; return backend.account.keySequence(action,fallback) }
    Connections { target: backend.account; function onHotkeysChanged() { root.keyRevision++ } }
    readonly property bool hasDraft: canvas.polygonDraft.length > 0
    function cancelDraft() { canvas.cancelDraft() }
    readonly property int taskIndex: {
        const rows = backend.tasks
        for (let i = 0; i < rows.length; ++i) if (rows[i].id === backend.taskId) return i
        return -1
    }
    function openConnection() { connection.open() }
    function guard(action) {
        if (backend.busy) return
        if (annotationDocument.dirty || hasDraft) { pendingAction = action; discardDialog.open() }
        else action()
    }
    function moveTask(delta) { guard(function() { backend.moveTask(delta) }) }
    Connections {
        target: backend
        function onWorkflowFinished() { finishedDialog.open() }
        function onTaskOpened() { root.loadRuntimeLayout() }
    }
    function selectedRow() {
        return canvas.selectedIndex >= 0 && canvas.selectedIndex < annotationDocument.rectangles.length ? annotationDocument.rectangles[canvas.selectedIndex] : null
    }
    Connections {
        target: annotationDocument
        function onChanged() {
            const labels = annotationDocument.labels
            for (let i = 0; i < labels.length; ++i) if (labels[i].name === root.activeLabel) return
            root.activeLabel = labels.length ? labels[0].name : ""
        }
    }
    Shortcut { sequence: root.key("editor:save","Ctrl+S"); enabled: root.visible && sequence.length>0 && !backend.busy && !connection.visible && !root.textEditing && !root.hasDraft; onActivated: backend.save() }
    Shortcut { sequence: root.key("annotation:submit","Ctrl+Return"); enabled: root.visible && sequence.length>0 && !backend.busy && !connection.visible && !root.textEditing && !root.hasDraft; onActivated: { if(root.submitAndContinue)backend.saveAndNext();else backend.save() } }
    Shortcut { sequence: root.key("annotation:undo","Ctrl+Z"); enabled: root.visible && sequence.length>0 && !backend.busy && !connection.visible && !root.textEditing; onActivated: annotationDocument.undo() }
    Shortcut { sequence: root.key("annotation:redo","Ctrl+Shift+Z"); enabled: root.visible && sequence.length>0 && !backend.busy && !connection.visible && !root.textEditing; onActivated: annotationDocument.redo() }
    Shortcut { sequence: "Ctrl+Y"; enabled: root.visible && root.key("annotation:redo","Ctrl+Shift+Z")==="Ctrl+Shift+Z" && !backend.busy && !connection.visible && !root.textEditing; onActivated: annotationDocument.redo() }
    Shortcut { sequence: root.key("region:delete","Delete"); enabled: root.visible && sequence.length>0 && !backend.busy && !connection.visible && !root.textEditing; onActivated: annotationDocument.removeRectangle(canvas.selectedIndex) }
    Shortcut { sequence: root.key("tool:move",root.key("native:select","V")); enabled: root.visible && sequence.length>0 && !connection.visible && !root.textEditing; onActivated: root.tool = "select" }
    Shortcut { sequence: root.key(annotationDocument.shapeType==="polygonlabels"?"tool:polygon":"tool:rect",root.key("native:draw","R")); enabled: root.visible && sequence.length>0 && !connection.visible && !root.textEditing; onActivated: root.tool = "draw" }
    Shortcut { sequence: root.key("tool:pan-image",root.key("native:pan","H")); enabled: root.visible && sequence.length>0 && !connection.visible && !root.textEditing; onActivated: root.tool = "pan" }
    RowLayout {
        anchors.fill: parent; spacing: 0
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
            RowLayout {
                Layout.fillWidth: true; Layout.leftMargin: 14; Layout.rightMargin: 14; spacing: 6
                visible: backend.taskId >= 0
                Label { text: root.quickView ? qsTr("Quick view · Task #%1").arg(backend.taskId) : qsTr("Task #%1").arg(backend.taskId); Layout.fillWidth: true; elide: Text.ElideRight; font.weight: Font.DemiBold }
                StudioButton { objectName: "taskSourceButton"; text: qsTr("Task source"); quiet: true; onClicked: sourceDialog.open() }
                StudioButton { objectName: "taskDetailsButton"; text: qsTr("Details"); quiet: true; onClicked: detailsDialog.open() }
                StudioButton { objectName: "expandTaskQuickView"; visible: root.quickView; text: qsTr("Expand"); quiet: true; onClicked: root.expandRequested() }
                StudioButton { objectName: "closeTaskQuickView"; visible: root.quickView; text: "×"; quiet: true; onClicked: root.closeRequested(); ToolTip.visible: hovered; ToolTip.text: qsTr("Close quick view") }
            }
            Rectangle {
                Layout.fillWidth: true; implicitHeight: warning.implicitHeight + 24; visible: annotationDocument.error.length > 0 || annotationDocument.validationError.length > 0 || backend.messageIsError
                color: Theme.dark ? "#492d26" : "#fff0eb"
                Label { id: warning; x: 14; y: 12; width: parent.width - 28; text: annotationDocument.error || annotationDocument.validationError || backend.message; wrapMode: Text.WordWrap; color: Theme.danger; font.pixelSize: 14 }
            }
            Label {
                visible: backend.samplingQueue; Layout.leftMargin: 14; Layout.topMargin: 8; text: qsTr("Project sampling queue"); color: Theme.accent; font.pixelSize: 13
            }
            Label {
                visible: root.hasShapes
                Layout.fillWidth: true; Layout.margins: 14; Layout.bottomMargin: 8
                text: canvas.polygonDraft.length ? qsTr("Click the first point or press Enter to finish · Esc to cancel") : qsTr("Select label and click the image to start"); font.pixelSize: 24; font.weight: Font.DemiBold
            }
            RowLayout {
                Layout.fillWidth: true; Layout.leftMargin: 14; Layout.rightMargin: 14
                visible: root.hasImages && (annotationDocument.imageFrames.length > 1 || drawingControls.model.length > 1)
                ComboBox {
                    objectName: "imageFrameSelector"; visible: annotationDocument.imageFrames.length > 1
                    Layout.fillWidth: true
                    model: annotationDocument.imageFrames; textRole: "name"
                    currentIndex: annotationDocument.activeImage
                    displayText: currentIndex >= 0 ? model[currentIndex].name + " · " + (model[currentIndex].itemIndex + 1) : ""
                    delegate: ItemDelegate { required property var modelData; required property int index; width: ListView.view.width; text: modelData.name + " · " + (modelData.itemIndex + 1); highlighted: index === annotationDocument.activeImage }
                    enabled: !backend.busy
                    onActivated: { canvas.cancelDraft(); canvas.selectedIndex = -1; annotationDocument.selectImage(currentIndex) }
                }
                ComboBox {
                    id: drawingControls
                    objectName: "drawingControlSelector"; Layout.fillWidth: true
                    visible: model.length > 1
                    model: annotationDocument.controls.filter(c => c.type === "rectanglelabels" || c.type === "polygonlabels")
                    textRole: "name"; valueRole: "name"
                    currentIndex: { const name=annotationDocument.controls[annotationDocument.activeControl]?.name; for(let i=0;i<model.length;++i)if(model[i].name===name)return i;return -1 }
                    enabled: !backend.busy
                    onActivated: { canvas.cancelDraft(); canvas.selectedIndex=-1; const all=annotationDocument.controls; for(let i=0;i<all.length;++i)if(all[i].name===currentValue)annotationDocument.selectControl(i) }
                }
            }
            RowLayout {
                visible: root.hasImages
                Layout.fillWidth: true; Layout.fillHeight: false; Layout.preferredHeight: visible ? Math.min(root.height - (root.hasRuntimeLayout ? 320 : 160), Math.max(260, (width - 62) / (canvas.imageSize.width > 0 ? canvas.imageSize.width / canvas.imageSize.height : 1.6))) : 0
                Layout.maximumHeight: Layout.preferredHeight
                Layout.leftMargin: 14; Layout.rightMargin: 10; spacing: 2
                AnnotationCanvas {
                    id: canvas; Layout.fillWidth: true; Layout.fillHeight: true
                    document: annotationDocument; imageSource: backend.imageSource; shortcutsEnabled: !root.textEditing
                    tool: root.tool; activeLabel: root.activeLabel; enabled: !backend.busy
                    inset: 0; backgroundColor: Theme.surface
                }
                Rectangle {
                    Layout.preferredWidth: 44; Layout.preferredHeight: tools.implicitHeight + 8; Layout.alignment: Qt.AlignTop; Layout.topMargin: 40
                    color: Theme.surface; border.color: Theme.border; radius: 6
                    ColumnLayout {
                        id: tools; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 4; spacing: 1
                        StudioButton { visible: root.hasShapes; glyph: "cursor"; quiet: root.tool !== "select"; primary: root.tool === "select"; Layout.fillWidth: true; leftPadding: 4; rightPadding: 4; onClicked: root.tool = "select"; ToolTip.visible: hovered; ToolTip.text: qsTr("Select / move (V)") }
                        StudioButton { visible: root.hasShapes; glyph: canvas.polygonMode ? "polygon-tool" : "rectangle-tool"; quiet: root.tool !== "draw"; primary: root.tool === "draw"; Layout.fillWidth: true; leftPadding: 4; rightPadding: 4; onClicked: root.tool = "draw"; ToolTip.visible: hovered; ToolTip.text: canvas.polygonMode ? qsTr("Draw polygon (R)") : qsTr("Draw rectangle (R)") }
                        StudioButton { glyph: "hand-tool"; quiet: root.tool !== "pan"; primary: root.tool === "pan"; Layout.fillWidth: true; leftPadding: 4; rightPadding: 4; onClicked: root.tool = "pan"; ToolTip.visible: hovered; ToolTip.text: qsTr("Pan (H)") }
                        StudioButton { glyph: "zoom-in"; quiet: true; Layout.fillWidth: true; leftPadding: 4; rightPadding: 4; onClicked: canvas.zoomAt(1.2, canvas.width/2, canvas.height/2) }
                        StudioButton { text: qsTr("Fit"); quiet: true; Layout.fillWidth: true; leftPadding: 4; rightPadding: 4; onClicked: canvas.resetView() }
                        StudioButton { glyph: "zoom-out"; quiet: true; Layout.fillWidth: true; leftPadding: 4; rightPadding: 4; onClicked: canvas.zoomAt(1/1.2, canvas.width/2, canvas.height/2) }
                    }
                }
            }
            Flow {
                visible: root.hasShapes
                Layout.fillWidth: true; Layout.leftMargin: 14; Layout.rightMargin: 14; Layout.topMargin: 12; spacing: 8
                Repeater {
                    model: annotationDocument.labels
                    Button {
                        id: labelButton; required property var modelData; required property int index
                        text: modelData.display + "   " + (modelData.hotkey || (index < 9 ? index + 1 : "")); implicitHeight: 28; topInset: 0; bottomInset: 0; leftPadding: 12; rightPadding: 12
                        contentItem: Text { text: labelButton.text; color: Theme.text; font.pixelSize: 13; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: Qt.rgba(Qt.color(labelButton.modelData.color).r, Qt.color(labelButton.modelData.color).g, Qt.color(labelButton.modelData.color).b, 0.16); radius: 3; border.width: root.activeLabel === labelButton.modelData.name ? 1 : 0; border.color: labelButton.modelData.color; Rectangle { width: 4; height: parent.height; color: labelButton.modelData.color } }
                        Shortcut { sequence: labelButton.modelData.hotkey || (labelButton.index < 9 ? String(labelButton.index + 1) : ""); enabled: root.visible && sequence.length > 0 && !root.textEditing && !backend.busy && !connection.visible; onActivated: {root.activeLabel=labelButton.modelData.name;root.tool="draw"} }
                        onClicked: { root.activeLabel = modelData.name; root.tool = "draw" }
                    }
                }
            }
            ScrollView {
                id: runtimeControls
                objectName: "runtimeConfigurationControls"
                visible: root.hasRuntimeLayout
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: visible ? 100 : 0
                Layout.leftMargin: 14; Layout.rightMargin: 14; Layout.topMargin: 12; Layout.bottomMargin: 12
                clip: true
                contentWidth: availableWidth
                ColumnLayout {
                    width: runtimeControls.availableWidth
                    spacing: 12
                    Repeater {
                        model: root.layoutNodes
                        ConfigNode {
                            required property var modelData
                            Layout.fillWidth: true
                            node: modelData
                            document: annotationDocument
                            taskData: backend.taskData
                            enabled: !backend.busy
                        }
                    }
                }
            }
            Item { visible: !root.hasRuntimeLayout; Layout.fillHeight: true }
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 54; color: Theme.surface
                Rectangle { width: parent.width; height: 1; color: Theme.border }
                RowLayout {
                    anchors.fill: parent; anchors.margins: 8; spacing: 6
                    ColumnLayout { spacing: 2; Label { text: "#" + backend.taskId; font.pixelSize: 12 } Label { text: backend.samplingQueue?qsTr("Visited %1").arg(backend.samplingPosition):qsTr("%1 of %2").arg(root.taskIndex < 0 ? 0 : (backend.page-1)*backend.dataManager.pageSize + root.taskIndex+1).arg(backend.totalTasks); font.pixelSize: 12 } }
                    StudioButton { Layout.preferredWidth: 28; text: "‹"; quiet: true; enabled: (backend.samplingQueue?backend.samplingPosition>1:((backend.page-1)*backend.dataManager.pageSize + root.taskIndex) > 0) && !backend.busy && !backend.demo; onClicked: root.moveTask(-1) }
                    StudioButton { Layout.preferredWidth: 28; text: "›"; quiet: true; enabled: (backend.samplingQueue || ((backend.page-1)*backend.dataManager.pageSize + root.taskIndex + 1) < backend.totalTasks) && !backend.busy && !backend.demo; onClicked: root.moveTask(1) }
                    Rectangle { width: 1; height: 24; color: Theme.border }
                    StudioButton { Layout.preferredWidth: 32; glyph: "undo"; quiet: true; enabled: annotationDocument.canUndo && !backend.busy; onClicked: annotationDocument.undo() }
                    StudioButton { Layout.preferredWidth: 32; glyph: "redo"; quiet: true; enabled: annotationDocument.canRedo && !backend.busy; onClicked: annotationDocument.redo() }
                    Label { visible: root.hasImages && root.width >= 1200; text: Math.round(canvas.zoom * 100) + "%"; color: Theme.muted; font.pixelSize: 12 }
                    Item { Layout.fillWidth: true }
                    Label { text: annotationDocument.dirty ? qsTr("● Unsaved") : qsTr("Saved"); font.pixelSize: 12; color: annotationDocument.dirty ? Theme.danger : Theme.muted }
                    StudioButton { objectName: "skipTask"; text: qsTr("Skip"); Layout.preferredWidth: 80; enabled: backend.taskId >= 0 && !backend.busy && !backend.demo; onClicked: root.guard(function() { backend.skipTask() }) }
                    CheckBox { text: root.width < 1250 ? qsTr("Next") : qsTr("Next after submit"); checked: root.submitAndContinue; onClicked: root.submitAndContinue = checked; font.pixelSize: 12 }
                    StudioButton { objectName: "submitAnnotation"; text: annotationDocument.annotationId < 0 ? qsTr("Submit") : qsTr("Update"); primary: true; Layout.preferredWidth: root.width < 1250 ? 105 : 150; Layout.preferredHeight: 40; enabled: !backend.busy && annotationDocument.editable && (!root.hasImages || canvas.hasImage) && !root.hasDraft; onClicked: root.submitAndContinue ? backend.saveAndNext() : backend.save() }
                }
            }
        }
        Rectangle { visible: root.width >= 1060; Layout.preferredWidth: 1; Layout.fillHeight: true; color: Theme.border }
        ColumnLayout {
            visible: root.width >= 1060
            Layout.preferredWidth: 320; Layout.minimumWidth: 320; Layout.maximumWidth: 320; Layout.fillHeight: true; spacing: 0
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 34; color: Theme.subtle
                RowLayout { anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; Label { text: qsTr("Info"); font.pixelSize: 14; font.weight: Font.DemiBold; Layout.fillWidth: true } Label { text: qsTr("Annotations"); color: Theme.muted; font.pixelSize: 14 } }
            }
            Item {
                Layout.fillWidth: true; Layout.preferredHeight: root.height * 0.45
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 24; spacing: 14
                    ComboBox {
                        objectName: "annotationSelector"
                        Layout.fillWidth: true; model: { languageManager.code; return backend.annotations }
                        textRole: "title"; valueRole: "id"; font.pixelSize: 12
                        enabled: backend.taskId >= 0 && !backend.busy
                        currentIndex: { const rows = model; for (let i=0;i<rows.length;++i) if(rows[i].id===annotationDocument.annotationId) return i; return -1 }
                        onActivated: { const id = currentValue; root.guard(function() { backend.selectAnnotation(id) }) }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        StudioButton { text: backend.groundTruth ? qsTr("★ Ground truth") : qsTr("☆ Ground truth"); objectName: "groundTruth"; quiet: !backend.groundTruth; primary: backend.groundTruth; enabled: !backend.busy && !backend.demo && annotationDocument.annotationId >= 0; onClicked: root.guard(function() { backend.setGroundTruth(!backend.groundTruth) }) }
                        StudioButton { text: qsTr("Delete"); objectName: "deleteAnnotation"; destructive: true; enabled: !backend.busy && !backend.demo && annotationDocument.annotationId >= 0; onClicked: deleteAnnotationConfirm.open() }
                    }
                    Item { Layout.fillHeight: true; visible: root.hasShapes && canvas.selectedIndex < 0 }
                    Image { source: "qrc:/assets/icons/cursor" + (Theme.dark ? "-dark" : "") + ".svg"; Layout.preferredWidth: 36; Layout.preferredHeight: 36; Layout.alignment: Qt.AlignHCenter; visible: root.hasShapes && canvas.selectedIndex < 0 }
                    Label { visible: root.hasShapes; text: canvas.selectedIndex < 0 ? qsTr("View region details") : qsTr("Region properties"); font.pixelSize: 16; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter }
                    Label { visible: root.hasShapes && canvas.selectedIndex < 0; Layout.fillWidth: true; text: qsTr("Select a region to view its properties, metadata and available actions"); color: Theme.muted; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; font.pixelSize: 14; lineHeight: 1.5 }
                    ColumnLayout {
                        visible: root.hasShapes && canvas.selectedIndex >= 0; Layout.fillWidth: true; spacing: 16
                        Label { text: { const r = root.selectedRow(); return r ? "X  " + r.x.toFixed(1) + "%    Y  " + r.y.toFixed(1) + "%\nW  " + r.width.toFixed(1) + "%    H  " + r.height.toFixed(1) + "%" : "" } color: Theme.muted; font.pixelSize: 14; lineHeight: 1.8 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: {
                                const row = root.selectedRow()
                                if (row) {
                                    for (const control of annotationDocument.controls)
                                        if (control.name === row.fromName) return control.labels
                                }
                                return annotationDocument.labels
                            }
                            textRole: "display"; valueRole: "name"
                            displayText: root.selectedRow() ? root.selectedRow().displayLabel : qsTr("Change label")
                            enabled: !backend.busy
                            onActivated: annotationDocument.setLabel(canvas.selectedIndex, currentValue)
                        }
                        StudioButton { text: qsTr("Delete selected region"); destructive: true; Layout.fillWidth: true; enabled: !backend.busy; onClicked: annotationDocument.removeRectangle(canvas.selectedIndex) }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
            Rectangle { visible: root.hasShapes; Layout.fillWidth: true; Layout.preferredHeight: 34; color: Theme.subtle; Label { text: qsTr("Regions     %1").arg(annotationDocument.rectangles.length); x: 16; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 14; font.weight: Font.DemiBold } }
            ListView {
                id: regions; visible: root.hasShapes; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: annotationDocument.rectangles
                ScrollBar.vertical: ScrollBar {}
                delegate: ItemDelegate {
                    required property var modelData; required property int index
                    width: ListView.view.width; height: 48; enabled: !backend.busy; highlighted: canvas.selectedIndex === index
                    contentItem: RowLayout { spacing: 12; Rectangle { width: 8; height: 8; radius: 2; color: modelData.color } Label { text: (index+1) + "  " + modelData.displayLabel; Layout.fillWidth: true; font.pixelSize: 14 } Label { text: modelData.shapeType === "polygonlabels" ? qsTr("Polygon") : qsTr("Rectangle"); color: Theme.muted; font.pixelSize: 12 } }
                    onClicked: { canvas.selectedIndex = index; root.tool = "select" }
                }
                ColumnLayout {
                    anchors.centerIn: parent; width: parent.width - 40; spacing: 12; visible: regions.count === 0
                    Image { source: "qrc:/assets/icons/rectangle-tool" + (Theme.dark ? "-dark" : "") + ".svg"; Layout.preferredWidth: 36; Layout.preferredHeight: 36; Layout.alignment: Qt.AlignHCenter }
                    Label { text: qsTr("Labeled regions will appear here"); Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; font.weight: Font.DemiBold; wrapMode: Text.WordWrap }
                    Label { text: qsTr("Start labeling and track your results using this panel"); Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; color: Theme.muted; lineHeight: 1.5 }
                }
            }
        }
    }
    Dialog {
        id: sourceDialog; objectName: "taskSourceDialog"; parent: Overlay.overlay; anchors.centerIn: parent
        modal: true; title: qsTr("Task source"); standardButtons: Dialog.Close
        width: Math.min(980, root.Window.window ? root.Window.window.width - 60 : 980)
        height: Math.min(680, root.Window.window ? root.Window.window.height - 90 : 680)
        ColumnLayout {
            anchors.fill: parent; spacing: 8
            RowLayout {
                Layout.fillWidth: true
                ComboBox { Layout.fillWidth: true; model: [qsTr("Full task"), qsTr("Task data"), qsTr("Annotations"), qsTr("Predictions")]; currentIndex: root.sourceSection; onActivated: root.sourceSection = currentIndex }
                StudioButton { text: qsTr("Select all"); onClicked: { sourceJson.forceActiveFocus(); sourceJson.selectAll() } }
                StudioButton { objectName: "copyTaskSource"; text: qsTr("Copy JSON"); onClicked: { sourceJson.selectAll(); sourceJson.copy() } }
            }
            ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                StudioArea { id: sourceJson; objectName: "taskSourceJson"; text: JSON.stringify(root.sourceObject(), null, 2); readOnly: true; selectByMouse: true; wrapMode: TextEdit.NoWrap; font.family: "Consolas"; font.pixelSize: 13 }
            }
        }
    }
    Dialog {
        id: detailsDialog; objectName: "taskDetailsDialog"; parent: Overlay.overlay; anchors.centerIn: parent
        modal: true; title: qsTr("Task details"); standardButtons: Dialog.Close
        width: Math.min(700, root.Window.window ? root.Window.window.width - 60 : 700)
        height: Math.min(620, root.Window.window ? root.Window.window.height - 90 : 620)
        ScrollView {
            id: detailsScroll
            anchors.fill: parent; contentWidth: availableWidth; clip: true
            ColumnLayout {
                width: detailsScroll.availableWidth; spacing: 12
                Repeater {
                    model: root.taskMetadata()
                    RowLayout {
                        required property var modelData
                        Layout.fillWidth: true; spacing: 20
                        Label { text: modelData.title; Layout.preferredWidth: 165; color: Theme.muted; wrapMode: Text.WordWrap }
                        Label { text: modelData.value; Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; textFormat: Text.PlainText }
                    }
                }
                StudioButton { text: qsTr("Open task source"); onClicked: { detailsDialog.close(); sourceDialog.open() } }
            }
        }
    }
    Dialog {
        id: deleteAnnotationConfirm; objectName: "deleteAnnotationConfirm"; parent: Overlay.overlay; anchors.centerIn: parent
        modal: true; width: 420; title: qsTr("Delete annotation?"); standardButtons: Dialog.Yes | Dialog.Cancel
        Label { width: parent.width; text: qsTr("Delete annotation #%1 and its saved regions? This cannot be undone.").arg(annotationDocument.annotationId); wrapMode: Text.WordWrap }
        onAccepted: root.guard(function() { backend.deleteAnnotation() })
    }
    Dialog {
        id: finishedDialog; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 420
        title: qsTr("End of task list"); standardButtons: Dialog.Ok
        Label { width: parent.width; text: qsTr("There are no more tasks in the current queue or filtered list. Return to the project to change filters or import data."); wrapMode: Text.WordWrap }
    }
    ConnectionDialog { id: connection }
    Dialog {
        id: discardDialog; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 420; title: qsTr("Unsaved changes")
        standardButtons: Dialog.Discard | Dialog.Cancel
        Label { width: parent.width; text: qsTr("Discard the unsaved annotation and continue?"); wrapMode: Text.WordWrap }
        onDiscarded: { root.cancelDraft(); backend.discardChanges(); const action = root.pendingAction; root.pendingAction = null; if(action) action() }
        onRejected: root.pendingAction = null
    }
}
