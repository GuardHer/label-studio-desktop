import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root
    objectName: "accountPage"
    readonly property var account: backend.account
    property int section: 0
    property bool profileInitialized: false
    property bool orgInitialized: false
    property bool policyInitialized: false
    property string profileBaseline: ""
    property string orgBaseline: ""
    property string keysBaseline: ""
    property bool emailBaseline: false
    property bool personalBaseline: false
    property bool legacyBaseline: false
    property var pendingAction: null
    property var confirmAction: null
    property string confirmText: ""
    property int selectedMember: -1
    readonly property bool profileDirty: profileInitialized && JSON.stringify([first.text,last.text,phone.text]) !== profileBaseline
    readonly property bool orgDirty: orgInitialized && JSON.stringify([orgTitle.text,orgContact.text]) !== orgBaseline
    readonly property bool policyDirty: policyInitialized && (personal.checked !== personalBaseline || legacy.checked !== legacyBaseline)
    readonly property bool dirty: profileDirty || orgDirty || policyDirty || (profileInitialized && newsletters.checked !== emailBaseline) || keys.text !== keysBaseline || oldPassword.text.length > 0 || newPassword.text.length > 0 || confirmPassword.text.length > 0
    function fillProfile() {
        first.text = account.profile.first_name || ""; last.text = account.profile.last_name || ""; phone.text = account.profile.phone || ""
        newsletters.checked = !!account.profile.allow_newsletters; emailBaseline = newsletters.checked
        profileBaseline = JSON.stringify([first.text,last.text,phone.text]); profileInitialized = true
    }
    function fillOrganization() {
        orgTitle.text = account.organization.title || ""; orgContact.text = account.organization.contact_info || ""
        orgBaseline = JSON.stringify([orgTitle.text,orgContact.text]); orgInitialized = true
    }
    function fillPolicy() {
        personal.checked = !!account.tokenSettings.api_tokens_enabled; legacy.checked = !!account.tokenSettings.legacy_api_tokens_enabled
        personalBaseline = personal.checked; legacyBaseline = legacy.checked; policyInitialized = true
    }
    function discard() {
        if (profileInitialized) fillProfile()
        if (orgInitialized) fillOrganization()
        if (policyInitialized) fillPolicy()
        keys.text = keysBaseline; oldPassword.clear(); newPassword.clear(); confirmPassword.clear()
    }
    function open(index) {
        section = index; profileInitialized = false; orgInitialized = false; policyInitialized = false
        keys.text = ""; keysBaseline = ""; secretInput.clear(); oldPassword.clear(); newPassword.clear(); confirmPassword.clear()
        account.load()
    }
    function guard(action) {
        if (backend.busy) return
        if (dirty) { pendingAction = action; unsaved.open() } else action()
    }
    function confirm(text,action) { confirmText=text;confirmAction=action; confirmation.open() }
    function showSection(index) { guard(function() { section=index; account.clearSecret(); secretInput.clear(); if(index===5) account.loadMember(account.profile.id) }) }
    function date(value) { return value ? Qt.formatDateTime(new Date(value),Qt.DefaultLocaleShortDate) : "—" }
    function projectNames(items) { return (items || []).map(function(p) { return p.title + " (#" + p.id + ")" }).join(", ") || "—" }
    function hotkeyValue(action,fallback) {
        try { const value=JSON.parse(keys.text).custom_hotkeys[action];return value || {key:fallback,active:true} } catch(e) { return {key:fallback,active:true} }
    }
    function editHotkey(action,key,active) {
        try { const value=JSON.parse(keys.text);const entry=value.custom_hotkeys[action] || {};entry.key=key;entry.active=active;value.custom_hotkeys[action]=entry;keys.text=JSON.stringify(value,null,2) } catch(e) { }
    }
    Connections {
        target: root.account
        function onChanged() {
            if (!root.profileInitialized && root.account.profile.id) root.fillProfile()
            if (!root.orgInitialized && root.account.organization.id) root.fillOrganization()
            if (!root.policyInitialized && root.account.tokenSettings.api_tokens_enabled !== undefined) root.fillPolicy()
        }
        function onProfileSaved() { root.fillProfile() }
        function onOrganizationSaved() { root.fillOrganization() }
        function onTokenPolicySaved() { root.fillPolicy() }
        function onHotkeysChanged() { keys.text = root.account.hotkeyJson; root.keysBaseline = keys.text }
        function onHotkeysImported(json) { keys.text = json }
        function onPasswordChanged() { oldPassword.clear(); newPassword.clear(); confirmPassword.clear() }
    }
    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.preferredWidth: 240; Layout.fillHeight: true; color: Theme.subtle
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 18; spacing: 6
                Label { text: qsTr("Account & Organization"); font.pixelSize: 18; font.weight: Font.DemiBold; Layout.bottomMargin: 18 }
                Repeater {
                    model: [qsTr("Personal Info"),qsTr("Hotkeys"),qsTr("Members & Invitations"),qsTr("Organization Settings"),qsTr("Email Preferences"),qsTr("Membership Info"),qsTr("Personal Access Token"),qsTr("Legacy Token"),qsTr("Change Password")]
                    StudioButton { required property string modelData; required property int index; Layout.fillWidth: true; text: modelData; primary: root.section===index; quiet: root.section!==index; onClicked: root.showSection(index) }
                }
                Item { Layout.fillHeight: true }
                Label { text: backend.serverUrl; Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; font.pixelSize: 12; color: Theme.muted }
                StudioButton { text: qsTr("Refresh"); quiet: true; enabled: !backend.busy && !backend.demo; onClicked: root.guard(function(){root.open(root.section)}) }
            }
        }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; contentWidth: availableWidth
            ColumnLayout {
                width: parent.width; spacing: 20
                Item { Layout.preferredHeight: 20 }
                Label { Layout.leftMargin: 40; Layout.rightMargin: 40; Layout.fillWidth: true; visible: backend.demo; text: qsTr("Connect to a server to manage your account and organization."); color: Theme.muted; wrapMode: Text.WordWrap }
                ColumnLayout {
                    Layout.margins: 40; Layout.topMargin: 0; Layout.fillWidth: true; Layout.maximumWidth: 900; spacing: 14; visible: root.section===0
                    Label { text: qsTr("Personal Info"); font.pixelSize: 28; font.weight: Font.DemiBold }
                    RowLayout {
                        Rectangle { width: 72; height: 72; radius: 36; color: Theme.selected
                            Label { anchors.centerIn: parent; text: root.account.profile.initials || "LS"; font.pixelSize: 22; color: Theme.accent }
                            Image { anchors.fill: parent; source: root.account.profile.avatar ? (String(root.account.profile.avatar).indexOf("://")>=0 ? root.account.profile.avatar : backend.serverUrl + String(root.account.profile.avatar).replace(/^\//,"")) : ""; fillMode: Image.PreserveAspectFit; visible: source.toString().length > 0 }
                        }
                        StudioButton { text: qsTr("Upload avatar"); enabled: !backend.busy && root.profileInitialized && !root.profileDirty; onClicked: avatarFile.open() }
                        StudioButton { text: qsTr("Remove avatar"); quiet: true; enabled: !backend.busy && !!root.account.profile.avatar && !root.profileDirty; onClicked: root.confirm(qsTr("Remove your current avatar?"),function(){root.account.removeAvatar()}) }
                    }
                    Label { text: qsTr("JPEG, GIF or PNG · up to 1 MB · maximum 1200 × 1200"); color: Theme.muted; font.pixelSize: 12 }
                    Label { text: qsTr("Email Address") }
                    StudioField { Layout.fillWidth: true; text: root.account.profile.email || ""; readOnly: true }
                    Label { text: qsTr("First Name") }
                    StudioField { id: first; objectName: "accountFirstName"; Layout.fillWidth: true; maximumLength: 100 }
                    Label { text: qsTr("Last Name") }
                    StudioField { id: last; Layout.fillWidth: true; maximumLength: 100 }
                    Label { text: qsTr("Phone") }
                    StudioField { id: phone; Layout.fillWidth: true; maximumLength: 100 }
                    StudioButton { objectName: "accountSaveProfile"; text: qsTr("Save changes"); primary: true; enabled: root.profileInitialized && !backend.busy && root.profileDirty; onClicked: root.account.saveProfile(first.text,last.text,phone.text) }
                }
                ColumnLayout {
                    Layout.margins: 40; Layout.topMargin: 0; Layout.fillWidth: true; Layout.maximumWidth: 900; spacing: 14; visible: root.section===1
                    Label { text: qsTr("Hotkeys"); font.pixelSize: 28; font.weight: Font.DemiBold }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Account defaults and personal project overrides are shared with the web app. Native actions use the action names listed below; other web actions are preserved."); color: Theme.muted }
                    RowLayout {
                        Label { text: qsTr("Scope") }
                        ComboBox { id: keyScope; Layout.preferredWidth: 200; model: [qsTr("Account defaults"),qsTr("Project override")]; onActivated: root.guard(function(){root.account.loadHotkeys(keyScope.currentIndex===1 ? Number(projectScope.text) : 0)}) }
                        StudioField { id: projectScope; Layout.preferredWidth: 110; placeholderText: qsTr("Project ID"); text: backend.project.id ? String(backend.project.id) : ""; visible: keyScope.currentIndex===1; validator: IntValidator { bottom: 1 } }
                        StudioButton { text: qsTr("Load"); enabled: !backend.busy && (keyScope.currentIndex===0 || projectScope.acceptableInput); onClicked: root.guard(function(){root.account.loadHotkeys(keyScope.currentIndex===1?Number(projectScope.text):0)}) }
                    }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Loaded scope: %1").arg(root.account.hotkeyProject ? qsTr("Project #%1").arg(root.account.hotkeyProject) : qsTr("Account defaults")) }
                    Repeater {
                        model: [
                            {action:"editor:save",name:qsTr("Save annotation"),key:"ctrl+s"},
                            {action:"annotation:annotation:submit",name:qsTr("Submit annotation"),key:"ctrl+enter"},
                            {action:"annotation:annotation:undo",name:qsTr("Undo"),key:"ctrl+z"},
                            {action:"annotation:annotation:redo",name:qsTr("Redo"),key:"ctrl+shift+z"},
                            {action:"regions:region:delete",name:qsTr("Delete region"),key:"delete"},
                            {action:"tools:tool:move",name:qsTr("Select / move"),key:"v"},
                            {action:annotationDocument.shapeType==="polygonlabels"?"tools:tool:polygon":"tools:tool:rect",name:qsTr("Draw shape"),key:"r"},
                            {action:"tools:tool:pan-image",name:qsTr("Pan image"),key:"h"}
                        ]
                        RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            Label { text: modelData.name; Layout.fillWidth: true }
                            StudioField { id: keyField; objectName: "accountHotkey"+modelData.action; Layout.preferredWidth: 180; text: root.hotkeyValue(modelData.action,modelData.key).key; enabled: !backend.busy; onTextEdited: root.editHotkey(modelData.action,text,keyEnabled.checked) }
                            CheckBox { id: keyEnabled; text: qsTr("Enabled"); checked: root.hotkeyValue(modelData.action,modelData.key).active!==false; enabled: !backend.busy; onClicked: root.editHotkey(modelData.action,keyField.text,checked) }
                        }
                    }
                    Label { text: qsTr("Advanced JSON (all actions)"); color: Theme.muted }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Example: {\"custom_hotkeys\": {\"editor:save\": {\"key\": \"ctrl+s\", \"active\": true}}}"); font.pixelSize: 12 }
                    ScrollView { Layout.fillWidth: true; Layout.preferredHeight: 200; StudioArea { id: keys; objectName: "accountHotkeys"; font.family: "Consolas"; selectByMouse: true; wrapMode: TextEdit.WrapAnywhere } }
                    RowLayout {
                        StudioButton { text: qsTr("Save hotkeys"); primary: true; enabled: !backend.busy && root.account.loaded; onClicked: root.account.saveHotkeys(keys.text) }
                        StudioButton { text: qsTr("Import JSON"); enabled: !backend.busy; onClicked: keysImport.open() }
                        StudioButton { text: qsTr("Export JSON"); onClicked: keysExport.open() }
                        StudioButton { text: qsTr("Reset"); enabled: !backend.busy && root.account.loaded; onClicked: root.confirm(qsTr("Reset this scope to inherited/default hotkeys?"),function(){root.account.saveHotkeys('{"custom_hotkeys":{}}')}) }
                    }
                }
                ColumnLayout {
                    Layout.margins: 40; Layout.topMargin: 0; Layout.fillWidth: true; spacing: 16; visible: root.section===2
                    RowLayout { Label { text: qsTr("Members & Invitations"); font.pixelSize: 28; font.weight: Font.DemiBold; Layout.fillWidth: true } StudioButton { text: qsTr("Invite Members"); primary: true; enabled: !backend.busy && root.account.loaded; onClicked: { root.account.loadInvite(); invitation.open() } } }
                    Label { text: qsTr("%1 members").arg(root.account.memberCount); color: Theme.muted }
                    Repeater {
                        model: root.account.members
                        Rectangle {
                            required property var modelData
                            Layout.fillWidth: true; implicitHeight: memberRow.implicitHeight+24; color: Theme.surface; border.color: Theme.border; radius: 5
                            RowLayout { id: memberRow; anchors.fill: parent; anchors.margins: 12
                                ColumnLayout { Layout.fillWidth: true
                                    Label { text: (modelData.user.first_name || "") + " " + (modelData.user.last_name || ""); font.weight: Font.DemiBold }
                                    Label { text: modelData.user.email || ""; color: Theme.muted }
                                    Label { text: qsTr("Last activity: %1").arg(root.date(modelData.user.last_activity)); font.pixelSize: 12; color: Theme.muted }
                                }
                                Label { text: qsTr("Owner"); visible: modelData.user.id===root.account.organization.created_by; color: Theme.accent }
                                StudioButton { text: qsTr("Details"); quiet: true; enabled: !backend.busy; onClicked: { root.selectedMember=modelData.user.id;root.account.loadMember(root.selectedMember);memberDialog.open() } }
                                StudioButton { text: qsTr("Remove"); destructive: true; visible: root.account.organization.created_by===root.account.profile.id && modelData.user.id!==root.account.profile.id; enabled: !backend.busy; onClicked: { const userId=modelData.user.id;root.confirm(qsTr("Remove %1 from this organization?").arg(modelData.user.email),function(){root.account.removeMember(userId)}) } }
                            }
                        }
                    }
                    RowLayout { StudioButton { text: qsTr("Previous"); enabled: !backend.busy && root.account.memberPage>1; onClicked: root.account.loadMembers(root.account.memberPage-1) } Label { text: qsTr("Page %1").arg(root.account.memberPage) } StudioButton { text: qsTr("Next"); enabled: !backend.busy && root.account.memberPage*20<root.account.memberCount; onClicked: root.account.loadMembers(root.account.memberPage+1) } }
                }
                ColumnLayout {
                    Layout.margins: 40; Layout.topMargin: 0; Layout.fillWidth: true; Layout.maximumWidth: 900; spacing: 16; visible: root.section===3
                    Label { text: qsTr("Organization Settings"); font.pixelSize: 28; font.weight: Font.DemiBold }
                    Label { text: qsTr("Organization Name") }
                    StudioField { id: orgTitle; objectName: "accountOrganizationName"; Layout.fillWidth: true }
                    Label { text: qsTr("Contact Info") }
                    StudioField { id: orgContact; Layout.fillWidth: true }
                    Label { text: qsTr("Created: %1").arg(root.date(root.account.organization.created_at)); color: Theme.muted }
                    StudioButton { text: qsTr("Save organization"); primary: true; enabled: !backend.busy && root.orgInitialized && root.orgDirty; onClicked: root.account.saveOrganization(orgTitle.text,orgContact.text) }
                    Label { text: qsTr("API Tokens Settings"); font.pixelSize: 22; Layout.topMargin: 24 }
                    CheckBox { id: personal; text: qsTr("Allow personal access tokens"); enabled: root.policyInitialized }
                    CheckBox { id: legacy; text: qsTr("Allow legacy API tokens"); enabled: root.policyInitialized }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Disabling a token type prevents clients using it from authenticating. Keep a working session available."); color: Theme.muted }
                    StudioButton { text: qsTr("Save token policy"); primary: true; enabled: !backend.busy && root.policyDirty; onClicked: root.confirm(qsTr("Apply this organization-wide token policy? Existing integrations may lose access."),function(){root.account.saveTokenSettings(personal.checked,legacy.checked)}) }
                }
                ColumnLayout {
                    Layout.margins: 40; Layout.topMargin: 0; Layout.fillWidth: true; Layout.maximumWidth: 900; spacing: 16; visible: root.section===4
                    Label { text: qsTr("Email Preferences"); font.pixelSize: 28; font.weight: Font.DemiBold }
                    CheckBox { id: newsletters; text: qsTr("Receive newsletters and product updates") }
                    StudioButton { text: qsTr("Save preferences"); primary: true; enabled: !backend.busy && root.profileInitialized && newsletters.checked!==root.emailBaseline; onClicked: root.account.saveEmailPreferences(newsletters.checked) }
                }
                ColumnLayout {
                    Layout.margins: 40; Layout.topMargin: 0; Layout.fillWidth: true; Layout.maximumWidth: 900; spacing: 16; visible: root.section===5
                    Label { text: qsTr("Membership Info"); font.pixelSize: 28; font.weight: Font.DemiBold }
                    Label { text: qsTr("Organization: %1").arg(root.account.organization.title || "—") }
                    Label { text: qsTr("Organization contact: %1").arg((root.account.profile.active_organization_meta || {}).email || "—") }
                    Label { text: qsTr("Joined: %1").arg(root.date(root.account.profile.date_joined)) }
                    Label { text: qsTr("Annotations: %1").arg(root.account.memberDetail.annotations_count || 0) }
                    Label { text: qsTr("Contributed projects: %1").arg(root.account.memberDetail.contributed_projects_count || 0) }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Created projects: %1").arg(root.projectNames(root.account.memberDetail.created_projects)) }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Contributed to: %1").arg(root.projectNames(root.account.memberDetail.contributed_to_projects)) }
                }
                ColumnLayout {
                    Layout.margins: 40; Layout.topMargin: 0; Layout.fillWidth: true; Layout.maximumWidth: 900; spacing: 16; visible: root.section===6
                    Label { text: qsTr("Personal Access Token"); font.pixelSize: 28; font.weight: Font.DemiBold }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Full tokens are shown only when created or rotated. Existing entries contain no signature and cannot be used as login credentials."); color: Theme.muted }
                    Label { visible: root.policyInitialized && !root.account.tokenSettings.api_tokens_enabled; text: qsTr("Personal access tokens are disabled by organization policy."); color: Theme.danger }
                    Repeater { model: root.account.tokens; ColumnLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        Label { text: String(modelData.token || "").slice(0,40) + "…"; font.family: "Consolas" }
                        StudioButton { text: qsTr("Revoke"); destructive: true; enabled: !backend.busy; onClicked: { const token=modelData.token;root.confirm(qsTr("Revoke this token? Clients using it will lose access."),function(){root.account.revokeToken(token)}) } }
                    } }
                    StudioButton { text: qsTr("Create token"); primary: true; enabled: !backend.busy && !!root.account.tokenSettings.api_tokens_enabled && root.account.tokens.length===0; onClicked: root.account.createToken() }
                    StudioField { Layout.fillWidth: true; text: root.account.secret; visible: text.length>0; echoMode: secretReveal.checked ? TextInput.Normal : TextInput.Password; readOnly: true }
                    RowLayout { visible: root.account.secret.length>0; CheckBox { id: secretReveal; text: qsTr("Show token") } StudioButton { text: qsTr("Copy token"); onClicked: root.account.copyText(root.account.secret) } StudioButton { text: qsTr("Clear"); quiet: true; onClicked: root.account.clearSecret() } }
                    Label { text: qsTr("Rotate an existing full token"); Layout.topMargin: 16 }
                    StudioField { id: secretInput; Layout.fillWidth: true; echoMode: TextInput.Password; placeholderText: qsTr("Paste the full refresh token") }
                    StudioButton { text: qsTr("Rotate token"); enabled: !backend.busy && secretInput.text.length>0 && !!root.account.tokenSettings.api_tokens_enabled; onClicked: root.confirm(qsTr("Replace this token and revoke the old one?"),function(){root.account.rotateToken(secretInput.text);secretInput.clear()}) }
                }
                ColumnLayout {
                    Layout.margins: 40; Layout.topMargin: 0; Layout.fillWidth: true; Layout.maximumWidth: 900; spacing: 16; visible: root.section===7
                    Label { text: qsTr("Legacy Token"); font.pixelSize: 28; font.weight: Font.DemiBold }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: root.account.tokenSettings.legacy_api_tokens_enabled ? qsTr("Legacy tokens remain valid until reset.") : qsTr("Legacy tokens are disabled by organization policy."); color: Theme.muted }
                    StudioField { Layout.fillWidth: true; text: root.account.legacyToken; readOnly: true; echoMode: legacyReveal.checked?TextInput.Normal:TextInput.Password }
                    CheckBox { id: legacyReveal; text: qsTr("Show token") }
                    RowLayout {
                        StudioButton { text: qsTr("Load token"); enabled: !backend.busy && !!root.account.tokenSettings.legacy_api_tokens_enabled; onClicked: root.account.loadLegacyToken() }
                        StudioButton { text: qsTr("Copy token"); enabled: root.account.legacyToken.length>0; onClicked: root.account.copyText(root.account.legacyToken) }
                        StudioButton { text: qsTr("Reset token"); destructive: true; enabled: !backend.busy && !!root.account.tokenSettings.legacy_api_tokens_enabled; onClicked: root.confirm(qsTr("Reset your legacy token? The previous token will stop working immediately."),function(){root.account.resetLegacyToken()}) }
                    }
                }
                ColumnLayout {
                    Layout.margins: 40; Layout.topMargin: 0; Layout.fillWidth: true; Layout.maximumWidth: 900; spacing: 16; visible: root.section===8
                    Label { text: qsTr("Change Password"); font.pixelSize: 28; font.weight: Font.DemiBold }
                    Label { text: qsTr("Current password") }
                    StudioField { id: oldPassword; Layout.fillWidth: true; echoMode: TextInput.Password }
                    Label { text: qsTr("New password") }
                    StudioField { id: newPassword; Layout.fillWidth: true; echoMode: TextInput.Password }
                    Label { text: qsTr("Confirm new password") }
                    StudioField { id: confirmPassword; Layout.fillWidth: true; echoMode: TextInput.Password }
                    Label { visible: confirmPassword.text.length>0 && newPassword.text!==confirmPassword.text; text: qsTr("Passwords do not match."); color: Theme.danger }
                    StudioButton { text: qsTr("Change Password"); primary: true; enabled: !backend.busy && root.account.loaded && oldPassword.text.length>0 && newPassword.text.length>0 && newPassword.text===confirmPassword.text; onClicked: root.account.changePassword(oldPassword.text,newPassword.text) }
                }
                Item { Layout.preferredHeight: 80 }
            }
        }
    }
    FileDialog { id: avatarFile; title: qsTr("Choose avatar"); nameFilters: [qsTr("Images (*.jpg *.jpeg *.png *.gif)")]; onAccepted: root.account.uploadAvatar(selectedFile) }
    FileDialog { id: keysImport; title: qsTr("Import hotkeys"); nameFilters: ["JSON (*.json)"]; onAccepted: root.account.importHotkeys(selectedFile) }
    FileDialog { id: keysExport; title: qsTr("Export hotkeys"); fileMode: FileDialog.SaveFile; defaultSuffix: "json"; nameFilters: ["JSON (*.json)"]; onAccepted: root.account.exportHotkeys(selectedFile,keys.text) }
    Dialog { id: unsaved; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 460; title: qsTr("Unsaved changes"); standardButtons: Dialog.Discard | Dialog.Cancel
        Label { width: parent.width; text: qsTr("Discard the unsaved changes and continue?"); wrapMode: Text.WordWrap }
        onDiscarded: { root.discard();const action=root.pendingAction;root.pendingAction=null;if(action)action() }
        onRejected: root.pendingAction=null
    }
    Dialog { id: confirmation; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 480; title: qsTr("Confirm action"); standardButtons: Dialog.Ok | Dialog.Cancel
        Label { width: parent.width; text: root.confirmText; wrapMode: Text.WordWrap }
        onAccepted: { const action=root.confirmAction;root.confirmAction=null;if(action)action() }
        onRejected: root.confirmAction=null
    }
    Dialog { id: invitation; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 600; title: qsTr("Invite Members"); standardButtons: Dialog.Close
        ColumnLayout { width: parent.width; spacing: 16
            Label { Layout.fillWidth: true; visible: backend.messageIsError; text: backend.message; color: Theme.danger; wrapMode: Text.WordWrap }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Share this invitation link with people you want to join the organization. Resetting it invalidates previous invitations.") }
            StudioField { Layout.fillWidth: true; text: root.account.inviteUrl; readOnly: true }
            RowLayout { StudioButton { text: qsTr("Copy link"); enabled: root.account.inviteUrl.length>0; onClicked: root.account.copyText(root.account.inviteUrl) } StudioButton { text: qsTr("Reset invitation link"); destructive: true; enabled: !backend.busy; onClicked: root.confirm(qsTr("Invalidate all previously shared invitation links?"),function(){root.account.resetInvite()}) } }
        }
    }
    Dialog { id: memberDialog; parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 620; title: qsTr("Member details"); standardButtons: Dialog.Close
        ColumnLayout { width: parent.width; spacing: 16
            Label { text: qsTr("User #%1").arg(root.selectedMember); font.pixelSize: 18 }
            Label { text: qsTr("Joined: %1").arg(root.date(root.account.memberDetail.created_at)) }
            Label { text: qsTr("Annotations: %1").arg(root.account.memberDetail.annotations_count || 0) }
            Label { text: qsTr("Contributed projects: %1").arg(root.account.memberDetail.contributed_projects_count || 0) }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Created projects: %1").arg(root.projectNames(root.account.memberDetail.created_projects)) }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Contributed to: %1").arg(root.projectNames(root.account.memberDetail.contributed_to_projects)) }
        }
    }
}
