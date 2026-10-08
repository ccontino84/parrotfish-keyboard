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
// the caption to the key's width.
//
// The keys come from en.qml (QWERTY) and de.qml (QWERTZ) of jolla-keyboard
// 0.10.4 (Jolla Ltd., BSD-3-Clause, notice below). QWERTY changes: a
// full-width apostrophe key ends the middle row (no half-key indent, as on
// Jolla's it.qml), B shows " in the symbol view, the middle row of the
// symbol view is it.qml's but keeps en.qml's ` (~ before ! and ?, ° on the
// second page), and M's second-page ~ became × (as on de.qml). QWERTZ:
// de.qml's keys unchanged.

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
            {"c": "t", "s": "T", "y": "5", "y2": "₹", "a": "tþ", "A": "TÞ"},
            {"c": "y", "s": "Y", "y": "6", "y2": "%", "a": "ýy¥", "A": "ÝY¥"},
            {"c": "u", "s": "U", "y": "7", "y2": "<", "a": "űûùuúü", "A": "ŰÛÙUÚÜ"},
            {"c": "i", "s": "I", "y": "8", "y2": ">", "a": "îïìií", "A": "ÎÏÌIÍ"},
            {"c": "o", "s": "O", "y": "9", "y2": "[", "a": "őøöôòoó", "A": "ŐØÖÔÒOÓ"},
            {"c": "p", "s": "P", "y": "0", "y2": "]"}
        ] },
        { "split": 5, "keys": [
            {"c": "a", "s": "A", "y": "*", "y2": "`", "a": "aäàâáãå", "A": "AÄÀÂÁÃÅ"},
            {"c": "s", "s": "S", "y": "#", "y2": "^", "a": "sß$", "A": "Sẞ$"},
            {"c": "d", "s": "D", "y": "+", "y2": "|", "a": "dð", "A": "DÐ"},
            {"c": "f", "s": "F", "y": "-", "y2": "_"},
            {"c": "g", "s": "G", "y": "=", "y2": "§"},
            {"c": "h", "s": "H", "y": "(", "y2": "{"},
            {"c": "j", "s": "J", "y": ")", "y2": "}"},
            {"c": "k", "s": "K", "y": "~", "y2": "°"},
            {"c": "l", "s": "L", "y": "!", "y2": "¡"},
            {"c": "'", "s": "'", "y": "?", "y2": "¿"}
        ] },
        { "split": 5, "keys": [
            {"c": "z", "s": "Z", "y": "@", "y2": "«"},
            {"c": "x", "s": "X", "y": "&", "y2": "»"},
            {"c": "c", "s": "C", "y": "/", "y2": "\"", "a": "cç", "A": "CÇ"},
            {"c": "v", "s": "V", "y": "\\", "y2": "“"},
            {"c": "b", "s": "B", "y": "\"", "y2": "”"},
            {"c": "n", "s": "N", "y": ";", "y2": "„", "a": "nñ", "A": "NÑ"},
            {"c": "m", "s": "M", "y": ":", "y2": "×"}
        ] }
    ] },
    "qwertz": { "name": "QWERTZ", "rows": [
        { "split": 5, "keys": [
            {"c": "q", "s": "Q", "y": "1", "y2": "€"},
            {"c": "w", "s": "W", "y": "2", "y2": "£"},
            {"c": "e", "s": "E", "y": "3", "y2": "$", "a": "èeéëê€", "A": "ÈEÉÊË€"},
            {"c": "r", "s": "R", "y": "4", "y2": "¥"},
            {"c": "t", "s": "T", "y": "5", "y2": "₹"},
            {"c": "z", "s": "Z", "y": "6", "y2": "¤"},
            {"c": "u", "s": "U", "y": "7", "y2": "<", "a": "üùuúû", "A": "ÜÙUÚÛ"},
            {"c": "i", "s": "I", "y": "8", "y2": ">", "a": "îïiìí", "A": "ÎÏIÌÍ"},
            {"c": "o", "s": "O", "y": "9", "y2": "[", "a": "öòóôõoø", "A": "ÖÒÓÔÕOØ"},
            {"c": "p", "s": "P", "y": "0", "y2": "]"},
            {"c": "ü", "s": "Ü", "y": "%", "y2": "‰", "fit": true}
        ] },
        { "split": 5, "keys": [
            {"c": "a", "s": "A", "y": "*", "y2": "`", "a": "aäàáãâåæ", "A": "AÄÀÁÃÂÅÆ"},
            {"c": "s", "s": "S", "y": "#", "y2": "^", "a": "sßş$", "A": "SẞŞ$"},
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
    ] }
}

// The arrangement called `name`, or QWERTY.
function get(name) {
    return arrangements[name] || arrangements["qwerty"]
}
