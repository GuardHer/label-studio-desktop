import QtQuick
import QtQuick.Controls

ComboBox {
    id: root
    implicitWidth: 120
    implicitHeight: 34
    font.pixelSize: 13
    model: [{name: "简体中文", code: "zh_CN"}, {name: "English", code: "en"}]
    textRole: "name"
    valueRole: "code"
    currentIndex: languageManager.code === "zh_CN" ? 0 : 1
    onActivated: languageManager.setLanguage(currentValue)
    Accessible.name: qsTr("Interface language")
    ToolTip.visible: hovered
    ToolTip.text: qsTr("Interface language")
}
