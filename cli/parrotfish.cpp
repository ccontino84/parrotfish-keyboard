// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// parrotfish: shows what the keyboard is learning, read-only from
// ~/.local/share/parrotfish-keyboard/learning.db (plugin/src/
// learningstore.cpp writes it). The keyboard saves when it closes: run this
// with the keyboard closed to see everything.
//   candidates  new words not learned yet, and how close they are
//   learned     learned new words, and how long until they fade away
#include <sqlite3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "parrotfish/learned.h"

namespace {

const char *usage =
    "Usage: parrotfish COMMAND [-l LANGUAGE] [--db FILE]\n"
    "\n"
    "Commands:\n"
    "  candidates     new words not learned yet: texts so far, uses, words left\n"
    "  learned        learned new words: count, and when they fade away\n"
    "  learned --all  also the dictionary words whose use was learned\n"
    "\n"
    "Reads ~/.local/share/parrotfish-keyboard/learning.db; the keyboard saves\n"
    "it when it closes.\n";

struct Lang {
    double typed = 0, nextFade = 0;
};

struct Word {
    std::string lang, spelling;
    double count, firstSeen, lastUsed;
    bool isNew, pending;
    int texts;
};

std::string column(sqlite3_stmt *st, int i)
{
    const unsigned char *t = sqlite3_column_text(st, i);
    return t ? reinterpret_cast<const char *>(t) : "";
}

// Display width of a UTF-8 string (one column per code point).
size_t width(const std::string &s)
{
    size_t n = 0;
    for (unsigned char c : s)
        n += (c & 0xC0) != 0x80;
    return n;
}

std::string pad(const std::string &s, size_t w)
{
    const size_t n = width(s);
    return n >= w ? s + " " : s + std::string(w - n, ' ');
}

std::string words(double n)
{
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.0f word%s", n, std::lround(n) == 1 ? "" : "s");
    return buf;
}

} // namespace

