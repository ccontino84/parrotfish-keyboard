.pragma library
// SPDX-FileCopyrightText: Jolla Ltd.
// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-TBD
//
// The key arrangements of the Parrotfish layout (parrotfish.qml), as data:
// the setting /apps/parrotfish-keyboard/layout picks one. Each has three
// letter rows; "split" is the child index where a split row's right half
// starts (row 3 counts the Shift key before the letters). Keys: c caption,
// s shifted, y and y2 the two symbol pages, a and A accents (A shifted;
// not "as": Qt 5.6 reads that as a keyword in QML files), fit = shrink
// the caption to the key's width, sticky = the sticky key (stickySigns:
// it types the sign last picked from its long press; QWERTZ and QWERTY ÅÖÄ
// get one in the bottom letter row, placeSticky).
//
// The keys come from en.qml (QWERTY), de.qml (QWERTZ), fr.qml (AZERTY) and
// fi.qml (QWERTY ÅÖÄ: sv.qml has the same letters) of jolla-keyboard
// 0.10.4 (Jolla Ltd., BSD-3-Clause, notice below).
// QWERTY changes: a
// full-width apostrophe key ends the middle row (no half-key indent, as on
// Jolla's it.qml) and is the sticky key, the middle row of the
// symbol view is it.qml's but keeps en.qml's ` (~ before ! and ?, ° on the
// second page), and M's second-page ~ became × (as on de.qml). QWERTZ:
// de.qml's keys. AZERTY: fr.qml's keys, but the apostrophe takes the place
// of ç after N (fr.qml has it in the bottom row, which narrows the
// spacebar) and is the sticky key; ç is on C's long press, and typed as c
// the engine puts it back. As the sticky key may type something else, the
// symbol pages have ' and " where en.qml has them: ' on B, " on C's second
// page (fr.qml's ÷ there is dropped). QWERTY ÅÖÄ: fi.qml's keys.
// All: more accents for the typing languages, from Jolla's da/fr/pt/ro
// layouts (A: æ ă, O: õ œ, S: ș, T: ț).

