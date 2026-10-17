import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

T.TextField {
    id: control
    implicitHeight: 40
    font.pixelSize: 16
    color: Theme.text
    placeholderTextColor: Theme.muted
    selectByMouse: true
    leftPadding: 14; rightPadding: 14
    topPadding: 8; bottomPadding: 8
    selectionColor: Theme.accent; selectedTextColor: "white"
    verticalAlignment: TextInput.AlignVCenter
    Text {
        x: control.leftPadding; width: control.width - control.leftPadding - control.rightPadding
        anchors.verticalCenter: parent.verticalCenter
        text: control.placeholderText; font: control.font; color: Theme.muted
        visible: control.text.length === 0; elide: Text.ElideRight
    }
    background: Rectangle {
        radius: 4; color: Theme.surface
        border.color: control.activeFocus ? Theme.accent : Theme.border
        border.width: control.activeFocus ? 2 : 1
    }
}