int main(int argc, char **argv)
{
    std::string command, only, db;
    bool all = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if ((a == "-l" || a == "--language") && i + 1 < argc)
            only = argv[++i];
        else if (a == "--db" && i + 1 < argc)
            db = argv[++i];
        else if (a == "--all" || a == "-a")
            all = true;
        else if (a == "-h" || a == "--help" || a == "help") {
            std::fputs(usage, stdout);
            return 0;
        } else if (command.empty() && a[0] != '-')
            command = a;
        else {
            std::fprintf(stderr, "parrotfish: unknown argument %s\n\n%s", a.c_str(), usage);
            return 2;
        }
    }
    if (command != "candidates" && command != "learned") {
        std::fputs(usage, command.empty() ? stdout : stderr);
        return command.empty() ? 0 : 2;
    }
    if (db.empty()) {
        const char *home = std::getenv("HOME");
        db = std::string(home ? home : "") + "/.local/share/parrotfish-keyboard/learning.db";
    }

    sqlite3 *h = nullptr;
    if (sqlite3_open_v2(db.c_str(), &h, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        std::fprintf(stderr, "parrotfish: cannot open %s (nothing learned yet?)\n", db.c_str());
        sqlite3_close(h);
        return 1;
    }
    sqlite3_busy_timeout(h, 2000);

    std::map<std::string, Lang> langs;
    std::vector<Word> list;
    sqlite3_stmt *st = nullptr;
    if (sqlite3_prepare_v2(h, "SELECT lang, typed, next_fade FROM langs", -1, &st, nullptr) == SQLITE_OK)
        while (sqlite3_step(st) == SQLITE_ROW)
            langs[column(st, 0)] = {sqlite3_column_double(st, 1), sqlite3_column_double(st, 2)};
    sqlite3_finalize(st);
    st = nullptr;
    if (sqlite3_prepare_v2(h, "SELECT lang, spelling, count, first_seen, last_used, is_new, pending, texts, key FROM words",
                           -1, &st, nullptr) != SQLITE_OK) {
        std::fprintf(stderr, "parrotfish: %s: %s\n", db.c_str(), sqlite3_errmsg(h));
        sqlite3_close(h);
        return 1;
    }
    while (sqlite3_step(st) == SQLITE_ROW) {
        Word w{column(st, 0), column(st, 1), sqlite3_column_double(st, 2), sqlite3_column_double(st, 3),
               sqlite3_column_double(st, 4), sqlite3_column_int(st, 5) != 0, sqlite3_column_int(st, 6) != 0,
               sqlite3_column_int(st, 7)};
        if (w.spelling.empty())
            w.spelling = column(st, 8);
        if (!only.empty() && w.lang != only)
            continue;
        const bool wanted = command == "candidates" ? w.pending : !w.pending && (w.isNew || all);
        if (wanted)
            list.push_back(w);
    }
    sqlite3_finalize(st);
    sqlite3_close(h);

    // The keyboard's own rules (it uses the defaults).
    const parrotfish::LearnParams p;
    if (command == "candidates")
        std::printf("A word missing from the dictionaries is learned once kept in %d different\n"
                    "texts within %.0f words typed in its language.\n",
                    p.newWordTexts, p.newWordWindow);
    else
        std::printf("Each use adds 1. Every %.0f words typed in a language, its counts are\n"
                    "multiplied by %g; a word below %g is forgotten.\n",
                    p.fadeEvery, p.fade, p.dropBelow);

    std::sort(list.begin(), list.end(), [&](const Word &a, const Word &b) {
        if (a.lang != b.lang)
            return a.lang < b.lang;
        if (command == "candidates" && a.texts != b.texts)
            return a.texts > b.texts;
        if (a.count != b.count)
            return a.count > b.count;
        return a.spelling < b.spelling;
    });
    size_t wordWidth = 4;
    for (const Word &w : list)
        wordWidth = std::max(wordWidth, width(w.spelling));
    wordWidth += 2;   // a space, and "*" after a dictionary word

    std::string current;
    for (const Word &w : list) {
        const Lang &l = langs[w.lang];
        if (w.lang != current) {
            current = w.lang;
            std::printf("\n%s: %s typed, next fading in %s\n", w.lang.c_str(), words(l.typed).c_str(),
                        words(std::max(0.0, l.nextFade - l.typed)).c_str());
            if (command == "candidates")
                std::printf("  %stexts  uses  left to reach %d texts\n", pad("word", wordWidth).c_str(),
                            p.newWordTexts);
            else
                std::printf("  %scount  last used        forgotten if not used again\n",
                            pad("word", wordWidth).c_str());
        }
        char buf[160];
        if (command == "candidates") {
            const double left = w.firstSeen + p.newWordWindow - l.typed;
            std::snprintf(buf, sizeof buf, "%d/%d", w.texts, p.newWordTexts);
            std::printf("  %s%s%s%s\n", pad(w.spelling, wordWidth).c_str(), pad(buf, 7).c_str(),
                        pad(std::to_string(std::lround(w.count)), 6).c_str(),
                        left > 0 ? words(left).c_str() : "none: starts over if typed again");
        } else {
            // Fadings until the count drops below dropBelow, the first one
            // at nextFade.
            int fadings = 0;
            for (double c = w.count; c >= p.dropBelow; c *= p.fade)
                ++fadings;
            const double after = std::max(0.0, l.nextFade - l.typed) + (fadings - 1) * p.fadeEvery;
            char count[32], used[48], gone[96];
            std::snprintf(count, sizeof count, "%.1f", w.count);
            std::snprintf(used, sizeof used, "%s ago", words(std::max(0.0, l.typed - w.lastUsed)).c_str());
            std::snprintf(gone, sizeof gone, "after %d fading%s, in %s", fadings, fadings == 1 ? "" : "s",
                          words(after).c_str());
            std::printf("  %s%s%s%s\n", pad(w.spelling + (w.isNew ? "" : "*"), wordWidth).c_str(),
                        pad(count, 7).c_str(), pad(used, 17).c_str(), gone);
        }
    }
    if (list.empty())
        std::printf("\nNone%s.\n", only.empty() ? "" : (" in " + only).c_str());
    else if (command == "learned" && all)
        std::printf("\n* a dictionary word: forgetting it only drops what was learned about it.\n");
    return 0;
}
