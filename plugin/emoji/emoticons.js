// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
.pragma library

// Emoticons and their emoji: the emoji button offers the emoji after one,
// and Space can replace it (ParrotfishHandler.qml). (y) and (n) are left
// out: Space would replace list items such as "(n) …".
var table = [
    [":)", "🙂"], [":-)", "🙂"], [":(", "🙁"], [":-(", "🙁"],
    [":D", "😀"], [":-D", "😀"], [";)", "😉"], [";-)", "😉"],
    [":P", "😛"], [":-P", "😛"], [":p", "😛"], [":-p", "😛"],
    [";P", "😜"], [";-P", "😜"], [";p", "😜"], [";-p", "😜"],
    [":O", "😮"], [":-O", "😮"], [":o", "😮"], [":-o", "😮"],
    [":'(", "😢"], [":')", "🥲"], [":*", "😘"], [":-*", "😘"],
    [":|", "😐"], [":-|", "😐"], [":/", "😕"], [":-/", "😕"],
    [":S", "😖"], [":-S", "😖"], [":$", "😳"], [":-$", "😳"],
    [">:(", "😠"], [">:-(", "😠"], ["O:)", "😇"], ["O:-)", "😇"],
    ["XD", "😆"], ["xD", "😆"], ["B)", "😎"], ["B-)", "😎"], ["8-)", "😎"],
    ["^^", "😊"], ["^_^", "😊"], ["-_-", "😑"], ["<3", "❤️"], ["</3", "💔"],
    ["\\m/", "🤘"], ["\\o/", "🙌"], ["o/", "👋"]
]

// The emoticon `text` ends with ({ text, emoji }), or null: the longest
// one, at the start of `text` or after a space (http:/ is no emoticon).
function atEnd(text) {
    var found = null
    for (var i = 0; i < table.length; ++i) {
        var e = table[i][0]
        var start = text.length - e.length
        if (start < 0 || text.substring(start) !== e || (found && found.text.length >= e.length))
            continue
        if (start === 0 || /\s/.test(text.charAt(start - 1)))
            found = { "text": e, "emoji": table[i][1] }
    }
    return found
}
