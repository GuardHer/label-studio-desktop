import QtQuick
import QtQuick.Controls

Item {
    id: root
    objectName: "annotationCanvas"
    required property var document
    property bool shortcutsEnabled: true
    property url imageSource
    property string tool: "draw"
    property string activeLabel: ""
    property real inset: 40
    property color backgroundColor: Theme.canvas
    property int selectedIndex: -1
    property real zoom: 1
    property real panX: 0
    property real panY: 0
    readonly property bool hasImage: image.status === Image.Ready
    readonly property size imageSize: image.sourceSize
    property string operation: ""
    property bool dragging: false
    property real startX: 0
    property real startY: 0
    property real draftX: 0
    property real draftY: 0
    property real draftW: 0
    property real draftH: 0
    property real anchorX: 0
    property real anchorY: 0
    property real pressX: 0
    property real pressY: 0
    property real initialPanX: 0
    property real initialPanY: 0
    property var original: ({})
    readonly property bool polygonMode: document.shapeType === "polygonlabels"
    property var polygonDraft: []
    property var polygonPreview: []
    property string polygonLabel: ""
    property int vertexIndex: -1
    function cancelDraft() { polygonDraft = []; polygonPreview = []; polygonOverlay.requestPaint() }
    function finishPolygon() {
        if (polygonDraft.length < 3) return
        const count = document.rectangles.length
        document.addPolygon(polygonDraft, polygonLabel)
        if (document.rectangles.length > count) { cancelDraft(); selectedIndex = count }
    }
    Shortcut { sequence: "Return"; enabled: root.visible && root.shortcutsEnabled && root.polygonDraft.length >= 3; onActivated: root.finishPolygon() }
    Shortcut { sequence: "Enter"; enabled: root.visible && root.shortcutsEnabled && root.polygonDraft.length >= 3; onActivated: root.finishPolygon() }
    Shortcut { sequence: "Escape"; enabled: root.visible && root.shortcutsEnabled && root.polygonDraft.length > 0; onActivated: root.cancelDraft() }
    onPolygonModeChanged: { cancelDraft(); dragging = false }
    onPolygonDraftChanged: polygonOverlay.requestPaint()
    onPolygonPreviewChanged: polygonOverlay.requestPaint()
    onSelectedIndexChanged: polygonOverlay.requestPaint()

    function resetView() { zoom = 1; panX = 0; panY = 0 }
    function clamp(n, lo, hi) { return Math.max(lo, Math.min(hi, n)) }
    function zoomAt(factor, px, py) {
        if (!hasImage) return
        const rx = (px - plane.x) / plane.width
        const ry = (py - plane.y) / plane.height
        zoom = clamp(zoom * factor, 0.2, 8)
        panX = px - rx * plane.width - (viewport.width - plane.width) / 2
        panY = py - ry * plane.height - (viewport.height - plane.height) / 2
    }
    function hit(x, y) {
        const rows = document.rectangles
        for (let i = rows.length - 1; i >= 0; --i) {
            const r = rows[i]
            if (r.shapeType === "polygonlabels") {
                const points = r.points
                let inside = false
                for (let a=0,b=points.length-1;a<points.length;b=a++) {
                    const p=points[a],q=points[b]
                    if (((p[1]>y)!==(q[1]>y)) && x<(q[0]-p[0])*(y-p[1])/(q[1]-p[1])+p[0]) inside=!inside
                }
                if (inside) return i
                continue
            }
            if (x >= r.x && x <= r.x + r.width && y >= r.y && y <= r.y + r.height) return i
        }
        return -1
    }
    onImageSourceChanged: { cancelDraft(); resetView(); selectedIndex = -1; dragging = false }
    Connections {
        target: root.document
        function onChanged() { if (root.selectedIndex >= root.document.rectangles.length) root.selectedIndex = -1; polygonOverlay.requestPaint() }
    }
    Rectangle {
        id: viewport
        anchors.fill: parent
        color: root.backgroundColor
        clip: true
        Item {
            id: plane
            objectName: "imagePlane"
            readonly property real fit: root.hasImage ? Math.min(Math.max(64, viewport.width - 2*root.inset) / image.sourceSize.width, Math.max(64, viewport.height - 2*root.inset) / image.sourceSize.height) : 1
            width: image.sourceSize.width * fit * root.zoom
            height: image.sourceSize.height * fit * root.zoom
            x: (viewport.width - width) / 2 + root.panX
            y: (viewport.height - height) / 2 + root.panY
            Image {
                id: image
                anchors.fill: parent
                source: root.imageSource
                cache: false
                smooth: true
            }
            Repeater {
                model: root.document.rectangles
                Rectangle {
                    id: region
                    required property var modelData
                    required property int index
                    visible: modelData.shapeType === "rectanglelabels"
                    readonly property bool selected: root.selectedIndex === index
                    readonly property bool preview: selected && root.dragging && root.operation !== "draw" && root.operation !== "pan"
                    x: (preview ? root.draftX : modelData.x) / 100 * plane.width
                    y: (preview ? root.draftY : modelData.y) / 100 * plane.height
                    width: (preview ? root.draftW : modelData.width) / 100 * plane.width
                    height: (preview ? root.draftH : modelData.height) / 100 * plane.height
                    color: Qt.rgba(border.color.r, border.color.g, border.color.b, selected ? 0.17 : 0.06)
                    border.color: modelData.color || Theme.accent
                    border.width: selected ? 3 : 2
                    Rectangle {
                        y: -height
                        width: badge.implicitWidth + 16
                        height: 25
                        color: region.border.color
                        Text { id: badge; anchors.centerIn: parent; text: (region.index + 1) + " · " + region.modelData.displayLabel; color: "#15161e"; font.pixelSize: 11; font.bold: true }
                    }
                    Repeater {
                        model: region.selected ? 4 : 0
                        Rectangle {
                            required property int index
                            x: (index % 2 ? region.width : 0) - 4
                            y: (index >= 2 ? region.height : 0) - 4
                            width: 8; height: 8
                            color: "white"; border.color: region.border.color; border.width: 2
                        }
                    }
                }
            }
            Canvas {
                id: polygonOverlay
                anchors.fill: parent
                visible: true
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                onPaint: {
                    const ctx=getContext("2d"); ctx.reset(); ctx.clearRect(0,0,width,height)
                    function path(points,color,selected,closed) {
                        if (!points.length) return
                        ctx.beginPath(); ctx.moveTo(points[0][0]*width/100,points[0][1]*height/100)
                        for (let i=1;i<points.length;i++) ctx.lineTo(points[i][0]*width/100,points[i][1]*height/100)
                        if (closed) { ctx.closePath(); ctx.fillStyle=Qt.rgba(Qt.color(color).r,Qt.color(color).g,Qt.color(color).b,selected?0.2:0.08); ctx.fill() }
                        ctx.strokeStyle=color; ctx.lineWidth=selected?3:2; ctx.stroke()
                        if (selected) for (const p of points) {
                            ctx.beginPath(); ctx.arc(p[0]*width/100,p[1]*height/100,4,0,Math.PI*2)
                            ctx.fillStyle="white"; ctx.fill(); ctx.strokeStyle=color; ctx.lineWidth=2; ctx.stroke()
                        }
                    }
                    const rows=root.document.rectangles
                    for (let i=0;i<rows.length;i++) {
                        if (rows[i].shapeType !== "polygonlabels") continue
                        const preview=root.dragging && root.selectedIndex===i && root.polygonPreview.length>0
                        path(preview?root.polygonPreview:rows[i].points,rows[i].color,root.selectedIndex===i,true)
                    }
                    path(root.polygonDraft,Theme.accent,true,false)
                }
            }
            Rectangle {
                visible: root.dragging && root.operation === "draw"
                x: root.draftX / 100 * plane.width; y: root.draftY / 100 * plane.height
                width: root.draftW / 100 * plane.width; height: root.draftH / 100 * plane.height
                color: "#226c63ff"; border.color: Theme.accent; border.width: 2
            }
        }
        MouseArea {
            id: pointer
            anchors.fill: parent
            objectName: "canvasPointer"
            enabled: root.hasImage && root.enabled
            acceptedButtons: Qt.LeftButton | Qt.MiddleButton
            cursorShape: root.tool === "draw" ? Qt.CrossCursor : root.tool === "pan" ? Qt.OpenHandCursor : Qt.ArrowCursor
            onWheel: wheel => { root.zoomAt(wheel.angleDelta.y > 0 ? 1.15 : 1 / 1.15, wheel.x, wheel.y); wheel.accepted = true }
            onPressed: mouse => {
                root.pressX = mouse.x; root.pressY = mouse.y
                root.initialPanX = root.panX; root.initialPanY = root.panY
                root.operation = ""; root.dragging = false
                if (root.tool === "pan" || mouse.button === Qt.MiddleButton) { root.operation = "pan"; root.dragging = true; return }
                if (!root.document.editable) return
                const x = (mouse.x - plane.x) / plane.width * 100
                const y = (mouse.y - plane.y) / plane.height * 100
                root.startX = root.clamp(x, 0, 100); root.startY = root.clamp(y, 0, 100)
                if (root.polygonMode && root.tool === "draw") {
                    if (x<0 || x>100 || y<0 || y>100 || !root.activeLabel) return
                    if (root.polygonDraft.length>=3) {
                        const first=root.polygonDraft[0]
                        if (Math.hypot((x-first[0])*plane.width/100,(y-first[1])*plane.height/100)<12) { root.finishPolygon(); return }
                    }
                    if (!root.polygonDraft.length) root.polygonLabel=root.activeLabel
                    root.polygonDraft=root.polygonDraft.concat([[x,y]])
                    return
                }
                if (root.tool !== "draw") {
                    root.vertexIndex=-1
                    const selected=root.selectedIndex>=0?root.document.rectangles[root.selectedIndex]:null
                    if (selected && selected.shapeType === "polygonlabels") {
                        const points=selected.points
                        for (let i=0;i<points.length;i++) if (Math.hypot((x-points[i][0])*plane.width/100,(y-points[i][1])*plane.height/100)<10) { root.vertexIndex=i; break }
                    }
                    const candidate=root.vertexIndex>=0?root.selectedIndex:root.hit(x,y)
                    const row=candidate>=0?root.document.rectangles[candidate]:null
                    if (row && row.shapeType === "polygonlabels") {
                        root.selectedIndex=candidate
                        root.original=row
                        root.polygonPreview=row.points.map(p=>[p[0],p[1]])
                        root.operation=root.vertexIndex>=0?"vertex":"polygonMove"
                        root.dragging=true; return
                    }
                }
                if (root.tool === "draw") {
                    if (x < 0 || x > 100 || y < 0 || y > 100 || !root.activeLabel) return
                    root.selectedIndex = -1; root.operation = "draw"
                    root.draftX = x; root.draftY = y; root.draftW = 0; root.draftH = 0
                } else {
                    if (root.selectedIndex >= 0 && root.document.rectangles[root.selectedIndex].shapeType === "rectanglelabels") {
                        const r = root.document.rectangles[root.selectedIndex]
                        const corners = [[r.x, r.y], [r.x + r.width, r.y], [r.x, r.y + r.height], [r.x + r.width, r.y + r.height]]
                        for (let i = 0; i < 4; ++i) {
                            if (Math.abs(x - corners[i][0]) * plane.width / 100 < 10 && Math.abs(y - corners[i][1]) * plane.height / 100 < 10) {
                                root.operation = "resize"; root.anchorX = corners[3 - i][0]; root.anchorY = corners[3 - i][1]
                                break
                            }
                        }
                    }
                    if (root.operation !== "resize") { root.selectedIndex = root.hit(x, y); root.operation = "move" }
                    if (root.selectedIndex < 0) return
                    const row = root.document.rectangles[root.selectedIndex]
                    root.original = { x: row.x, y: row.y, width: row.width, height: row.height }
                    root.draftX = row.x; root.draftY = row.y; root.draftW = row.width; root.draftH = row.height
                }
                root.dragging = true
            }
            onPositionChanged: mouse => {
                if (!root.dragging) return
                if (root.operation === "pan") { root.panX = root.initialPanX + mouse.x - root.pressX; root.panY = root.initialPanY + mouse.y - root.pressY; return }
                const x = root.clamp((mouse.x - plane.x) / plane.width * 100, 0, 100)
                const y = root.clamp((mouse.y - plane.y) / plane.height * 100, 0, 100)
                if (root.operation === "vertex") {
                    const points=root.polygonPreview.slice(); points[root.vertexIndex]=[x,y]; root.polygonPreview=points; return
                }
                if (root.operation === "polygonMove") {
                    const dx=root.clamp(x-root.startX,-root.original.x,100-root.original.x-root.original.width)
                    const dy=root.clamp(y-root.startY,-root.original.y,100-root.original.y-root.original.height)
                    root.polygonPreview=root.original.points.map(p=>[p[0]+dx,p[1]+dy]); return
                }
                if (root.operation === "move") {
                    root.draftX = root.clamp(root.original.x + x - root.startX, 0, 100 - root.original.width)
                    root.draftY = root.clamp(root.original.y + y - root.startY, 0, 100 - root.original.height)
                } else {
                    const ax = root.operation === "resize" ? root.anchorX : root.startX
                    const ay = root.operation === "resize" ? root.anchorY : root.startY
                    root.draftX = Math.min(ax, x); root.draftY = Math.min(ay, y)
                    root.draftW = Math.abs(ax - x); root.draftH = Math.abs(ay - y)
                }
            }
            onReleased: {
                if (!root.dragging) return
                if (root.operation === "vertex" || root.operation === "polygonMove") {
                    root.document.updatePolygon(root.selectedIndex,root.polygonPreview); root.polygonPreview=[]
                } else if (root.operation === "draw") {
                    root.document.addRectangle(root.draftX, root.draftY, root.draftX + root.draftW, root.draftY + root.draftH, root.activeLabel)
                    root.selectedIndex = root.document.rectangles.length - 1
                } else if (root.operation === "move" || root.operation === "resize") {
                    root.document.updateRectangle(root.selectedIndex, root.draftX, root.draftY, root.draftW, root.draftH)
                }
                root.dragging = false; polygonOverlay.requestPaint()
            }
            onCanceled: { root.dragging = false; root.polygonPreview=[]; polygonOverlay.requestPaint() }
        }
        Column {
            anchors.centerIn: parent
            spacing: 12
            visible: !root.hasImage
            Label { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Open a task to start labeling"); font.pixelSize: 24; font.bold: true; color: Theme.text }
            Label { text: qsTr("Choose a task from the project or open the example"); color: Theme.muted }
        }
    }
}
