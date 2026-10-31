// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

// Next-word suggestions and completions in running text (word-pair data).
//
//   nextword-bench -l data/layouts/phone-qwerty.layout -t tests/data/tatoeba-en.txt
//                  -d build/data/en.pfd [-d OTHER.pfd]... [-n 2000] [-m LANG]
//                  [-P name=value]... [-v]
//
// Each line is a new text (lines starting with # are skipped), with all
// dictionaries loaded and Normal strength, as sentence-bench. -m LANG starts
// it leaning towards LANG, as the plugin does for the last text's language.
//
// Next word: before each word, with the line up to it as context (the first
// word from the sentence start), the engine's next-word suggestions; the
// word is a top-1 or top-3 hit if it is the first or among the first three
// (case aside). Counted for all words, the first word of a line and the
// rest; "unknown" is the share of words no loaded dictionary knows (never
// suggested).
//
// Keystrokes saved: each word is typed letter by letter, taps right on the
// keys, completions on. As soon as it is among the first three suggestions
// (the next words before the first letter, then those of the letters typed),
// it is picked with one tap, which also types the space. KSR = 1 - taps /
// (letters + 1 space per word). Shift, punctuation and numbers aren't counted.
//
// -v prints each word missed by the next-word top 3, with the suggestions
// and the letters typed before it was picked. The last line sums up.

#include "parrotfish/engine.h"
#include "typos.h"   // lower, isLetter

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace parrotfish;
using namespace bench;

namespace {

struct Stats {
    int words = 0, top1 = 0, top3 = 0;
};

enum Group { First, Rest, GroupCount };
const char *const groupNames[] = {"word 1", "rest"};

double pct(int a, int b)
{
    return b ? 100.0 * a / b : 0.0;
}

// The rank of `word` among the first three suggestions, case aside, or -1.
int rank3(const std::vector<Suggestion> &s, const std::u32string &word)
{
    const std::u32string w = lower(word);
    for (size_t k = 0; k < s.size() && k < 3; ++k)
        if (lower(s[k].word) == w)
            return static_cast<int>(k);
    return -1;
}

std::string top3(const std::vector<Suggestion> &s)
{
    std::string out;
    for (size_t k = 0; k < s.size() && k < 3; ++k)
        out += (k ? " " : "") + utf8::encode(s[k].word);
    return out.empty() ? "-" : out;
}

} // namespace

int main(int argc, char **argv)
{
    std::string layoutPath, textPath, remembered;
    std::vector<std::string> dictPaths;
    int maxLines = 2000;
    bool verbose = false;
    EngineParams params;
    params.corrector.completions = true;   // as on the phone
    params.strength = Strength::Normal;    // the phone's default
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-l" && i + 1 < argc) layoutPath = argv[++i];
        else if (a == "-t" && i + 1 < argc) textPath = argv[++i];
        else if (a == "-d" && i + 1 < argc) dictPaths.push_back(argv[++i]);
        else if (a == "-n" && i + 1 < argc) maxLines = std::stoi(argv[++i]);
        else if (a == "-m" && i + 1 < argc) remembered = argv[++i];
        else if (a == "-v") verbose = true;
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
    const KeyModel keys = KeyModel::parse(ls.str());
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
        std::fprintf(stderr, "usage: nextword-bench -l LAYOUT -t TEXT -d DICT [-d DICT]... [-n N] [-m LANG]"
                             " [-P name=value]... [-v]\n");
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

    Stats groups[GroupCount];
    int lines = 0, unknown = 0, picked = 0, pickedNext = 0;
    long letters = 0, taps = 0;
    std::string line;
    while (std::getline(tf, line) && lines < maxLines) {
        if (line.empty() || line[0] == '#')
            continue;
        ++lines;
        // The line with the apostrophes words() uses, so that each word is
        // found in it and the context keeps the punctuation (a new sentence).
        std::u32string text = utf8::decode(line);
        for (char32_t &c : text)
            if (c == 0x2019 || c == 0x02BC)
                c = U'\'';
        const std::vector<std::u32string> ws = words(text);
        size_t pos = 0;
        for (size_t wi = 0; wi < ws.size(); ++wi) {
            const std::u32string &target = ws[wi];
            const size_t at = text.find(target, pos);
            if (at == std::u32string::npos)
                break;   // can't happen: words() takes its words from text
            pos = at + target.size();
            if (std::none_of(target.begin(), target.end(), [](char32_t c) { return isLetter(c); }))
                continue;   // numbers
            std::u32string context = text.substr(0, at);
            if (!context.empty() && (context.back() == U'\'' || context.back() == U'-'))
                context += U' ';   // not inside a word
            engine.startText(context, prior);

            Stats &g = groups[wi == 0 ? First : Rest];
            ++g.words;
            const std::vector<Suggestion> next = engine.predict(context, 3);
            const int r = rank3(next, target);
            if (r == 0) ++g.top1;
            if (r >= 0) ++g.top3;
            bool known = false;
            for (size_t d = 0; d < engine.languageCount() && !known; ++d)
                known = engine.dictionary(d).find(lower(target)) || engine.dictionary(d).find(target);
            if (!known) ++unknown;

            // Letters typed until the word is among the first three.
            size_t typed = 0;
            if (r < 0) {
                for (typed = 1; typed < target.size(); ++typed)
                    if (rank3(engine.process(tapsFor(target.substr(0, typed))).suggestions, target) >= 0)
                        break;
            }
            letters += target.size() + 1;
            taps += typed + 1;
            if (typed < target.size()) {
                ++picked;
                if (typed == 0) ++pickedNext;
            }
            if (verbose && r < 0) {
                const std::vector<std::u32string> before = words(context);
                std::printf("miss %-14s → %-14s next %-30s picked after %zu/%zu%s\n",
                            before.empty() ? "^" : utf8::encode(before.back()).c_str(),
                            utf8::encode(target).c_str(), top3(next).c_str(), typed, target.size(),
                            known ? "" : " (unknown)");
            }
        }
    }

    Stats all;
    for (const Stats &g : groups) {
        all.words += g.words; all.top1 += g.top1; all.top3 += g.top3;
    }
    std::printf("%s, %zu languages, %d lines, start lean %s\n", textPath.c_str(), engine.languageCount(),
                lines, remembered.empty() ? "none" : remembered.c_str());
    std::printf("%-10s %6s %7s %7s\n", "next word", "words", "top1", "top3");
    for (int k = 0; k < GroupCount; ++k)
        std::printf("%-10s %6d %6.1f%% %6.1f%%\n", groupNames[k], groups[k].words,
                    pct(groups[k].top1, groups[k].words), pct(groups[k].top3, groups[k].words));
    std::printf("%-10s %6d %6.1f%% %6.1f%%\n", "all", all.words, pct(all.top1, all.words),
                pct(all.top3, all.words));
    std::printf("unknown words %.1f%%; picked %.1f%% of words (%.1f%% before the first letter);"
                " taps %ld of %ld\n", pct(unknown, all.words), pct(picked, all.words),
                pct(pickedNext, all.words), taps, letters);
    std::printf("summary: next word top1 %.1f%% top3 %.1f%%, after word 1 top1 %.1f%% top3 %.1f%%,"
                " KSR %.1f%%, unknown %.1f%%\n", pct(all.top1, all.words), pct(all.top3, all.words),
                pct(groups[Rest].top1, groups[Rest].words), pct(groups[Rest].top3, groups[Rest].words),
                letters ? 100.0 * (1.0 - double(taps) / letters) : 0.0, pct(unknown, all.words));
    return 0;
}
