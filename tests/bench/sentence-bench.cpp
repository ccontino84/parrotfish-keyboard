// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

// Correction and autocorrect in running text (milestone 1/MVP).
//
//   sentence-bench -l data/layouts/it.layout -t tests/data/tatoeba-it.txt
//                  -d build/data/it.pfd [-d OTHER.pfd]... [-n 2000] [-r 0.3]
//                  [-s SEED] [-m LANG] [-B] [-F] [-D] [-R] [-P name=value]... [-v]
//                  [-S slips.log [-M] | -U] [-N PCT] [-X] [-o OOV.txt [-u RATE]]
//
// Words of real sentences are typed one by one with the earlier words as
// context, as on the phone (all dictionaries loaded, equal starting weights,
// completions on, Normal autocorrect). A share -r of the words gets one typo (the kinds of
// typos.h); the rest are typed correctly and must not be changed.
//
// Each sentence is a new text. -m LANG starts it leaning towards LANG, as
// the plugin does for the language the last text was in (×3); pass the
// sentences' language or another one (the user switched languages). -B adds
// the results for the first word of a text, for other words of two typed
// letters and for the rest (with -v: each word's group and confidence); -F
// types only the first words (with the same typos). -D adds two kinds of
// typos: one letter of a double missed, a letter tapped twice (other typos
// change too). -R makes the typos real words instead: another word of the
// first dictionary one adjacent key or one swap away (from → form), for
// Strong (-P strength=3); words with no such neighbour stay clean.
//
// Closer to real typing (2026-10-31): a wrong key goes mostly sideways
// (typos.h); -S reads the directions from a slips.log (one person's hands:
// -M mirrors them), -U picks any adjacent key as before. -N PCT adds a
// number row PCT% as tall as a letter row above the letters (the engine
// gets its keys as on the phone). -X adds typos of two words or the number
// row: the space left out (thisis), the space typed as a key above the
// spacebar (thisbis), and with -N a top-row letter typed as the digit above
// it (h3llo). -o adds unknown words (names, rare words: one per line, those
// no loaded dictionary knows), a share -u (0.1) of the sentences' words,
// typed correctly: autocorrect must leave them alone.
//
// The last line sums up: typos fixed and fixed wrong, correct and unknown
// words changed, and the word error rate before and after autocorrect
// (wrong words / typed words; a word typed with its neighbour as one,
// thisis, counts once).

#include "parrotfish/engine.h"
#include "typos.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace parrotfish;
using namespace bench;

namespace {

struct Stats {
    int total = 0, realWord = 0, top1 = 0, top3 = 0, fixed = 0, wrong = 0;
};

// Where a word stands, for -B.
enum Group { First, Short, Rest, GroupCount };
const char *const groupNames[] = {"word 1", "2 letters", "rest"};

struct GroupStats {
    int clean = 0, cleanWrong = 0, fixable = 0, fixed = 0, wrong = 0;
};

double pct(int a, int b)
{
    return b ? 100.0 * a / b : 0.0;
}

} // namespace

