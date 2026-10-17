import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Button {
    id: control
    property bool primary: false
    property bool destructive: false
    property bool quiet: false
    property string glyph: ""
    property bool alignLeft: false
    property bool elide: false
    font.pixelSize: 14
    font.weight: Font.Medium
    implicitHeight: 36
    implicitWidth: Math.max(36, implicitContentWidth + leftPadding + rightPadding)
    leftPadding: 14; rightPadding: 14
    topPadding: 6; bottomPadding: 6
    topInset: 0; bottomInset: 0
    hoverEnabled: true
    Material.elevation: 0
    contentItem: Item {
        implicitWidth: caption.implicitWidth + (control.glyph.length ? 26 : 0)
        implicitHeight: Math.max(20, caption.implicitHeight)
        Row {
            x: control.alignLeft ? 0 : (parent.width-width)/2; anchors.verticalCenter: parent.verticalCenter; spacing: 6
            Image { source: control.glyph.length ? "qrc:/assets/icons/" + control.glyph + (Theme.dark ? "-dark" : "") + ".svg" : ""; width: visible ? 20 : 0; height: 20; visible: control.glyph.length > 0; anchors.verticalCenter: parent.verticalCenter }
            Text {
                id: caption; text: control.text; font: control.font
                width: control.elide ? Math.max(0, control.availableWidth - (control.glyph.length ? 26 : 0)) : implicitWidth
                elide: control.elide ? Text.ElideRight : Text.ElideNone
                color: !control.enabled ? Theme.muted : control.primary ? (Theme.dark ? "#1d2545" : "#f0f3fe") : control.destructive ? Theme.danger : Theme.text
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }
        }
    }
    background: Rectangle {
        radius: 4
        color: control.primary ? Theme.accent : control.down ? Theme.selected : control.hovered ? Theme.subtle : control.quiet ? "transparent" : Theme.surface
        border.width: control.quiet && !control.hovered ? 0 : 1
        border.color: control.activeFocus || control.primary ? Theme.accent : control.destructive ? Theme.danger : Theme.border
        opacity: control.enabled ? 1 : 0.5
    }
}
