pragma Singleton
import QtQuick

QtObject {
    property bool dark: false
    readonly property color background: dark ? "#1b1a18" : "#fdfdfc"
    readonly property color surface: dark ? "#23221f" : "#fdfdfc"
    readonly property color subtle: dark ? "#2b2925" : "#f9f8f6"
    readonly property color canvas: dark ? "#191816" : "#f3f2ef"
    readonly property color border: dark ? "#45413a" : "#e1ded5"
    readonly property color text: dark ? "#f0efed" : "#262522"
    readonly property color muted: dark ? "#b0aba1" : "#77736b"
    readonly property color accent: dark ? "#acb9ee" : "#4c5fa9"
    readonly property color selected: dark ? "#30364d" : "#f0f3fe"
    readonly property color positive: dark ? "#83ba88" : "#3e7d46"
    readonly property color danger: dark ? "#e99784" : "#b3523d"
}