/*
 * Copyright (C) 2012-2013 Jolla ltd and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: Pekka Vuorela <pekka.vuorela@jollamobile.com>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * Neither the name of Jolla Ltd nor the names of its contributors may be
 * used to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


var arrangements = {
    "qwerty": { "name": "QWERTY", "rows": [
        { "split": 5, "keys": [
            {"c": "q", "s": "Q", "y": "1", "y2": "€"},
            {"c": "w", "s": "W", "y": "2", "y2": "£"},
            {"c": "e", "s": "E", "y": "3", "y2": "$", "a": "èeéêë€", "A": "ÈEÉÊË€"},
            {"c": "r", "s": "R", "y": "4", "y2": "¥"},
            {"c": "t", "s": "T", "y": "5", "y2": "₹", "a": "tþț", "A": "TÞȚ"},
            {"c": "y", "s": "Y", "y": "6", "y2": "%", "a": "ýy¥", "A": "ÝY¥"},
            {"c": "u", "s": "U", "y": "7", "y2": "<", "a": "űûùuúü", "A": "ŰÛÙUÚÜ"},
            {"c": "i", "s": "I", "y": "8", "y2": ">", "a": "îïìií", "A": "ÎÏÌIÍ"},
            {"c": "o", "s": "O", "y": "9", "y2": "[", "a": "őøöôòoóõœ", "A": "ŐØÖÔÒOÓÕŒ"},
            {"c": "p", "s": "P", "y": "0", "y2": "]"}
        ] },
        { "split": 5, "keys": [
            {"c": "a", "s": "A", "y": "*", "y2": "`", "a": "aäàâáãåæă", "A": "AÄÀÂÁÃÅÆĂ"},
            {"c": "s", "s": "S", "y": "#", "y2": "^", "a": "sßș$", "A": "SẞȘ$"},
            {"c": "d", "s": "D", "y": "+", "y2": "|", "a": "dð", "A": "DÐ"},
            {"c": "f", "s": "F", "y": "-", "y2": "_"},
            {"c": "g", "s": "G", "y": "=", "y2": "§"},
            {"c": "h", "s": "H", "y": "(", "y2": "{"},
            {"c": "j", "s": "J", "y": ")", "y2": "}"},
            {"c": "k", "s": "K", "y": "~", "y2": "°"},
            {"c": "l", "s": "L", "y": "!", "y2": "¡"},
            {"c": "'", "s": "'", "y": "?", "y2": "¿", "sticky": true}
        ] },
        { "split": 5, "keys": [
            {"c": "z", "s": "Z", "y": "@", "y2": "«"},
            {"c": "x", "s": "X", "y": "&", "y2": "»"},
            {"c": "c", "s": "C", "y": "/", "y2": "\"", "a": "cç", "A": "CÇ"},
            {"c": "v", "s": "V", "y": "\\", "y2": "“"},
            {"c": "b", "s": "B", "y": "'", "y2": "”"},
            {"c": "n", "s": "N", "y": ";", "y2": "„", "a": "nñ", "A": "NÑ"},
            {"c": "m", "s": "M", "y": ":", "y2": "×"}
        ] }
    ] },
    "qwertz": { "name": "QWERTZ", "stickyInLastRow": true, "rows": [
        { "split": 5, "keys": [
            {"c": "q", "s": "Q", "y": "1", "y2": "€"},
            {"c": "w", "s": "W", "y": "2", "y2": "£"},
            {"c": "e", "s": "E", "y": "3", "y2": "$", "a": "èeéëê€", "A": "ÈEÉÊË€"},
            {"c": "r", "s": "R", "y": "4", "y2": "¥"},
            {"c": "t", "s": "T", "y": "5", "y2": "₹", "a": "tț", "A": "TȚ"},
            {"c": "z", "s": "Z", "y": "6", "y2": "¤"},
            {"c": "u", "s": "U", "y": "7", "y2": "<", "a": "üùuúû", "A": "ÜÙUÚÛ"},
            {"c": "i", "s": "I", "y": "8", "y2": ">", "a": "îïiìí", "A": "ÎÏIÌÍ"},
            {"c": "o", "s": "O", "y": "9", "y2": "[", "a": "öòóôõoøœ", "A": "ÖÒÓÔÕOØŒ"},
            {"c": "p", "s": "P", "y": "0", "y2": "]"},
            {"c": "ü", "s": "Ü", "y": "%", "y2": "‰", "fit": true}
        ] },
        { "split": 5, "keys": [
            {"c": "a", "s": "A", "y": "*", "y2": "`", "a": "aäàáãâåæă", "A": "AÄÀÁÃÂÅÆĂ"},
            {"c": "s", "s": "S", "y": "#", "y2": "^", "a": "sßşș$", "A": "SẞŞȘ$"},
            {"c": "d", "s": "D", "y": "+", "y2": "|", "a": "dð", "A": "DÐ"},
            {"c": "f", "s": "F", "y": "-", "y2": "_"},
            {"c": "g", "s": "G", "y": "=", "y2": "§", "a": "gğ", "A": "GĞ"},
            {"c": "h", "s": "H", "y": "(", "y2": "{"},
            {"c": "j", "s": "J", "y": ")", "y2": "}"},
            {"c": "k", "s": "K", "y": "\"", "y2": "°"},
            {"c": "l", "s": "L", "y": "~", "y2": "·"},
            {"c": "ö", "s": "Ö", "y": "!", "y2": "¡"},
            {"c": "ä", "s": "Ä", "y": "?", "y2": "¿"}
        ] },
        { "split": 5, "keys": [
            {"c": "y", "s": "Y", "y": "@", "y2": "«", "a": "ýy¥", "A": "ÝY¥"},
            {"c": "x", "s": "X", "y": "&", "y2": "»"},
            {"c": "c", "s": "C", "y": "/", "y2": "÷", "a": "cç", "A": "CÇ"},
            {"c": "v", "s": "V", "y": "\\", "y2": "“"},
            {"c": "b", "s": "B", "y": "'", "y2": "”"},
            {"c": "n", "s": "N", "y": ";", "y2": "„", "a": "nñ", "A": "NÑ"},
            {"c": "m", "s": "M", "y": ":", "y2": "×"}
        ] }
    ] },
    "azerty": { "name": "AZERTY", "rows": [
        { "split": 5, "keys": [
            {"c": "a", "s": "A", "y": "1", "y2": "€", "a": "aâàæäáãåă", "A": "AÂÀÆÄÁÃÅĂ"},
            {"c": "z", "s": "Z", "y": "2", "y2": "£"},
            {"c": "e", "s": "E", "y": "3", "y2": "$", "a": "èeéêë", "A": "ÈEÉÊË"},
            {"c": "r", "s": "R", "y": "4", "y2": "¥"},
            {"c": "t", "s": "T", "y": "5", "y2": "₹", "a": "tþț", "A": "TÞȚ"},
            {"c": "y", "s": "Y", "y": "6", "y2": "%", "a": "ýy¥", "A": "ÝY¥"},
            {"c": "u", "s": "U", "y": "7", "y2": "<", "a": "üûuùú", "A": "ÜÛUÙÚ"},
            {"c": "i", "s": "I", "y": "8", "y2": ">", "a": "íìîiï", "A": "ÍÌÎIÏ"},
            {"c": "o", "s": "O", "y": "9", "y2": "[", "a": "øöòóôoœõ", "A": "ØÖÒÓÔOŒÕ"},
            {"c": "p", "s": "P", "y": "0", "y2": "]"}
        ] },
        { "split": 5, "keys": [
            {"c": "q", "s": "Q", "y": "*", "y2": "`"},
            {"c": "s", "s": "S", "y": "#", "y2": "^", "a": "sș", "A": "SȘ"},
            {"c": "d", "s": "D", "y": "+", "y2": "|", "a": "dð", "A": "DÐ"},
            {"c": "f", "s": "F", "y": "-", "y2": "_"},
            {"c": "g", "s": "G", "y": "=", "y2": "§"},
            {"c": "h", "s": "H", "y": "(", "y2": "{"},
            {"c": "j", "s": "J", "y": ")", "y2": "}"},
            {"c": "k", "s": "K", "y": "~", "y2": "°"},
            {"c": "l", "s": "L", "y": "!", "y2": "¡"},
            {"c": "m", "s": "M", "y": "?", "y2": "¿"}
        ] },
        { "split": 5, "keys": [
            {"c": "w", "s": "W", "y": "@", "y2": "«"},
            {"c": "x", "s": "X", "y": "&", "y2": "»"},
            {"c": "c", "s": "C", "y": "/", "y2": "\"", "a": "cç", "A": "CÇ"},
            {"c": "v", "s": "V", "y": "\\", "y2": "“"},
            {"c": "b", "s": "B", "y": "'", "y2": "”"},
            {"c": "n", "s": "N", "y": ";", "y2": "„", "a": "nñ", "A": "NÑ"},
            {"c": "'", "s": "'", "y": ":", "y2": "‰", "sticky": true}
        ] }
    ] },
    "nordic": { "name": "QWERTY ÅÖÄ", "stickyInLastRow": true, "rows": [
        { "split": 5, "keys": [
            {"c": "q", "s": "Q", "y": "1", "y2": "€"},
            {"c": "w", "s": "W", "y": "2", "y2": "£"},
            {"c": "e", "s": "E", "y": "3", "y2": "$", "a": "éeèëê€", "A": "ÉEÈËÊ€"},
            {"c": "r", "s": "R", "y": "4", "y2": "¥"},
            {"c": "t", "s": "T", "y": "5", "y2": "₹", "a": "þtŧț", "A": "ÞTŦȚ"},
            {"c": "y", "s": "Y", "y": "6", "y2": "¤", "a": "y¥", "A": "Y¥"},
            {"c": "u", "s": "U", "y": "7", "y2": "<", "a": "üùuûú", "A": "ÜÙUÛÚ"},
            {"c": "i", "s": "I", "y": "8", "y2": ">", "a": "îïiìí", "A": "ÎÏIÌÍ"},
            {"c": "o", "s": "O", "y": "9", "y2": "[", "a": "öøòóôoõœ", "A": "ÖØÒÓÔOÕŒ"},
            {"c": "p", "s": "P", "y": "0", "y2": "]"},
            {"c": "å", "s": "Å", "y": "%", "y2": "‰", "fit": true}
        ] },
        { "split": 5, "keys": [
            {"c": "a", "s": "A", "y": "*", "y2": "`", "a": "aäåæàáâãă", "A": "AÄÅÆÀÁÂÃĂ"},
            {"c": "s", "s": "S", "y": "#", "y2": "^", "a": "sšßș$", "A": "SŠẞȘ$"},
            {"c": "d", "s": "D", "y": "+", "y2": "|", "a": "ðdđ", "A": "ÐDĐ"},
            {"c": "f", "s": "F", "y": "-", "y2": "_"},
            {"c": "g", "s": "G", "y": "=", "y2": "§"},
            {"c": "h", "s": "H", "y": "(", "y2": "{"},
            {"c": "j", "s": "J", "y": ")", "y2": "}"},
            {"c": "k", "s": "K", "y": "\"", "y2": "°"},
            {"c": "l", "s": "L", "y": "~", "y2": "·"},
            {"c": "ö", "s": "Ö", "y": "!", "y2": "¡", "a": "øö", "A": "ØÖ"},
            {"c": "ä", "s": "Ä", "y": "?", "y2": "¿", "a": "æä", "A": "ÆÄ"}
        ] },
        { "split": 5, "keys": [
            {"c": "z", "s": "Z", "y": "@", "y2": "«", "a": "zž", "A": "ZŽ"},
            {"c": "x", "s": "X", "y": "&", "y2": "»"},
            {"c": "c", "s": "C", "y": "/", "y2": "÷", "a": "çcč", "A": "ÇCČ"},
            {"c": "v", "s": "V", "y": "\\", "y2": "“"},
            {"c": "b", "s": "B", "y": "'", "y2": "”"},
            {"c": "n", "s": "N", "y": ";", "y2": "„", "a": "ñnŋ", "A": "ÑNŊ"},
            {"c": "m", "s": "M", "y": ":", "y2": "×"}
        ] }
    ] }
}

// What the sticky key can type, in the order of its long-press menu: a
// short list of the most used signs (agreed 2026-10-31); the setting
// /apps/parrotfish-keyboard/sticky-key keeps the one last picked.
var stickySigns = "'\"-_@#/"

// The sticky key on the right or the left of its row (agreed 2026-10-31:
// in the same row, so that no key gets smaller; "right" is the default,
// "left" suits left hands or keeps it away from Backspace). QWERTY has it
// at the end of the middle row, AZERTY after N. QWERTZ and QWERTY ÅÖÄ have
// no apostrophe key: theirs goes in the bottom letter row, whose end keys
// are wider than the letters and leave room; it has no symbol-view sign of
// its own there, so it types its sign in the symbol view too. On the left,
// the letters of its row move one key to the right, while the symbol-view
// signs stay in place (they go by position, not with the letters).
function placeSticky(a, side) {
    var left = side === "left"
    if (!left && !a.stickyInLastRow)
        return a
    var rows = []
    for (var r = 0; r < a.rows.length; ++r) {
        var keys = a.rows[r].keys.slice()
        var split = a.rows[r].split
        var at = -1
        for (var i = 0; i < keys.length; ++i)
            if (keys[i].sticky)
                at = i
        if (a.stickyInLastRow && r === a.rows.length - 1) {
            var key = {"c": "'", "s": "'", "sticky": true}
            if (left) {
                keys.unshift(key)
                split += 1   // the same letters on each half
            } else {
                keys.push(key)
            }
        } else if (left && at > 0) {
            var signs = []
            for (i = 0; i < keys.length; ++i)
                signs.push([keys[i].y, keys[i].y2])
            keys.unshift(keys.splice(at, 1)[0])
            for (i = 0; i < keys.length; ++i) {
                var copy = {}
                for (var p in keys[i])
                    copy[p] = keys[i][p]
                copy.y = signs[i][0]
                copy.y2 = signs[i][1]
                keys[i] = copy
            }
            split += 1
        }
        rows.push({"split": split, "keys": keys})
    }
    return {"name": a.name, "rows": rows}
}

// The arrangement called `name`, or QWERTY. With `numberRow`, it gets a
// "numbers" row above the letters (withNumberRow). `stickySide` ("right",
// "left") places the sticky key (placeSticky).
function get(name, numberRow, stickySide) {
    var a = arrangements[name] || arrangements["qwerty"]
    a = placeSticky(a, stickySide)
    return numberRow ? withNumberRow(a) : a
}

// The first row of the second symbol page while the number row is on:
// superscripts and fractions next to the digits, then the most used
// maths and typographic signs (agreed 2026-10-31); an 11th sign for
// 11-key rows.
var freeRow = "¹ ² ³ ½ ¼ ¾ ± ≈ – … ≠"

// `a` with a number row (revision 2, agreed 2026-10-31): Jolla's digit
// row of the first symbol page becomes the number row, shown the same in
// every view. Below it, the first symbol page is the second page's first
// row (currencies, %, brackets) over the first page's other two rows; the
// second page is freeRow over the second page's other two rows. Every
// other sign keeps its page and row.
function withNumberRow(a) {
    var digits = a.rows[0].keys
    var numbers = []
    for (var i = 0; i < digits.length; ++i)
        numbers.push({"c": digits[i].y, "s": digits[i].y, "y": digits[i].y, "y2": digits[i].y})
    var free = freeRow.split(" ")
    var rows = []
    for (var r = 0; r < a.rows.length; ++r) {
        var keys = []
        for (i = 0; i < a.rows[r].keys.length; ++i) {
            var k = a.rows[r].keys[i]
            var copy = {}
            for (var p in k)
                copy[p] = k[p]
            if (r === 0) {
                copy.y = k.y2
                copy.y2 = free[i] || ""
            }
            keys.push(copy)
        }
        rows.push({"split": a.rows[r].split, "keys": keys})
    }
    return {"name": a.name, "numbers": numbers, "rows": rows}
}

// The long-press menus of the comma and the period: Jolla's (: , ; and
// ! . ?) plus the signs of the typing languages (agreed 2026-10-31): ¡ ¿ with Spanish,
// the em dash — with English or Spanish. Kept short: the hyphen is on the
// sticky key, an ellipsis is typed as three periods, quotes are on the
// second symbol page. `languages` is the dconf value of
// /apps/parrotfish-keyboard/languages, `localeName` the system locale.
function punctuation(languages, localeName) {
    var codes = typingLanguages(languages, localeName)
    var es = codes.indexOf("es") >= 0
    var dash = es || codes.indexOf("en") >= 0
    return {"comma": ":,;" + (dash ? "—" : ""), "period": es ? "¡!.?¿" : "!.?"}
}

// The typing languages: the codes in `value` (separated by spaces), or the
// system language if none, else English. Same as selected() in
// settings/languages.js, with its codes.
var languageCodes = ["da", "de", "en", "es", "fr", "it", "nl", "nb", "pt_BR", "pt_PT", "ro", "fi", "sv"]
function typingLanguages(value, localeName) {
    var parts = String(value || "").split(" ")
    var codes = []
    for (var i = 0; i < parts.length; ++i)
        if (languageCodes.indexOf(parts[i]) >= 0)
            codes.push(parts[i])
    if (codes.length > 0)
        return codes
    var name = String(localeName)
    var lang = name.split("_")[0]
    if (lang === "no" || lang === "nn")
        lang = "nb"
    else if (lang === "pt")
        lang = "pt_PT"
    var first = languageCodes.indexOf(name) >= 0 ? name : lang
    return [languageCodes.indexOf(first) >= 0 ? first : "en"]
}

// The arrangement used until one is chosen in the settings: the usual one
// for the system locale (Qt.locale().name, e.g. "fi_FI"), as on Jolla's
// layouts and physical keyboards there. Same as layoutFor() in
// settings/languages.js.
function preferred(localeName) {
    var name = String(localeName)
    var lang = name.split("_")[0]
    if (name === "fr_CA")
        return "qwerty"
    if (lang === "de" || name === "fr_CH")
        return "qwertz"
    if (lang === "fr")
        return "azerty"
    if (lang === "fi" || lang === "sv")
        return "nordic"
    return "qwerty"
}
