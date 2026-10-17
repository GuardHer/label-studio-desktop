import QtQuick
import QtQuick.Templates as T

T.TextArea {
    id: control
    implicitWidth: 300; implicitHeight: 120
    color: Theme.text; font.pixelSize: 16
    selectByMouse: true; wrapMode: TextEdit.WordWrap
    padding: 14
    selectionColor: Theme.accent; selectedTextColor: "white"
    background: Rectangle { radius: 4; color: Theme.surface; border.color: control.activeFocus ? Theme.accent : Theme.border }
    Text {
        x: control.leftPadding; y: control.topPadding
        width: control.width - control.leftPadding - control.rightPadding
        text: control.placeholderText; font: control.font; color: Theme.muted
        visible: control.text.length === 0; wrapMode: Text.WordWrap
    }
}
