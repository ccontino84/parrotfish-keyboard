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
    // Whether the field shows the preedit (the word being typed) in
    // surroundingText, as some apps do.
    property bool showsPreedit: false
    property string _base: ""      // the text without the preedit
    property int _baseCursor: -1   // -1: no preedit shown
    function _moveBy(n) {
        if (movesCursor)
            cursorPosition = Math.max(0, Math.min(surroundingText.length, cursorPosition + n))
    }
    function sendKey(key, modifiers, text, type) {
        sentKeys = sentKeys.concat([key])
        keyTypes = keyTypes.concat([type])
        if (type !== 1 && (key === Qt.Key_Left || key === Qt.Key_Right))   // not on a release
            _moveBy(key === Qt.Key_Left ? -1 : 1)
        if (key === Qt.Key_Backspace && backspaceCodePoints > 0) {
            var end = cursorPosition
            var start = end
            for (var n = 0; n < backspaceCodePoints && start > 0; ++n) {
                var c = surroundingText.charCodeAt(start - 1)
                start -= (c >= 0xDC00 && c <= 0xDFFF && start >= 2) ? 2 : 1
            }
            var text = surroundingText.substring(0, start) + surroundingText.substring(end)
            cursorPosition = start
            surroundingText = text
        }
    }
    // Code points a Backspace key event deletes before the cursor (0: the
    // text stays as the test set it).
    property int backspaceCodePoints: 0
    property var preedits: []   // [text, replaceStart, replaceLength, cursorPos]
    function sendPreedit(text, formatting, replaceStart, replaceLength, cursorPos) {
        preedits = preedits.concat([[text, replaceStart, replaceLength, cursorPos]])
        if (!showsPreedit || !surroundingTextValid)
            return
        if (_baseCursor < 0) {
            _base = surroundingText
            _baseCursor = cursorPosition
        }
        surroundingText = _base.substring(0, _baseCursor) + text + _base.substring(_baseCursor)
        cursorPosition = _baseCursor + text.length
    }
    function sendCommit(text, replaceStart, replaceLength, cursorPos) {
        commits = commits.concat([[replaceStart, replaceLength, cursorPos]])
        committedText += text
        if (_baseCursor >= 0) {
            // The field stays as the app last reported it (the preedit in
            // it) until the app reports again: no change, no signal. With
            // insertsCommits the commit takes the preedit's place at once.
            if (insertsCommits) {
                surroundingText = _base
                cursorPosition = _baseCursor
            }
            _baseCursor = -1
        }
        if (insertsCommits && surroundingTextValid && !replaceLength && !replaceStart) {
            surroundingText = surroundingText.substring(0, cursorPosition) + text
                    + surroundingText.substring(cursorPosition)
            cursorPosition += text.length
        }
        if (text === "" && replaceLength === 0)
            _moveBy(replaceStart + Math.max(0, cursorPos))
    }
}
