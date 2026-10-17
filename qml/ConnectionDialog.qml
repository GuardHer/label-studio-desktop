import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    parent: Overlay.overlay
    objectName: "connectionDialog"
    title: qsTr("Connect to Label Studio")
    modal: true
    width: 470
    anchors.centerIn: parent
    standardButtons: Dialog.Ok | Dialog.Cancel
    onOpened: {
        address.text = initialServer; token.text = ""; token.forceActiveFocus()
        standardButton(Dialog.Ok).text = Qt.binding(function() { return qsTr("Connect") })
        standardButton(Dialog.Cancel).text = Qt.binding(function() { return qsTr("Cancel") })
    }
    onClosed: token.clear()
    onAccepted: backend.connectTo(address.text, token.text, auth.currentIndex)
    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        Label { text: qsTr("Server address"); color: Theme.muted }
        TextField { id: address; Layout.fillWidth: true; placeholderText: "http://127.0.0.1:8080"; selectByMouse: true }
        Label { text: qsTr("Authentication method"); color: Theme.muted }
        ComboBox {
            id: auth; objectName: "authMode"; Layout.fillWidth: true
            // Stable model identities keep the authentication method unchanged on retranslation.
            model: 3
            readonly property var captions: [qsTr("Legacy API Token"), qsTr("Personal access token / JWT Refresh"), qsTr("JWT Access Token")]
            displayText: captions[currentIndex] || ""
            delegate: ItemDelegate {
                required property int index
                width: auth.width
                text: auth.captions[index]
                highlighted: auth.highlightedIndex === index
            }
        }
        TextField { id: token; objectName: "serverToken"; Layout.fillWidth: true; placeholderText: qsTr("Paste your Label Studio API Token"); echoMode: TextInput.Password; selectByMouse: true }
        Label { Layout.fillWidth: true; text: qsTr("Get a token from your Label Studio account settings. Credentials are used for this session only."); wrapMode: Text.WordWrap; color: Theme.muted; font.pixelSize: 12 }
    }
}
