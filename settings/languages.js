.pragma library
// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// The typing languages Parrotfish has dictionaries for (data/CMakeLists.txt),
// for the settings. `experimental`: too little text to build and test its
// data as well as the others' (Common Voice and Tatoeba). The dconf key /apps/parrotfish-keyboard/languages holds
// the chosen codes separated by spaces; empty means the default below, the
// same as the plugin's (PredictionEngine::loadLanguages).

var all = [
    { "code": "da", "name": "Dansk", "experimental": true },
    { "code": "de", "name": "Deutsch" },
    { "code": "en", "name": "English" },
    { "code": "es", "name": "Español" },
    { "code": "fr", "name": "Français" },
    { "code": "it", "name": "Italiano" },
    { "code": "nl", "name": "Nederlands" },
    { "code": "nb", "name": "Norsk bokmål", "experimental": true },
    { "code": "pt_BR", "name": "Português (Brasil)" },
    { "code": "pt_PT", "name": "Português (Portugal)" },
    { "code": "ro", "name": "Română", "experimental": true },
    { "code": "fi", "name": "Suomi", "experimental": true },
    { "code": "sv", "name": "Svenska", "experimental": true }
]

// Every enabled language is searched on each keystroke (about 6 ms each
// at worst on a phone, estimated from 3 languages), and close languages
// compete for the same words. Beyond this many, the page says so.
var comfortable = 4

function known(code) {
    for (var i = 0; i < all.length; ++i) {
        if (all[i].code === code)
            return true
    }
    return false
}

// The chosen codes, in the order of `all`. `value` is the dconf string,
// `localeName` the system locale (Qt.locale().name, e.g. "it_IT").
function selected(value, localeName) {
    var parts = String(value || "").split(" ")
    var codes = []
    for (var i = 0; i < all.length; ++i) {
        if (parts.indexOf(all[i].code) >= 0)
            codes.push(all[i].code)
    }
    if (codes.length > 0)
        return codes
    // Default: the system language only (pt_BR, or its language: it_IT ->
    // it); English if there is no dictionary for it.
    var lang = String(localeName).split("_")[0]
    if (lang === "no" || lang === "nn")
        lang = "nb"
    else if (lang === "pt")
        lang = "pt_PT"   // pt_AO, pt_MZ...: the Portugal spelling
    var first = known(localeName) ? localeName : lang
    return [known(first) ? first : "en"]
}

// The layout used until one is chosen: the usual one for the system
// locale. Same as preferred() in layouts/parrotfish/arrangements.js.
function layoutFor(localeName) {
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

function names(codes) {
    var out = []
    for (var i = 0; i < all.length; ++i) {
        if (codes.indexOf(all[i].code) >= 0)
            out.push(all[i].name)
    }
    return out.join(", ")
}

// `codes` with `code` switched on or off, as the dconf string.
function toggled(codes, code) {
    var out = []
    for (var i = 0; i < all.length; ++i) {
        var on = codes.indexOf(all[i].code) >= 0
        if (all[i].code === code)
            on = !on
        if (on)
            out.push(all[i].code)
    }
    return out.join(" ")
}
