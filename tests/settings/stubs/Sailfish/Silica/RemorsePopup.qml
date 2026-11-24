import QtQuick 2.15
Item { property string text; signal triggered; signal canceled; function execute(text, fn, timeout) { Remorse.last = {"text": text, "fn": fn} } }
