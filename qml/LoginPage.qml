import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    objectName: "loginPage"
    property bool registering: false
    signal tokenRequested()
    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.preferredWidth: parent.width * 0.46; Layout.fillHeight: true
            color: Theme.subtle
            Image { source: Theme.dark ? "qrc:/assets/logo-dark.svg" : "qrc:/assets/logo.svg"; x: 48; y: 40; width: 194; height: 30 }
            ColumnLayout {
                anchors.centerIn: parent; width: parent.width - 112; spacing: 24
                Label { Layout.fillWidth: true; text: qsTr("A full-fledged open source solution for data labeling"); font.pixelSize: 38; font.weight: Font.DemiBold; wrapMode: Text.WordWrap; lineHeight: 1.15 }
                Label { text: qsTr("Did you know?"); font.pixelSize: 18; font.weight: Font.DemiBold; Layout.topMargin: 28 }
                Label { Layout.fillWidth: true; text: qsTr("There's an Enterprise version of Label Studio packed with more features and automation to label data faster while ensuring the highest quality."); font.pixelSize: 16; color: Theme.muted; wrapMode: Text.WordWrap; lineHeight: 1.4 }
                Label { text: qsTr("Learn more  ↗"); color: Theme.accent; font.pixelSize: 16; MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Qt.openUrlExternally("https://humansignal.com/") } }
                Image { source: "qrc:/assets/opossum_looking.png"; Layout.preferredWidth: 130; Layout.preferredHeight: 100; fillMode: Image.PreserveAspectFit; Layout.topMargin: 10 }
            }
            Label { x: 48; anchors.bottom: parent.bottom; anchors.bottomMargin: 32; text: qsTr("Brought to you by   HumanSignal"); color: Theme.muted; font.pixelSize: 14 }
        }
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            LanguageSelector { objectName: "loginLanguageSelector"; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 24 }
            ColumnLayout {
                anchors.centerIn: parent; width: Math.min(400, parent.width - 96); spacing: 12
                Label { text: root.registering ? qsTr("Create an account") : qsTr("Log in"); font.pixelSize: 32; font.weight: Font.DemiBold; Layout.bottomMargin: 20 }
                Label { text: qsTr("Server"); color: Theme.muted; font.pixelSize: 14 }
                StudioField { id: server; objectName: "loginServer"; Layout.fillWidth: true; text: initialServer; placeholderText: "http://localhost:8080"; Layout.bottomMargin: 8 }
                Label { text: qsTr("Email Address"); font.pixelSize: 14 }
                StudioField { id: email; objectName: "loginEmail"; Layout.fillWidth: true; inputMethodHints: Qt.ImhEmailCharactersOnly; onAccepted: password.forceActiveFocus() }
                Label { text: qsTr("Password"); font.pixelSize: 14; Layout.topMargin: 8 }
                StudioField { id: password; objectName: "loginPassword"; Layout.fillWidth: true; echoMode: TextInput.Password; onAccepted: {if(submit.enabled)submit.clicked()} }
                Label { visible: root.registering; text: qsTr("Confirm password") }
                StudioField { id: passwordConfirm; visible: root.registering; Layout.fillWidth: true; echoMode: TextInput.Password }
                Label { visible: root.registering; text: qsTr("Invitation link or token (if required)") }
                StudioField { id: invitation; visible: root.registering; Layout.fillWidth: true }
                CheckBox { id: newsletters; visible: root.registering; text: qsTr("Receive newsletters and product updates") }
                Label { Layout.fillWidth: true; text: backend.message; visible: backend.messageIsError; color: Theme.danger; wrapMode: Text.WordWrap; font.pixelSize: 13 }
                StudioButton {
                    id: submit; objectName: "loginSubmit"; text: backend.busy ? qsTr("Please wait…") : root.registering ? qsTr("Create account") : qsTr("Log in"); primary: true
                    Layout.fillWidth: true; Layout.preferredHeight: 42; Layout.topMargin: 16
                    enabled: !backend.busy && email.text.trim().length > 0 && password.text.length > 0 && (!root.registering || password.text === passwordConfirm.text)
                    onClicked: { if(!enabled)return; if(root.registering) backend.signup(server.text,email.text,password.text,invitation.text,newsletters.checked); else backend.login(server.text,email.text,password.text); password.clear();passwordConfirm.clear() }
                }
                StudioButton { text: root.registering ? qsTr("Already have an account? Log in") : qsTr("Create an account"); quiet: true; Layout.alignment: Qt.AlignHCenter; enabled: !backend.busy; onClicked: {root.registering=!root.registering;password.clear();passwordConfirm.clear();backend.clearMessage()} }
                StudioButton { text: qsTr("Use an API token instead"); quiet: true; Layout.alignment: Qt.AlignHCenter; onClicked: root.tokenRequested() }
                Label { text: qsTr("Credentials are used for this session only."); color: Theme.muted; font.pixelSize: 12; Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 8 }
            }
        }
    }
}
