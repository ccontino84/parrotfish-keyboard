pragma Singleton
import QtQuick 2.15
// The last remorse asked for: its text and the function it would run.
QtObject {
    property var last: null
    function popupAction(item, text, fn) { last = {"text": text, "fn": fn} }
    function itemAction(item, text, fn) { last = {"text": text, "fn": fn} }
}