int main(int argc, char **argv)
{
    std::string layoutPath, textPath;
    std::vector<std::string> dictPaths;
    int maxSentences = 2000;
    double rate = 0.3;
    unsigned seed = 1;
    bool verbose = false, breakdown = false, firstOnly = false, doubles = false, realWords = false;
    std::string remembered, slipsPath, oovPath;
    bool uniform = false, mirrored = false, extraKinds = false;
    int numberPct = 0;
    double unknownRate = 0.1;
    EngineParams params;
    params.corrector.completions = true;   // as on the phone
    params.strength = Strength::Normal;    // the phone's default (the engine's is Gentle)
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-l" && i + 1 < argc) layoutPath = argv[++i];
        else if (a == "-t" && i + 1 < argc) textPath = argv[++i];
        else if (a == "-d" && i + 1 < argc) dictPaths.push_back(argv[++i]);
        else if (a == "-n" && i + 1 < argc) maxSentences = std::stoi(argv[++i]);
        else if (a == "-r" && i + 1 < argc) rate = std::stod(argv[++i]);
        else if (a == "-s" && i + 1 < argc) seed = std::stoul(argv[++i]);
        else if (a == "-m" && i + 1 < argc) remembered = argv[++i];
        else if (a == "-B") breakdown = true;
        else if (a == "-F") firstOnly = breakdown = true;
        else if (a == "-D") doubles = true;
        else if (a == "-R") realWords = true;
        else if (a == "-v") verbose = true;
        else if (a == "-S" && i + 1 < argc) slipsPath = argv[++i];
        else if (a == "-U") uniform = true;
        else if (a == "-M") mirrored = true;
        else if (a == "-N" && i + 1 < argc) numberPct = std::stoi(argv[++i]);
        else if (a == "-X") extraKinds = true;
        else if (a == "-o" && i + 1 < argc) oovPath = argv[++i];
        else if (a == "-u" && i + 1 < argc) unknownRate = std::stod(argv[++i]);
        else if (a == "-P" && i + 1 < argc) {
            if (!setParam(params, argv[++i])) {
                std::fprintf(stderr, "unknown parameter %s\n", argv[i]);
                return 2;
            }
        }
    }
    std::ifstream lf(layoutPath);
    std::stringstream ls;
    ls << lf.rdbuf();
    KeyModel keys = KeyModel::parse(ls.str());
    if (numberPct > 0 && !keys.keys().empty()) {
        // Digits over the keys of the top row, in a row numberPct% as tall.
        std::vector<Key> ks = keys.keys();
        float top = ks[0].y;
        for (const Key &k : ks)
            top = std::min(top, k.y);
        std::vector<Key> row;
        for (const Key &k : ks)
            if (k.y == top)
                row.push_back(k);
        std::sort(row.begin(), row.end(), [](const Key &a, const Key &b) { return a.x < b.x; });
        const char32_t digits[] = U"1234567890";
        for (size_t i = 0; i < row.size() && i < 10; ++i) {
            const float h = row[i].height * numberPct / 100.0f;
            ks.push_back({digits[i], row[i].x, top - row[i].height / 2 - h / 2, row[i].width, h});
        }
        keys = KeyModel(ks, keys.sigma());
    }
    Engine engine(params);
    for (const std::string &p : dictPaths) {
        std::string error;
        if (!engine.addLanguage(p, &error)) {
            std::fprintf(stderr, "%s\n", error.c_str());
            return 1;
        }
    }
    std::ifstream tf(textPath);
    if (keys.keys().empty() || engine.languageCount() == 0 || !tf) {
        std::fprintf(stderr, "usage: sentence-bench -l LAYOUT -t SENTENCES -d DICT [-d DICT]... [-n N] [-r RATE]"
                             " [-s SEED] [-m LANG] [-B] [-F] [-D] [-R] [-P name=value]... [-v]\n");
        return 2;
    }
    std::vector<double> prior;
    if (!remembered.empty()) {
        const int k = engine.languageIndex(remembered);
        if (k < 0) {
            std::fprintf(stderr, "-m %s: no such language loaded\n", remembered.c_str());
            return 2;
        }
        prior.assign(engine.languageCount(), 1.0);
        prior[k] *= 3.0;   // as PredictionEngine::setContext
    }
    engine.setLayout(keys);
    Typos typos(keys, engine.dictionary(0), seed);
    typos.setUniform(uniform);
    typos.setMirrored(mirrored);
    if (!slipsPath.empty() && !typos.fitDirections(slipsPath)) {
        std::fprintf(stderr, "-S %s: no wrong keys of this layout\n", slipsPath.c_str());
        return 2;
    }
    // Unknown words: letters only, 3+ of them, in no loaded dictionary.
    std::vector<std::u32string> unknown;
    if (!oovPath.empty()) {
        std::ifstream of(oovPath);
        for (std::string w; std::getline(of, w);) {
            const std::u32string u = utf8::decode(w);
            if (u.size() < 3 || !std::all_of(u.begin(), u.end(), isLetter))
                continue;
            bool known = false;
            for (size_t d = 0; d < engine.languageCount() && !known; ++d)
                known = engine.dictionary(d).find(lower(u)) || engine.dictionary(d).find(u);
            if (!known)
                unknown.push_back(u);
        }
        if (unknown.empty()) {
            std::fprintf(stderr, "-o %s: no unknown words\n", oovPath.c_str());
            return 2;
        }
    }
    // Typo kinds to pick from.
    std::vector<Kind> kinds;
    for (int k = Neighbour; k <= (doubles ? DoubleExtra : Double); ++k)
        kinds.push_back(Kind(k));
    if (extraKinds) {
        kinds.push_back(Joined);
        kinds.push_back(SpaceLetter);
        if (typos.hasDigits())
            kinds.push_back(NumberRow);
    }
    // Keys above the spacebar (phone QWERTY, QWERTZ: c to m).
    const std::u32string aboveSpace = U"cvbnm";
    std::uniform_real_distribution<double> coin(0.0, 1.0);

    Stats stats[KindCount];
    GroupStats groups[GroupCount];
    double totalUs = 0, maxUs = 0;
    int searches = 0, sentences = 0, typedWords = 0, wrongBefore = 0, wrongAfter = 0;
    std::string line;
    while (std::getline(tf, line) && sentences < maxSentences) {
        if (line.empty() || line[0] == '#')
            continue;
        ++sentences;
        std::vector<std::u32string> ws = words(utf8::decode(line));
        std::vector<bool> isUnknown(ws.size(), false);
        if (!unknown.empty()) {
            for (size_t i = 0; i <= ws.size(); ++i)
                if (coin(typos.rng()) < unknownRate) {
                    ws.insert(ws.begin() + i, unknown[typos.pos(unknown.size())]);
                    isUnknown.insert(isUnknown.begin() + i, true);
                    ++i;
                }
        }
        auto isPlain = [](const std::u32string &w) {
            return w.size() >= 2 && std::all_of(w.begin(), w.end(),
                [](char32_t c) { return isLetter(c) || c == U'\''; });
        };
        std::u32string context;
        for (size_t wi = 0; wi < ws.size(); ++wi) {
            std::u32string target = ws[wi];
            const bool plain = isPlain(target);
            if (plain) {
                Kind kind = isUnknown[wi] ? Unknown : Clean;
                std::u32string typed = target;
                std::u32string next;   // the word typed with this one (Joined, SpaceLetter)
                if (realWords && kind == Clean && coin(typos.rng()) < rate) {
                    const Dictionary &d = engine.dictionary(0);
                    std::vector<std::u32string> real;
                    for (const std::u32string &t : typos.oneSlip(target)) {
                        std::u32string stored;   // not names or acronyms (jim → Jim)
                        const uint32_t node = d.find(t, &stored);
                        if (node && stored == t && !(d.attributes(node) & format::AttrMisspelling)
                                && t != lower(target))
                            real.push_back(t);
                    }
                    if (!real.empty()) {
                        kind = RealWord;
                        typed = real[typos.pos(real.size())];
                        if (utf8::isUpper(target[0]))
                            typed[0] = utf8::toUpper(typed[0]);
                    }
                } else if (!realWords && kind == Clean && coin(typos.rng()) < rate) {
                    kind = kinds[typos.pos(kinds.size())];
                    if (kind == Accent && !typos.hasAccents(target))
                        kind = Neighbour;
                    if ((kind == Joined || kind == SpaceLetter)
                            && !(wi + 1 < ws.size() && isPlain(ws[wi + 1]) && !isUnknown[wi + 1]))
                        kind = Neighbour;
                    if (kind == Joined || kind == SpaceLetter) {
                        next = ws[wi + 1];
                        typed = target + (kind == SpaceLetter ? std::u32string(1, aboveSpace[typos.pos(aboveSpace.size())])
                                                              : std::u32string()) + lower(next);
                        target += U" " + next;
                        ++wi;
                    } else if (kind == NumberRow) {
                        typed = typos.numberRowSlip(target);
                        if (typed.empty())
                            typed = typos.apply(target, kind = Neighbour);
                    } else {
                        typed = typos.apply(target, kind);
                    }
                    // Keep the capital of a sentence start, as typed with Shift.
                    if (!typed.empty() && utf8::isUpper(target[0]))
                        typed[0] = utf8::toUpper(typed[0]);
                    if (typed.size() < 2 || lower(typed) == lower(target)) {
                        kind = Clean;
                        typed = target;
                    }
                }
                if (firstOnly && !context.empty()) {
                    context += target + U" ";
                    continue;   // the same typos as without -F
                }
                engine.startText(context, prior);
                const auto t0 = std::chrono::steady_clock::now();
                const Result r = engine.process(tapsFor(typed));
                const double us = std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - t0).count();
                totalUs += us;
                maxUs = std::max(maxUs, us);
                ++searches;

                Stats &s = stats[kind];
                ++s.total;
                const bool fired = r.autocorrect >= 0;
                const std::u32string got = fired ? r.suggestions[r.autocorrect].word : typed;
                const int group = context.empty() ? First : typed.size() <= 2 ? Short : Rest;
                GroupStats &g = groups[group];
                char tag[48] = "";
                if (breakdown)
                    std::snprintf(tag, sizeof tag, " [%s, %.2f]", groupNames[group], r.confidence);
                ++typedWords;
                if (lower(typed) != lower(target)) ++wrongBefore;
                if (lower(got) != lower(target)) ++wrongAfter;
                if (kind == Clean) {
                    ++g.clean;
                } else if (kind != Unknown && !r.typedIsWord) {
                    ++g.fixable;
                    if (fired && lower(got) == lower(target)) ++g.fixed;
                    if (fired && lower(got) != lower(target)) ++g.wrong;
                }
                if (kind == Clean || kind == Unknown) {
                    if (lower(got) != lower(target)) {
                        ++s.wrong;
                        ++g.cleanWrong;
                        if (verbose)
                            std::printf("changed  %s → %s%s%s\n", utf8::encode(typed).c_str(), utf8::encode(got).c_str(),
                                        r.realWordFix ? " (real-word fix)" : "", tag);
                    }
                } else if (kind == RealWord) {
                    if (fired && lower(got) == lower(target)) ++s.fixed;
                    if (fired && lower(got) != lower(target)) ++s.wrong;
                    if (verbose && !fired)
                        std::printf("missed %s → %s top %s\n", utf8::encode(typed).c_str(), utf8::encode(target).c_str(),
                                    r.suggestions.empty() ? "-" : utf8::encode(r.suggestions[0].word).c_str());
                    if (verbose && fired)
                        std::printf("real word %-14s → %-14s got %-14s margin %.2f%s\n",
                                    utf8::encode(typed).c_str(), utf8::encode(target).c_str(),
                                    utf8::encode(got).c_str(), r.margin, tag);
                } else if (r.typedIsWord) {
                    ++s.realWord;
                } else {
                    int rank = -1;
                    for (size_t k = 0; k < r.suggestions.size() && k < 3; ++k)
                        if (lower(r.suggestions[k].word) == lower(target)) { rank = static_cast<int>(k); break; }
                    if (rank == 0) ++s.top1;
                    if (rank >= 0) ++s.top3;
                    if (fired && lower(got) == lower(target)) ++s.fixed;
                    if (fired && lower(got) != lower(target)) ++s.wrong;
                    if (verbose && (rank != 0 || !fired || lower(got) != lower(target)))
                        std::printf("%-13s %-14s → %-14s got %-14s rank %d%s%s\n", kindName(kind),
                                    utf8::encode(typed).c_str(), utf8::encode(target).c_str(),
                                    r.suggestions.empty() ? "-" : utf8::encode(r.suggestions[0].word).c_str(),
                                    rank, fired ? " (autocorrected)" : "", tag);
                }
            }
            context += target + U" ";
        }
    }

    std::printf("%s, %zu languages, %d sentences, typo rate %.2f, seed %u\n", textPath.c_str(),
                engine.languageCount(), sentences, rate, seed);
    std::printf("%-14s %6s %8s %7s %7s %10s %10s\n", "typo", "words", "realword", "top1", "top3",
                "autofixed", "autowrong");
    Stats all;
    for (int k = 0; k < KindCount; ++k) {
        const Stats &s = stats[k];
        if (!s.total) continue;
        if (k == Clean || k == Unknown) {
            std::printf("%-14s %6d %8s %7s %7s %10s %9.2f%%\n", kindName(k), s.total, "", "", "", "",
                        pct(s.wrong, s.total));
            continue;
        }
        const int fixable = s.total - s.realWord;
        std::printf("%-14s %6d %7.1f%% %6.1f%% %6.1f%% %9.1f%% %9.1f%%\n", kindName(k), s.total,
                    pct(s.realWord, s.total), pct(s.top1, fixable), pct(s.top3, fixable),
                    pct(s.fixed, fixable), pct(s.wrong, fixable));
        all.total += s.total; all.realWord += s.realWord; all.top1 += s.top1; all.top3 += s.top3;
        all.fixed += s.fixed; all.wrong += s.wrong;
    }
    const int fixable = all.total - all.realWord;
    std::printf("%-14s %6d %7.1f%% %6.1f%% %6.1f%% %9.1f%% %9.1f%%\n", "all typos", all.total,
                pct(all.realWord, all.total), pct(all.top1, fixable), pct(all.top3, fixable),
                pct(all.fixed, fixable), pct(all.wrong, fixable));
    if (breakdown) {
        std::printf("start lean: %s\n", remembered.empty() ? "none" : remembered.c_str());
        std::printf("%-10s %6s %12s %5s %12s %12s\n", "words", "clean", "changed", "typos",
                    "autofixed", "autowrong");
        for (int k = 0; k < GroupCount; ++k) {
            const GroupStats &g = groups[k];
            std::printf("%-10s %6d %5d %5.2f%% %5d %5d %5.1f%% %5d %5.1f%%\n", groupNames[k], g.clean,
                        g.cleanWrong, pct(g.cleanWrong, g.clean), g.fixable, g.fixed,
                        pct(g.fixed, g.fixable), g.wrong, pct(g.wrong, g.fixable));
        }
    }
    std::printf("time per word: mean %.0f µs, max %.0f µs\n", totalUs / searches, maxUs);
    std::printf("summary: typos fixed %.1f%% wrong %.1f%%, correct changed %.2f%%, unknown changed %.2f%%,"
                " WER %.2f%% -> %.2f%%\n", pct(all.fixed, fixable), pct(all.wrong, fixable),
                pct(stats[Clean].wrong, stats[Clean].total), pct(stats[Unknown].wrong, stats[Unknown].total),
                pct(wrongBefore, typedWords), pct(wrongAfter, typedWords));
    return 0;
}
