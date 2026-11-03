pragma Singleton
import QtQuick 2.15
QtObject {
    property bool active: true
    property int appOrientation: 0
    property int contentType: 0
    property bool hiddenText: false
    property bool predictionEnabled: true
    property var actionKeyOverride: ({ "icon": "", "label": "Enter", "enabled": true, "highlighted": false })
    // The editor's text: tests set these; sendKey() records the keys in sentKeys.
    property string surroundingText: ""
    property int cursorPosition: 0
    property bool surroundingTextValid: true
    property var sentKeys: []
    property var commits: []   // [replaceStart, replaceLength, cursorPos] of empty commits
    property string committedText: ""   // every committed text, joined
    property var extensions: ({})
    signal focusTargetChanged(bool activeEditor)
    signal inputMethodReset()
    property var keyTypes: []
    // Whether the editor moves its cursor for arrow keys and placing commits
    // (false: as an Android app).
    property bool movesCursor: true
    // Whether a commit goes into surroundingText at once, as maliit's copy of
    // the field does (no selection, no replacement), before the app reports.
    property bool insertsCommits: false
    function _moveBy(n) {
        if (movesCursor)
            cursorPosition = Math.max(0, Math.min(surroundingText.length, cursorPosition + n))
    }
    function sendKey(key, modifiers, text, type) {
        sentKeys = sentKeys.concat([key])
        keyTypes = keyTypes.concat([type])
        if (type !== 1 && (key === Qt.Key_Left || key === Qt.Key_Right))   // not on a release
            _moveBy(key === Qt.Key_Left ? -1 : 1)
    }
    property var preedits: []   // [text, replaceStart, replaceLength, cursorPos]
    function sendPreedit(text, formatting, replaceStart, replaceLength, cursorPos) {
        preedits = preedits.concat([[text, replaceStart, replaceLength, cursorPos]])
    }
    function sendCommit(text, replaceStart, replaceLength, cursorPos) {
        commits = commits.concat([[replaceStart, replaceLength, cursorPos]])
        committedText += text
        if (insertsCommits && surroundingTextValid && !replaceLength && !replaceStart) {
            surroundingText = surroundingText.substring(0, cursorPosition) + text
                    + surroundingText.substring(cursorPosition)
            cursorPosition += text.length
        }
        if (text === "" && replaceLength === 0)
            _moveBy(replaceStart + Math.max(0, cursorPos))
    }
}
