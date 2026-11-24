.pragma library
// SPDX-FileCopyrightText: Jolla Ltd.
// SPDX-FileCopyrightText: Oleksii Serdiuk
// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: BSD-3-Clause AND LGPL-2.1-or-later
//
// The key arrangements of the Parrotfish layout, as data. "split" is the
// child index where a split row's right half starts (row 3 counts Shift).
// Keys: c caption, s shifted, y and y2 the two symbol pages, a and A
// accents (not "as": a keyword for Qt 5.6's QML), fit = shrink the caption,
// sticky = the sticky key (stickySigns).
//
// From en.qml (QWERTY), de.qml (QWERTZ) and fr.qml (AZERTY) of
// jolla-keyboard 0.10.4 (Jolla Ltd., BSD-3-Clause, notice below); ЙЦУКЕН
// from ru.qml (Jolla Ltd., as on Sailfish OS 5.1) and uk.qml (Oleksii Serdiuk
// and Jolla Ltd., notice below), Greek from el.qml (Jolla Ltd.). Changes:
// QWERTY: a full-width apostrophe key (the sticky key) ends the middle row,
// whose symbol view is it.qml's with en.qml's `; M's second-page ~ is ×.
// AZERTY: the apostrophe (the sticky key) takes ç's place after N, ç goes to
// C's long press; ' on B and " on C's second page (÷ dropped). All Latin:
// more accents, from Jolla's da/fr/pt/ro and pl/cs/sl layouts. ЙЦУКЕН: the
// sticky key in place of the bottom row's - (ru) or ' (uk), the other
// language's letters on long press (ы: і, і: ы; э є, ъ ї, г ґ, е ё), a key
// switching between the two (a 12th in row 2; ґ then on г's long press).
// Greek: ; types ; also shifted (: on its long press, with ' and ·).

