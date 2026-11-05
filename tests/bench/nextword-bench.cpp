// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

// Next-word suggestions and completions in running text (word-pair data).
//
//   nextword-bench -l data/layouts/phone-qwerty.layout -t tests/data/tatoeba-en.txt
//                  -d build/data/en.pfd [-d OTHER.pfd]... [-n 2000] [-m LANG]
//                  [-P name=value]... [-v] [-Y SETTINGS]... [-k START] [-Z | -J]
//                  [-Q [-q name=value]... [-H LINES]] [-V LINES]
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
//
// -Q learns as one user (Engine::learn, as the plugin): each word is kept
// once typed; -H LINES lines make one text (1), -q sets LearnParams. -V
// LINES: the first LINES lines are only typed (and learned), not counted,
// with or without -Q (the same words measured either way).
//
// -Y name=value[,...] (repeatable) replays the decision with other
// settings (Engine::decide on each search; isDecisionParam() only): the
// keystrokes depend on it a little (the autocorrection comes first in the
// bar). Each adds a summary[SETTINGS]: line. -k START leaves out the first
// START lines (with -n END: lines START+1..END, a shard of the text); -Z
// adds counts lines, which -J (reading them on stdin, no other option)
// adds up into the summary lines of the whole text (tools/pf shards).

#include "parrotfish/engine.h"
#include "typos.h"   // lower, isLetter

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
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

// The counts behind the summary: next words (word 1, rest), unknown words,
// and per decision setting the words picked and the taps.
struct Counts {
    int words[2] = {0, 0}, top1[2] = {0, 0}, top3[2] = {0, 0};
    long unknown = 0, letters = 0, picked = 0, pickedNext = 0, taps = 0;
    std::vector<long> values() const
    {
        return {words[0], words[1], top1[0], top1[1], top3[0], top3[1], unknown, letters, picked, pickedNext, taps};
    }
    void add(const std::vector<long> &v)
    {
        for (int g = 0; g < 2; ++g) {
            words[g] += static_cast<int>(v[g]);
            top1[g] += static_cast<int>(v[2 + g]);
            top3[g] += static_cast<int>(v[4 + g]);
        }
        unknown += v[6]; letters += v[7]; picked += v[8]; pickedNext += v[9]; taps += v[10];
    }
};

std::string summaryText(const Counts &c)
{
    const int words = c.words[0] + c.words[1], top1 = c.top1[0] + c.top1[1], top3 = c.top3[0] + c.top3[1];
    char buf[400];
    std::snprintf(buf, sizeof buf, "next word top1 %.1f%% top3 %.1f%%, after word 1 top1 %.1f%% top3 %.1f%%,"
                  " KSR %.1f%%, unknown %.1f%%", pct(top1, words), pct(top3, words), pct(c.top1[1], c.words[1]),
                  pct(c.top3[1], c.words[1]), c.letters ? 100.0 * (1.0 - double(c.taps) / c.letters) : 0.0,
                  pct(static_cast<int>(c.unknown), words));
    return buf;
}

