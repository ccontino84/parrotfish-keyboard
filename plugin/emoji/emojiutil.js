// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
.pragma library

// "1f9d1-200d-1f4bb" for 🧑‍💻, as EmojiImageProvider reads it (QML's
// JavaScript has no codePointAt).
function codePoints(text) {
    var out = []
    for (var i = 0; i < text.length; ++i) {
        var c = text.charCodeAt(i)
        if (c >= 0xD800 && c <= 0xDBFF && i + 1 < text.length) {
            var low = text.charCodeAt(i + 1)
            if (low >= 0xDC00 && low <= 0xDFFF) {
                c = (c - 0xD800) * 0x400 + (low - 0xDC00) + 0x10000
                ++i
            }
        }
        out.push(c.toString(16))
    }
    return out.join("-")
}

// The image of an emoji, drawn in colour by the plugin.
function source(text) {
    return text !== "" ? "image://parrotfish-emoji/" + codePoints(text) : ""
}

// The recent emoji: as many as one screen of the panel holds, by how often
// they were used, older uses counting less (one can be removed by hand:
// recentForget()). Kept in dconf as text, one emoji per line: the emoji, a
// tab, its score.
var recentMax = 36
var recentFading = 0.97   // every use, the others' scores fade by this

function _parseRecent(stored) {
    var out = []
    var lines = String(stored || "").split("\n")
    for (var i = 0; i < lines.length; ++i) {
        var parts = lines[i].split("\t")
        var score = parseFloat(parts[1])
        if (parts[0] !== "" && score > 0)
            out.push({ "emoji": parts[0], "score": score })
    }
    return out
}

// The emoji to show, most used first.
function recentList(stored) {
    return _parseRecent(stored).map(function(e) { return e.emoji })
}

// `stored` after `emoji` was used: it gains a point, the others fade. A
// new emoji always gets in: it takes the place of the lowest other one.
function recentUse(stored, emoji) {
    var entries = _parseRecent(stored)
    var used = null
    var others = []
    for (var i = 0; i < entries.length; ++i) {
        if (entries[i].emoji === emoji)
            used = entries[i]
        else
            others.push({ "emoji": entries[i].emoji, "score": entries[i].score * recentFading })
    }
    used = { "emoji": emoji, "score": (used ? used.score * recentFading : 0) + 1 }
    // Highest first; equal scores keep their order (the sort may not be stable).
    others = others.map(function(e, index) { e.index = index; return e })
    others.sort(function(a, b) { return b.score - a.score || a.index - b.index })
    others = others.slice(0, recentMax - 1)
    var all = []
    var placed = false
    for (i = 0; i < others.length; ++i) {
        if (!placed && used.score >= others[i].score) {
            all.push(used)   // before the equal ones: the latest first
            placed = true
        }
        all.push(others[i])
    }
    if (!placed)
        all.push(used)
    return all.map(function(e) { return e.emoji + "\t" + e.score.toFixed(3) }).join("\n")
}

// `stored` without `emoji` (removed by hand).
function recentForget(stored, emoji) {
    return _parseRecent(stored).filter(function(e) { return e.emoji !== emoji })
        .map(function(e) { return e.emoji + "\t" + e.score.toFixed(3) }).join("\n")
}

// The variants picked from emoji's long-press menus: kept in dconf as text,
// one emoji per line: the emoji, a tab, the variant. pick() with the emoji
// itself as the variant takes its entry out.
function pickedMap(stored) {
    var out = {}
    var lines = String(stored || "").split("\n")
    for (var i = 0; i < lines.length; ++i) {
        var parts = lines[i].split("\t")
        if (parts.length === 2 && parts[0] !== "" && parts[1] !== "")
            out[parts[0]] = parts[1]
    }
    return out
}

function pick(stored, emoji, variant) {
    var map = pickedMap(stored)
    if (variant === emoji)
        delete map[emoji]
    else
        map[emoji] = variant
    var lines = []
    for (var e in map)
        lines.push(e + "\t" + map[e])
    return lines.join("\n")
}

// The code point ending at `end` in `text`: { code, length } (length 0 at
// the start).
function codePointBefore(text, end) {
    if (end <= 0)
        return { "code": -1, "length": 0 }
    var low = text.charCodeAt(end - 1)
    if (low >= 0xDC00 && low <= 0xDFFF && end >= 2) {
        var high = text.charCodeAt(end - 2)
        if (high >= 0xD800 && high <= 0xDBFF)
            return { "code": (high - 0xD800) * 0x400 + (low - 0xDC00) + 0x10000, "length": 2 }
    }
    return { "code": low, "length": 1 }
}

function withoutLastCodePoint(text) {
    return text.substring(0, text.length - codePointBefore(text, text.length).length)
}

// The emoji of more than one code point that `text` ends with (a skin
// tone, a ZWJ sequence, a flag, a keycap, a subdivision flag's tags), or
// "": what one Backspace should delete (apps delete one code point per
// Backspace key event).
function emojiAtEnd(text) {
    var isExtender = function(c) {
        return c === 0xFE0F || c === 0x20E3 || (c >= 0x1F3FB && c <= 0x1F3FF)
                || (c >= 0xE0020 && c <= 0xE007F)
    }
    var isFlagLetter = function(c) { return c >= 0x1F1E6 && c <= 0x1F1FF }
    var end = text.length
    var pos = end
    var points = 0
    var c = codePointBefore(text, pos)
    if (isFlagLetter(c.code)) {
        // Flags are pairs of letters: count them from the start of the run.
        var run = 0
        for (var p = pos; isFlagLetter(codePointBefore(text, p).code); p -= 2)
            ++run
        return run >= 2 && run % 2 === 0 ? text.substring(end - 4) : ""
    }
    for (;;) {
        for (c = codePointBefore(text, pos); c.length > 0 && isExtender(c.code);
             c = codePointBefore(text, pos)) {
            pos -= c.length
            ++points
        }
        if (c.length === 0)
            break
        pos -= c.length   // the base
        ++points
        c = codePointBefore(text, pos)
        if (c.code !== 0x200D || pos < 2)
            break
        pos -= c.length   // ZWJ: another emoji before it
        ++points
    }
    return points >= 2 ? text.substring(pos) : ""
}