/*
 * Copyright (C) 2012-2016 Jolla ltd and/or its subsidiary(-ies). All rights reserved.
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


/*
 * Copyright (c) 2013 – 2015, Oleksii Serdiuk
 * Copyright (c) 2013 – 2022, Jolla Ltd.
 * All rights reserved.
 *
 * Contact: Pekka Vuorela <pekka.vuorela@jolla.com>
 * Contact: Oleksii Serdiuk <contacts[at]oleksii[dot]name>
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Jolla Ltd. nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


var arrangements = {
    // noSticky: the signs without the sticky key (withoutSticky), as on en.qml.
    "qwerty": { "name": "QWERTY", "noSticky": {"k": ["!", "¡"], "l": ["?", "¿"], "m": [":", "~"]},
                "slots": [{"row": 0, "y": "%", "y2": "‰"}, {"row": 1, "y": "?", "y2": "¿"},
                          {"row": 1, "y": "\"", "y2": "·"}], "rows": [
        { "split": 5, "keys": [
            {"c": "q", "s": "Q", "y": "1", "y2": "€"},
            {"c": "w", "s": "W", "y": "2", "y2": "£"},
            {"c": "e", "s": "E", "y": "3", "y2": "$", "a": "èeéêë€ęě", "A": "ÈEÉÊË€ĘĚ"},
            {"c": "r", "s": "R", "y": "4", "y2": "¥", "a": "rř", "A": "RŘ"},
            {"c": "t", "s": "T", "y": "5", "y2": "₹", "a": "tþțť", "A": "TÞȚŤ"},
            {"c": "y", "s": "Y", "y": "6", "y2": "%", "a": "ýy¥", "A": "ÝY¥"},
            {"c": "u", "s": "U", "y": "7", "y2": "<", "a": "űûùuúüů", "A": "ŰÛÙUÚÜŮ"},
            {"c": "i", "s": "I", "y": "8", "y2": ">", "a": "îïìií", "A": "ÎÏÌIÍ"},
            {"c": "o", "s": "O", "y": "9", "y2": "[", "a": "őøöôòoóõœ", "A": "ŐØÖÔÒOÓÕŒ"},
            {"c": "p", "s": "P", "y": "0", "y2": "]"}
        ] },
        { "split": 5, "keys": [
            {"c": "a", "s": "A", "y": "*", "y2": "`", "a": "aäàâáãåæăą", "A": "AÄÀÂÁÃÅÆĂĄ"},
            {"c": "s", "s": "S", "y": "#", "y2": "^", "a": "sßș$śš", "A": "SẞȘ$ŚŠ"},
            {"c": "d", "s": "D", "y": "+", "y2": "|", "a": "dðďđ", "A": "DÐĎĐ"},
            {"c": "f", "s": "F", "y": "-", "y2": "_"},
            {"c": "g", "s": "G", "y": "=", "y2": "§"},
            {"c": "h", "s": "H", "y": "(", "y2": "{"},
            {"c": "j", "s": "J", "y": ")", "y2": "}"},
            {"c": "k", "s": "K", "y": "~", "y2": "°"},
            {"c": "l", "s": "L", "y": "!", "y2": "¡", "a": "lł", "A": "LŁ"},
            {"c": "'", "s": "'", "y": "?", "y2": "¿", "sticky": true}
        ] },
        { "split": 5, "keys": [
            {"c": "z", "s": "Z", "y": "@", "y2": "«", "a": "zżźž", "A": "ZŻŹŽ"},
            {"c": "x", "s": "X", "y": "&", "y2": "»"},
            {"c": "c", "s": "C", "y": "/", "y2": "\"", "a": "cçćč", "A": "CÇĆČ"},
            {"c": "v", "s": "V", "y": "\\", "y2": "“"},
            {"c": "b", "s": "B", "y": "'", "y2": "”"},
            {"c": "n", "s": "N", "y": ";", "y2": "„", "a": "nñńň", "A": "NÑŃŇ"},
            {"c": "m", "s": "M", "y": ":", "y2": "×"}
        ] }
    ] },
    "qwertz": { "name": "QWERTZ", "stickyInLastRow": true, "rows": [
        { "split": 5, "keys": [
            {"c": "q", "s": "Q", "y": "1", "y2": "€"},
            {"c": "w", "s": "W", "y": "2", "y2": "£"},
            {"c": "e", "s": "E", "y": "3", "y2": "$", "a": "èeéëê€ęě", "A": "ÈEÉÊË€ĘĚ"},
            {"c": "r", "s": "R", "y": "4", "y2": "¥", "a": "rř", "A": "RŘ"},
            {"c": "t", "s": "T", "y": "5", "y2": "₹", "a": "tțť", "A": "TȚŤ"},
            {"c": "z", "s": "Z", "y": "6", "y2": "¤", "a": "zżźž", "A": "ZŻŹŽ"},
            {"c": "u", "s": "U", "y": "7", "y2": "<", "a": "üùuúûů", "A": "ÜÙUÚÛŮ"},
            {"c": "i", "s": "I", "y": "8", "y2": ">", "a": "îïiìí", "A": "ÎÏIÌÍ"},
            {"c": "o", "s": "O", "y": "9", "y2": "[", "a": "öòóôõoøœ", "A": "ÖÒÓÔÕOØŒ"},
            {"c": "p", "s": "P", "y": "0", "y2": "]"},
            {"c": "ü", "s": "Ü", "y": "%", "y2": "‰", "fit": true, "slot": 0}
        ] },
        { "split": 5, "keys": [
            {"c": "a", "s": "A", "y": "*", "y2": "`", "a": "aäàáãâåæăą", "A": "AÄÀÁÃÂÅÆĂĄ"},
            {"c": "s", "s": "S", "y": "#", "y2": "^", "a": "sßşș$śš", "A": "SẞŞȘ$ŚŠ"},
            {"c": "d", "s": "D", "y": "+", "y2": "|", "a": "dðďđ", "A": "DÐĎĐ"},
            {"c": "f", "s": "F", "y": "-", "y2": "_"},
            {"c": "g", "s": "G", "y": "=", "y2": "§", "a": "gğ", "A": "GĞ"},
            {"c": "h", "s": "H", "y": "(", "y2": "{"},
            {"c": "j", "s": "J", "y": ")", "y2": "}"},
            {"c": "k", "s": "K", "y": "\"", "y2": "°"},
            {"c": "l", "s": "L", "y": "~", "y2": "·", "a": "lł", "A": "LŁ"},
            {"c": "ö", "s": "Ö", "y": "!", "y2": "¡", "fit": true, "slot": 1},
            {"c": "ä", "s": "Ä", "y": "?", "y2": "¿", "fit": true, "slot": 2}
        ] },
        { "split": 5, "keys": [
            {"c": "y", "s": "Y", "y": "@", "y2": "«", "a": "ýy¥", "A": "ÝY¥"},
            {"c": "x", "s": "X", "y": "&", "y2": "»"},
            {"c": "c", "s": "C", "y": "/", "y2": "÷", "a": "cçćč", "A": "CÇĆČ"},
            {"c": "v", "s": "V", "y": "\\", "y2": "“"},
            {"c": "b", "s": "B", "y": "'", "y2": "”"},
            {"c": "n", "s": "N", "y": ";", "y2": "„", "a": "nñńň", "A": "NÑŃŇ"},
            {"c": "m", "s": "M", "y": ":", "y2": "×"}
        ] }
    ] },
    // stickyStays: without the sticky key, a plain apostrophe key.
    "azerty": { "name": "AZERTY", "stickyStays": true,
                "slots": [{"row": 0, "y": "%", "y2": "‰"}, {"row": 1, "y": "\"", "y2": "·"},
                          {"row": 2, "y": "…", "y2": "–"}], "rows": [
        { "split": 5, "keys": [
            {"c": "a", "s": "A", "y": "1", "y2": "€", "a": "aâàæäáãåăą", "A": "AÂÀÆÄÁÃÅĂĄ"},
            {"c": "z", "s": "Z", "y": "2", "y2": "£", "a": "zżźž", "A": "ZŻŹŽ"},
            {"c": "e", "s": "E", "y": "3", "y2": "$", "a": "èeéêëęě", "A": "ÈEÉÊËĘĚ"},
            {"c": "r", "s": "R", "y": "4", "y2": "¥", "a": "rř", "A": "RŘ"},
            {"c": "t", "s": "T", "y": "5", "y2": "₹", "a": "tþțť", "A": "TÞȚŤ"},
            {"c": "y", "s": "Y", "y": "6", "y2": "%", "a": "ýy¥", "A": "ÝY¥"},
            {"c": "u", "s": "U", "y": "7", "y2": "<", "a": "üûuùúů", "A": "ÜÛUÙÚŮ"},
            {"c": "i", "s": "I", "y": "8", "y2": ">", "a": "íìîiï", "A": "ÍÌÎIÏ"},
            {"c": "o", "s": "O", "y": "9", "y2": "[", "a": "øöòóôoœõ", "A": "ØÖÒÓÔOŒÕ"},
            {"c": "p", "s": "P", "y": "0", "y2": "]"}
        ] },
        { "split": 5, "keys": [
            {"c": "q", "s": "Q", "y": "*", "y2": "`"},
            {"c": "s", "s": "S", "y": "#", "y2": "^", "a": "sșśš", "A": "SȘŚŠ"},
            {"c": "d", "s": "D", "y": "+", "y2": "|", "a": "dðďđ", "A": "DÐĎĐ"},
            {"c": "f", "s": "F", "y": "-", "y2": "_"},
            {"c": "g", "s": "G", "y": "=", "y2": "§"},
            {"c": "h", "s": "H", "y": "(", "y2": "{"},
            {"c": "j", "s": "J", "y": ")", "y2": "}"},
            {"c": "k", "s": "K", "y": "~", "y2": "°"},
            {"c": "l", "s": "L", "y": "!", "y2": "¡", "a": "lł", "A": "LŁ"},
            {"c": "m", "s": "M", "y": "?", "y2": "¿"}
        ] },
        { "split": 5, "keys": [
            {"c": "w", "s": "W", "y": "@", "y2": "«"},
            {"c": "x", "s": "X", "y": "&", "y2": "»"},
            {"c": "c", "s": "C", "y": "/", "y2": "\"", "a": "cçćč", "A": "CÇĆČ"},
            {"c": "v", "s": "V", "y": "\\", "y2": "“"},
            {"c": "b", "s": "B", "y": "'", "y2": "”"},
            {"c": "n", "s": "N", "y": ";", "y2": "„", "a": "nñńň", "A": "NÑŃŇ"},
            {"c": "'", "s": "'", "y": ":", "y2": "‰", "sticky": true}
        ] }
    ] },
    // Other scripts: Jolla's rows as they are, with the bottom row's extra
    // key (`bottomKey`; not a sticky key: ' is part of Ukrainian words).
    // `languages`: the rows of the typing
    // language's own layout (uk.qml); `switchLabels`: the languages the
    // switch key cycles through, with a name for each (other()).
    "jcuken": { "name": "ЙЦУКЕН", "script": "Cyrl", "bottomKey": "-", "narrowEnds": true, "symbolCaption": "АБВ",
                "switchLabels": {"ru": "РУС", "uk": "УКР"}, "rows": [
        { "split": 6, "keys": [
            {"c": "й", "s": "Й", "y": "1", "y2": "€"},
            {"c": "ц", "s": "Ц", "y": "2", "y2": "£"},
            {"c": "у", "s": "У", "y": "3", "y2": "$", "a": "уў", "A": "УЎ"},
            {"c": "к", "s": "К", "y": "4", "y2": "¥"},
            {"c": "е", "s": "Е", "y": "5", "y2": "₹", "a": "ёе€", "A": "ЁЕ€"},
            {"c": "н", "s": "Н", "y": "6", "y2": "₽"},
            {"c": "г", "s": "Г", "y": "7", "y2": "<", "a": "гґ", "A": "ГҐ"},
            {"c": "ш", "s": "Ш", "y": "8", "y2": ">"},
            {"c": "щ", "s": "Щ", "y": "9", "y2": "["},
            {"c": "з", "s": "З", "y": "0", "y2": "]"},
            {"c": "х", "s": "Х", "y": "№", "y2": "¢"},
            {"c": "ъ", "s": "Ъ", "y": "%", "y2": "‰", "a": "ъї", "A": "ЪЇ", "fit": true}
        ] },
        { "split": 5, "keys": [
            {"c": "ф", "s": "Ф", "y": "*", "y2": "`"},
            {"c": "ы", "s": "Ы", "y": "#", "y2": "√", "a": "ыі", "A": "ЫІ"},
            {"c": "в", "s": "В", "y": "+", "y2": "±"},
            {"c": "а", "s": "А", "y": "×", "y2": "_"},
            {"c": "п", "s": "П", "y": "=", "y2": "≈"},
            {"c": "р", "s": "Р", "y": "(", "y2": "{"},
            {"c": "о", "s": "О", "y": ")", "y2": "}"},
            {"c": "л", "s": "Л", "y": "\"", "y2": "°"},
            {"c": "д", "s": "Д", "y": "~", "y2": "·"},
            {"c": "ж", "s": "Ж", "y": "!", "y2": "¡"},
            {"c": "э", "s": "Э", "y": "?", "y2": "¿", "a": "эє", "A": "ЭЄ"}
        ] },
        { "split": 6, "keys": [
            {"c": "я", "s": "Я", "y": "@", "y2": "«"},
            {"c": "ч", "s": "Ч", "y": "&", "y2": "»"},
            {"c": "с", "s": "С", "y": "/", "y2": "÷"},
            {"c": "м", "s": "М", "y": "\\", "y2": "“"},
            {"c": "и", "s": "И", "y": "'", "y2": "”"},
            {"c": "т", "s": "Т", "y": ";", "y2": "„"},
            {"c": "ь", "s": "Ь", "y": ":", "y2": "©"},
            {"c": "б", "s": "Б", "y": "^", "y2": "®"},
            {"c": "ю", "s": "Ю", "y": "|", "y2": "§"}
        ] }
    ], "languages": { "uk": { "bottomKey": "'", "rows": [
        { "split": 6, "keys": [
            {"c": "й", "s": "Й", "y": "1", "y2": "€"},
            {"c": "ц", "s": "Ц", "y": "2", "y2": "£"},
            {"c": "у", "s": "У", "y": "3", "y2": "$"},
            {"c": "к", "s": "К", "y": "4", "y2": "¥"},
            {"c": "е", "s": "Е", "y": "5", "y2": "₹", "a": "её", "A": "ЕЁ"},
            {"c": "н", "s": "Н", "y": "6", "y2": "₴"},
            {"c": "г", "s": "Г", "y": "7", "y2": "<", "a": "гґ₴", "A": "ГҐ₴"},
            {"c": "ш", "s": "Ш", "y": "8", "y2": ">"},
            {"c": "щ", "s": "Щ", "y": "9", "y2": "["},
            {"c": "з", "s": "З", "y": "0", "y2": "]"},
            {"c": "х", "s": "Х", "y": "№", "y2": "¢"},
            {"c": "ї", "s": "Ї", "y": "%", "y2": "‰", "a": "їъ", "A": "ЇЪ", "fit": true}
        ] },
        { "split": 6, "keys": [
            {"c": "ф", "s": "Ф", "y": "*", "y2": "`"},
            {"c": "і", "s": "І", "y": "#", "y2": "√", "a": "іы", "A": "ІЫ"},
            {"c": "в", "s": "В", "y": "+", "y2": "±"},
            {"c": "а", "s": "А", "y": "×", "y2": "_"},
            {"c": "п", "s": "П", "y": "=", "y2": "≈"},
            {"c": "р", "s": "Р", "y": "(", "y2": "{"},
            {"c": "о", "s": "О", "y": ")", "y2": "}"},
            {"c": "л", "s": "Л", "y": "\"", "y2": "°"},
            {"c": "д", "s": "Д", "y": "~", "y2": "·"},
            {"c": "ж", "s": "Ж", "y": "²", "y2": "³"},
            {"c": "є", "s": "Є", "y": "!", "y2": "¡", "a": "єэ", "A": "ЄЭ"},
            {"c": "ґ", "s": "Ґ", "y": "?", "y2": "¿", "switchSlot": true}
        ] },
        { "split": 6, "keys": [
            {"c": "я", "s": "Я", "y": "@", "y2": "«"},
            {"c": "ч", "s": "Ч", "y": "&", "y2": "»"},
            {"c": "с", "s": "С", "y": "/", "y2": "÷"},
            {"c": "м", "s": "М", "y": "\\", "y2": "“"},
            {"c": "и", "s": "И", "y": "-", "y2": "”"},
            {"c": "т", "s": "Т", "y": ";", "y2": "„"},
            {"c": "ь", "s": "Ь", "y": ":", "y2": "©"},
            {"c": "б", "s": "Б", "y": "^", "y2": "®"},
            {"c": "ю", "s": "Ю", "y": "|", "y2": "§"}
        ] }
    ] } } },
    // d, D: the letter after Jolla's tonos dead key (`dead`).
    "greek": { "name": "Ελληνικά", "script": "Grek", "symbolCaption": "ΑΒΓ", "rows": [
        { "split": 5, "keys": [
            {"c": ";", "s": ";", "y": "1", "y2": "€", "a": ";'·:", "A": ";'·:"},
            {"c": "ς", "s": "ς", "y": "2", "y2": "£"},
            {"c": "ε", "s": "Ε", "y": "3", "y2": "$", "a": "έε€", "A": "ΈΕ€", "d": "΄έ", "D": "΄Έ"},
            {"c": "ρ", "s": "Ρ", "y": "4", "y2": "¥"},
            {"c": "τ", "s": "Τ", "y": "5", "y2": "₹"},
            {"c": "υ", "s": "Υ", "y": "6", "y2": "%", "a": "ύυϋΰ", "A": "ΎΥΫ", "d": "΄ύ", "D": "΄Ύ"},
            {"c": "θ", "s": "Θ", "y": "7", "y2": "<"},
            {"c": "ι", "s": "Ι", "y": "8", "y2": ">", "a": "ίιϊΐ", "A": "ΊΙΪ", "d": "΄ί", "D": "΄Ί"},
            {"c": "ο", "s": "Ο", "y": "9", "y2": "[", "a": "όο", "A": "ΌΟ", "d": "΄ό", "D": "΄Ό"},
            {"c": "π", "s": "Π", "y": "0", "y2": "]"}
        ] },
        { "split": 5, "keys": [
            {"c": "α", "s": "Α", "y": "*", "y2": "`", "a": "αά", "A": "ΑΆ", "d": "΄ά", "D": "΄Ά"},
            {"c": "σ", "s": "Σ", "y": "#", "y2": "^"},
            {"c": "δ", "s": "Δ", "y": "+", "y2": "|"},
            {"c": "φ", "s": "Φ", "y": "-", "y2": "_"},
            {"c": "γ", "s": "Γ", "y": "=", "y2": "§"},
            {"c": "η", "s": "Η", "y": "(", "y2": "{", "a": "ηή", "A": "ΗΉ", "d": "΄ή", "D": "΄Ή"},
            {"c": "ξ", "s": "Ξ", "y": ")", "y2": "}"},
            {"c": "κ", "s": "Κ", "y": "!", "y2": "¡"},
            {"c": "λ", "s": "Λ", "y": "?", "y2": "¿"},
            {"c": "΄", "s": "΄", "dead": true}
        ] },
        { "split": 5, "keys": [
            {"c": "ζ", "s": "Ζ", "y": "@", "y2": "«"},
            {"c": "χ", "s": "Χ", "y": "&", "y2": "»"},
            {"c": "ψ", "s": "Ψ", "y": "/", "y2": "\""},
            {"c": "ω", "s": "Ω", "y": "\\", "y2": "“", "a": "ωώ", "A": "ΩΏ", "d": "΄ώ", "D": "΄Ώ"},
            {"c": "β", "s": "Β", "y": "'", "y2": "”"},
            {"c": "ν", "s": "Ν", "y": ";", "y2": "„"},
            {"c": "μ", "s": "Μ", "y": ":", "y2": "~"}
        ] }
    ] }
}

// What the sticky key can type, in its long-press menu's order.
var stickySigns = "'\"-_@#/"

// The long-press menu of a key typing `sign`: it first, then the sticky
// key's other signs.
function signsAfter(sign) {
    return sign + stickySigns.replace(sign, "")
}

// The sticky key on the `side` of its row. Without an apostrophe key
// (QWERTZ, QWERTY with the language keys) it goes in the bottom letter row,
// typing its sign in the symbol view too. Symbol-view signs stay in place.
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
            var moved = keys.slice()
            moved.unshift(moved.splice(at, 1)[0])
            keys = keepSigns(keys, moved)
            split += 1
        }
        rows.push({"split": split, "keys": keys})
    }
    return {"name": a.name, "rows": rows}
}

// `a` without the sticky key: Jolla's rows (noSticky: caption → [y, y2];
// stickyStays: a plain ' key).
function withoutSticky(a) {
    var rows = []
    for (var r = 0; r < a.rows.length; ++r) {
        var keys = []
        for (var i = 0; i < a.rows[r].keys.length; ++i) {
            var k = a.rows[r].keys[i]
            if (k.sticky && !a.stickyStays)
                continue
            var copy = {}
            for (var p in k)
                if (p !== "sticky")
                    copy[p] = k[p]
            var signs = a.noSticky ? a.noSticky[k.c] : null
            if (signs) {
                copy.y = signs[0]
                copy.y2 = signs[1]
            }
            keys.push(copy)
        }
        rows.push({"split": a.rows[r].split, "keys": keys})
    }
    return {"name": a.name, "rows": rows}
}

// `a` with the language keys (a key's `slot`: its index in the language's
// extra-keys): QWERTZ's are de.qml's ü / ö ä; QWERTY and AZERTY get them at
// their `slots`. Without them, QWERTZ is QWERTY with Z and Y swapped.
function withLanguageKeys(a) {
    if (!a.slots)
        return a
    var rows = []
    for (var r = 0; r < a.rows.length; ++r) {
        var keys = [], sticky = null
        for (var i = 0; i < a.rows[r].keys.length; ++i) {
            var k = a.rows[r].keys[i]
            if (k.sticky && r < a.rows.length - 1)
                continue   // to the last row (placeSticky)
            if (k.sticky)
                sticky = keys.length
            keys.push(k)
        }
        for (i = 0; i < a.slots.length; ++i)
            if (a.slots[i].row === r)
                keys.push({"c": "", "s": "", "y": a.slots[i].y, "y2": a.slots[i].y2, "fit": true, "slot": i})
        // The sticky key stays the outermost; the signs keep their places.
        if (sticky !== null) {
            var outer = keys.slice()
            outer.push(outer.splice(sticky, 1)[0])
            keys = keepSigns(keys, outer)
        }
        rows.push({"split": a.rows[r].split, "keys": keys})
    }
    // A sticky key that was in the middle row goes to the last one.
    var moved = rows.length > 1 && a.rows[1].keys.some(function(k) { return k.sticky })
    return {"name": a.name, "rows": rows, "stickyInLastRow": moved || a.stickyInLastRow === true,
            "stickyStays": a.stickyStays}
}

function withoutLanguageKeys(name, a) {
    if (a.slots)
        return a
    var q = arrangements["qwerty"]
    var rows = []
    for (var r = 0; r < q.rows.length; ++r) {
        var keys = []
        for (var i = 0; i < q.rows[r].keys.length; ++i) {
            var k = q.rows[r].keys[i]
            var swap = k.c === "y" ? a.rows[0].keys[5] : k.c === "z" ? a.rows[2].keys[0] : null
            if (swap) {
                var copy = {}
                for (var p in k)
                    copy[p] = k[p]
                copy.c = swap.c
                copy.s = swap.s
                copy.a = swap.a
                copy.A = swap.A
                k = copy
            }
            keys.push(k)
        }
        rows.push({"split": q.rows[r].split, "keys": keys})
    }
    return {"name": a.name, "noSticky": q.noSticky, "rows": rows}
}

// With the sticky key off, a language key (slot 4) in its place, if a typing
// language has a letter for it (`withLetter`), so that the keys keep their places.
function withLanguageKeyInLastRow(a, withLetter) {
    if (!a.stickyStays && (!a.stickyInLastRow || !withLetter))
        return a
    var rows = a.rows.slice()
    var last = rows.length - 1
    var keys = rows[last].keys.slice()
    var key = {"c": "'", "s": "'", "fit": true, "slot": 4, "fb": "'"}
    var at = -1
    for (var i = 0; i < keys.length; ++i)
        if (keys[i].sticky)
            at = i
    if (at >= 0) {
        key.y = keys[at].y
        key.y2 = keys[at].y2
        keys[at] = key
    } else {
        keys.push(key)
    }
    rows[last] = {"split": rows[last].split, "keys": keys}
    return {"name": a.name, "rows": rows, "stickyInLastRow": false}
}

// The language keys on the `side` of their rows (inside a sticky key there);
// the symbol-view signs stay in place, the digits move with their keys.
function placeLanguageKeys(a, side) {
    if (side !== "left")
        return a
    var toLeft = function(keys, split) {
        var lead = [], slots = [], rest = []
        for (var i = 0; i < keys.length; ++i) {
            var k = keys[i]
            if (i === 0 && k.sticky) {
                lead.push(k)
            } else if (k.slot !== undefined) {
                slots.push(k)
                if (i >= split)
                    split += 1   // the same letters on each half
            } else {
                rest.push(k)
            }
        }
        return {"split": split, "keys": lead.concat(slots, rest)}
    }
    var rows = []
    for (var r = 0; r < a.rows.length; ++r) {
        var row = toLeft(a.rows[r].keys, a.rows[r].split)
        rows.push({"split": row.split, "keys": keepSigns(a.rows[r].keys, row.keys)})
    }
    var result = {"name": a.name, "rows": rows}
    if (a.numbers) {
        var numbers = toLeft(a.numbers, Math.floor(a.numbers.length / 2))
        result.numbers = numbers.keys
        result.numbersSplit = numbers.split
    }
    return result
}

// `keys` in a new order (`moved`), each place keeping its symbol-view signs
// (only if every key has signs).
function keepSigns(keys, moved) {
    for (var i = 0; i < keys.length; ++i)
        if (keys[i].y === undefined)
            return moved
    var result = []
    for (i = 0; i < moved.length; ++i) {
        var copy = {}
        for (var p in moved[i])
            copy[p] = moved[i][p]
        copy.y = keys[i].y
        copy.y2 = keys[i].y2
        result.push(copy)
    }
    return result
}

// The long-press menus for a language key showing `letter`: those of the
// key that has it among its accents (è: E's).
function menusOf(a, letter, letters) {
    for (var r = 0; letter !== "" && r < a.rows.length; ++r)
        for (var i = 0; i < a.rows[r].keys.length; ++i) {
            var k = a.rows[r].keys[i]
            if (k.slot === undefined && k.a && k.a.indexOf(letter) >= 0)
                return accentMenus(k.a, k.A || "", letters, k.c)
        }
    return {"a": "", "A": "", "more": "", "moreShifted": ""}
}

// The script of the arrangement called `name` (ISO 15924).
function scriptOf(name) {
    var a = arrangements[name]
    return a && a.script ? a.script : "Latn"
}

// The languages of `codes` the arrangement called `name` has a switch key
// label for (ЙЦУКЕН: ru, uk), in the order of `codes`.
function switchable(name, codes) {
    var a = arrangements[name], out = []
    for (var i = 0; a && a.switchLabels && i < codes.length; ++i)
        if (a.switchLabels[codes[i]])
            out.push(codes[i])
    return out
}

// `language` if the arrangement called `name` has rows of its own for it, else "".
function variantOf(name, language) {
    var a = arrangements[name]
    return a && a.languages && a.languages[language] ? language : ""
}

// An arrangement of another script: its rows (or `variant`'s), the bottom
// row's extra key (`bottomSign`; none without one: Greek),
// the number row; `switchTo`: the language a key at the end of row 2 shows
// the keys of (ЙЦУКЕН with Russian and Ukrainian), in place of a
// `switchSlot` key (ґ, then on г's long press);
// `narrowEnds`: Shift and Backspace at Jolla's narrow width (ru.qml);
// `symbolCaption`: the symbol key's label back to the letters (АБВ).
function other(a, variant, numberRow, switchTo) {
    var v = variant !== "" && a.languages ? a.languages[variant] : null
    var rows = v ? v.rows : a.rows
    if (switchTo && a.switchLabels && a.switchLabels[switchTo]) {
        var keys = []
        for (var i = 0; i < rows[1].keys.length; ++i)
            if (!rows[1].keys[i].switchSlot)
                keys.push(rows[1].keys[i])
        keys.push({"c": a.switchLabels[switchTo], "s": a.switchLabels[switchTo], "switchTo": switchTo, "fit": true})
        rows = [rows[0], {"split": rows[1].split, "keys": keys}].concat(rows.slice(2))
    }
    var sign = v && v.bottomKey ? v.bottomKey : a.bottomKey
    var b = {"name": a.name, "script": a.script, "rows": rows, "bottomSign": sign,
             "narrowEnds": a.narrowEnds === true,
             "symbolCaption": a.symbolCaption}
    return numberRow ? carry(b, withNumberRow(b)) : b
}

// `to` with the bottom-row fields of `from`.
function carry(from, to) {
    var fields = ["script", "bottomSign", "narrowEnds", "symbolCaption"]
    for (var i = 0; i < fields.length; ++i)
        if (from[fields[i]] !== undefined)
            to[fields[i]] = from[fields[i]]
    return to
}

// The arrangement called `name` (else QWERTY) with the layout's settings;
// `lastRowLetter`: a typing language has a letter for the sticky key's place;
// `variant`: the typing language whose own rows to show (variantOf());
// `switchTo`: other().
function get(name, numberRow, stickySide, stickyOn, languageKeys, languageSide, lastRowLetter, variant, switchTo) {
    var a = arrangements[name] || arrangements["qwerty"]
    if (!arrangements[name])
        name = "qwerty"
    if (scriptOf(name) !== "Latn")
        return other(a, variant || "", numberRow, switchTo || "")
    a = languageKeys ? withLanguageKeys(a) : withoutLanguageKeys(name, a)
    if (languageKeys && stickyOn === false)
        a = withLanguageKeyInLastRow(a, lastRowLetter === true)
    a = stickyOn === false ? withoutSticky(a) : placeSticky(a, stickySide)
    if (numberRow)
        a = withNumberRow(a)
    return languageKeys ? placeLanguageKeys(a, languageSide) : a
}

// Where a row of `count` keys splits in landscape: as many on each half, or
// one more where the arrangement's `split` has it (Jolla's keyboard gives
// all letters the fullest half's width).
function evenSplit(count, split) {
    var low = Math.floor(count / 2), high = Math.ceil(count / 2)
    return Math.abs(high - split) < Math.abs(low - split) ? high : low
}

// The second symbol page's first row with the number row (11 for 11-key rows).
var freeRow = "¹ ² ³ ½ ¼ ¾ ± ≈ – … ≠"

// The symbol view's first page with the numpad: rows 1-2 nine keys (the
// ends padded to the edges), row 3 seven between 1/2 and Backspace, the
// digits next to Backspace; `bottom`, after the spacebar, puts 0 under 8.
// Jolla's signs keep their row where they can.
var numpad = {"rows": ["()~;:123!", "*#+-=456?", "@&/\\789"], "bottom": "'0"}

// Signs of an arrangement's 123 page the numpad has no key for: on the
// long press of a numpad key (key: its menu, the key's own sign first).
var numpadMore = {
    "jcuken": {"#": "#\u2116", "*": "*\u00d7", "+": "+%", "~": "~^", "\\": "\\|", "'": "'\""}
}

// `a` with a number row: the first symbol page's digits, in every view;
// the second page's first row moves to the first page, freeRow takes its place.
function withNumberRow(a) {
    var digits = a.rows[0].keys
    var numbers = []
    for (var i = 0; i < digits.length; ++i) {
        var n = {"c": digits[i].y, "s": digits[i].y, "y": digits[i].y, "y2": digits[i].y}
        if (digits[i].slot === 0) {
            n.slot = 3   // above the top row's language key: one too
            n.fb = digits[i].y
        }
        numbers.push(n)
    }
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

// The arrows of the long-press menus (LongPressMenu.qml).
var more = "\u22ef"   // ⋯, drawn as ›
var back = "\u22ee"   // ⋮, drawn as ‹ (with a key's other letters)

// A key's long-press menus for the typing languages' `letters`: {a, A,
// more, moreShifted}; the other letters go behind `more`.
function accentMenus(a, A, letters, plain) {
    var own = plain ? plain.toLowerCase() : ""
    var split = function(accents) {
        var keep = "", rest = ""
        for (var i = 0; i < accents.length; ++i) {
            var c = accents.charAt(i), lower = c.toLowerCase()
            if (lower === c.toUpperCase() || (lower >= "a" && lower <= "z") || lower === own
                    || letters.indexOf(lower) >= 0)
                keep += c   // a sign, the plain letter or a typing language's letter
            else
                rest += c
        }
        return {"keep": keep, "rest": rest}
    }
    var s = split(a), S = split(A)
    if (s.rest === "")
        return {"a": a, "A": A, "more": "", "moreShifted": ""}
    return {"a": s.keep + more, "A": S.keep + more, "more": s.rest, "moreShifted": S.rest}
}