// -J: the counts lines of shards on stdin, added up per setting.
int joinCounts()
{
    std::vector<std::string> order;
    std::map<std::string, Counts> sums;
    for (std::string line; std::getline(std::cin, line);) {
        if (line.compare(0, 6, "counts") != 0)
            continue;
        const size_t colon = line.find(": ");
        const std::string key = line.substr(6, colon - 6);
        std::istringstream in(line.substr(colon + 2));
        std::vector<long> v;
        for (long x; in >> x;)
            v.push_back(x);
        if (v.size() != 11)
            return 2;
        if (!sums.count(key))
            order.push_back(key);
        sums[key].add(v);
    }
    for (const std::string &key : order)
        if (!key.empty())
            std::printf("summary%s: %s\n", key.c_str(), summaryText(sums[key]).c_str());
    if (sums.count(""))
        std::printf("summary: %s\n", summaryText(sums[""]).c_str());
    return 0;
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
    int maxLines = 2000, skipLines = 0;
    bool verbose = false, printCounts = false;
    std::vector<std::string> variantSpecs;
    bool learn = false;
    int textLines = 1, warmLines = 0;
    LearnParams learnParams;
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
        else if (a == "-Y" && i + 1 < argc) variantSpecs.push_back(argv[++i]);
        else if (a == "-k" && i + 1 < argc) skipLines = std::stoi(argv[++i]);
        else if (a == "-Q") learn = true;
        else if (a == "-H" && i + 1 < argc) textLines = std::max(1, std::stoi(argv[++i]));
        else if (a == "-V" && i + 1 < argc) warmLines = std::stoi(argv[++i]);
        else if (a == "-q" && i + 1 < argc) {
            if (!setParam(learnParams, argv[++i])) {
                std::fprintf(stderr, "unknown learning parameter %s\n", argv[i]);
                return 2;
            }
        }
        else if (a == "-Z") printCounts = true;
        else if (a == "-J") return joinCounts();
        else if (a == "-P" && i + 1 < argc) {
            if (!setParam(params, argv[++i])) {
                std::fprintf(stderr, "unknown parameter %s\n", argv[i]);
                return 2;
            }
        }
    }
    std::vector<EngineParams> variants;
    for (const std::string &spec : variantSpecs) {
        EngineParams v = params;
        std::istringstream in(spec);
        for (std::string a; std::getline(in, a, ',');) {
            const std::string name = a.substr(0, a.find('='));
            if (!isDecisionParam(name) || !setParam(v, a)) {
                std::fprintf(stderr, "-Y %s: %s is not a decision parameter (isDecisionParam)\n", spec.c_str(),
                             name.c_str());
                return 2;
            }
        }
        if ((v.strength == Strength::Strong) != (params.strength == Strength::Strong)) {
            std::fprintf(stderr, "-Y %s: Strong searches differently, it can't be replayed\n", spec.c_str());
            return 2;
        }
        variants.push_back(v);
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
    Learned learned(learnParams);
    if (learn)
        engine.setLearned(&learned);

    Stats groups[GroupCount];
    int lines = 0, unknown = 0, picked = 0, pickedNext = 0;
    long letters = 0, taps = 0;
    std::vector<long> vTaps(variants.size(), 0), vPicked(variants.size(), 0), vPickedNext(variants.size(), 0);
    std::string line;
    while (std::getline(tf, line) && lines < maxLines) {
        if (line.empty() || line[0] == '#')
            continue;
        ++lines;
        if (lines <= skipLines)
            continue;   // -k: another shard's
        if (learn && lines > 1 && (lines - 1) % textLines == 0) {
            learned.endText();
            learned.maintain();   // as the plugin when the keyboard closes
        }
        const bool measure = lines > warmLines;
        // The line with the apostrophes words() uses, so that each word is
        // found in it and the context keeps the punctuation (a new sentence).
        std::u32string text = utf8::decode(line);
        for (char32_t &c : text)
            if (c == 0x2019 || c == 0x02BC)
                c = U'\'';
        // A final apostrophe stays where it is part of the word (po'):
        // bench::typedWords().
        const std::vector<std::u32string> ws = typedWords(text, engine);
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
            if (!measure) {
                if (learn)
                    engine.learn(learned, context, target);
                continue;
            }

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
            std::vector<size_t> vTyped(variants.size(), 0);
            if (r < 0 && variants.empty()) {
                for (typed = 1; typed < target.size(); ++typed)
                    if (rank3(engine.process(tapsFor(target.substr(0, typed))).suggestions, target) >= 0)
                        break;
            } else if (r < 0) {
                // -Y: each prefix searched once, decided for each setting.
                std::vector<bool> done(variants.size() + 1, false);
                size_t left = done.size();
                for (size_t n = 1; n < target.size() && left > 0; ++n) {
                    const Searched found = engine.search(tapsFor(target.substr(0, n)));
                    for (size_t v = 0; v < done.size(); ++v) {
                        if (done[v] || rank3(engine.decide(found, v ? variants[v - 1] : params).suggestions, target) < 0)
                            continue;
                        done[v] = true;
                        --left;
                        (v ? vTyped[v - 1] : typed) = n;
                    }
                }
                for (size_t v = 0; v < done.size(); ++v)
                    if (!done[v])
                        (v ? vTyped[v - 1] : typed) = target.size();
            }
            letters += target.size() + 1;
            taps += typed + 1;
            if (typed < target.size()) {
                ++picked;
                if (typed == 0) ++pickedNext;
            }
            for (size_t v = 0; v < variants.size(); ++v) {
                vTaps[v] += vTyped[v] + 1;
                if (vTyped[v] < target.size()) {
                    ++vPicked[v];
                    if (vTyped[v] == 0) ++vPickedNext[v];
                }
            }
            if (verbose && r < 0) {
                const std::vector<std::u32string> before = words(context);
                std::printf("miss %-14s → %-14s next %-30s picked after %zu/%zu%s\n",
                            before.empty() ? "^" : utf8::encode(before.back()).c_str(),
                            utf8::encode(target).c_str(), top3(next).c_str(), typed, target.size(),
                            known ? "" : " (unknown)");
            }
            if (learn)
                engine.learn(learned, context, target);
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
    if (learn) {
        size_t newWords = 0, pending = 0, pairs = 0, used = 0;
        for (const std::string &code : learned.languages()) {
            const Learned::Language *l = learned.language(code);
            pairs += l->pairs;
            for (const auto &w : l->words)
                (w.second.pending ? pending : w.second.isNew ? newWords : used) += 1;
        }
        std::printf("learned: %zu words counted, %zu new words (%zu pending), %zu pairs\n", used, newWords,
                    pending, pairs);
    }
    std::printf("unknown words %.1f%%; picked %.1f%% of words (%.1f%% before the first letter);"
                " taps %ld of %ld\n", pct(unknown, all.words), pct(picked, all.words),
                pct(pickedNext, all.words), taps, letters);
    Counts counts;
    for (int g = 0; g < 2; ++g) {
        counts.words[g] = groups[g].words;
        counts.top1[g] = groups[g].top1;
        counts.top3[g] = groups[g].top3;
    }
    counts.unknown = unknown;
    counts.letters = letters;
    for (size_t v = 0; v <= variants.size(); ++v) {
        counts.picked = v ? vPicked[v - 1] : picked;
        counts.pickedNext = v ? vPickedNext[v - 1] : pickedNext;
        counts.taps = v ? vTaps[v - 1] : taps;
        const std::string key = v ? "[" + variantSpecs[v - 1] + "]" : "";
        if (printCounts) {
            std::string line;
            for (long x : counts.values())
                line += " " + std::to_string(x);
            std::printf("counts%s:%s\n", key.c_str(), line.c_str());
        }
        if (v)
            std::printf("summary%s: %s\n", key.c_str(), summaryText(counts).c_str());
    }
    counts.picked = picked;
    counts.pickedNext = pickedNext;
    counts.taps = taps;
    std::printf("summary: %s\n", summaryText(counts).c_str());
    return 0;
}
