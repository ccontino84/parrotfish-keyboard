// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

// Correction and autocorrect accuracy on generated typos (milestone 1).
//
//   typo-bench -l data/layouts/it.layout -d build/data/it.pfd [-d OTHER.pfd]...
//              [-o oov-it.txt] [-n 1000] [-s SEED] [-u] [-D] [-P name=value]... [-v]
//
// The first dictionary is the language typed. Further dictionaries are
// loaded as extra typing languages; the text is taken to be clearly in the
// first language (weight 0.98), or all languages are equally likely (-u).
//
// Words are drawn from the first dictionary in proportion to their
// frequency (as they occur in text), skipping offensive words, misspellings,
// abbreviations and one-letter words. Each gets one kind of typo. The noise
// is deliberately simpler than the engine's model (a uniformly chosen
// adjacent key), so the test doesn't just measure the model against itself.
//
// -o FILE: real words missing from the dictionaries, one per line (see
// tools/make-oov-list.py), typed correctly: autocorrect must leave them alone.
//
// -D adds two kinds: one letter of a double missed (aparte), a letter tapped
// twice (coomputer); the other kinds are unchanged.

#include "parrotfish/engine.h"
#include "parrotfish/utf8.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <random>
#include <sstream>

using namespace parrotfish;

namespace {

struct Word {
    std::u32string text;
    double weight;
};

std::u32string lower(std::u32string s)
{
    for (char32_t &c : s)
        c = utf8::toLower(c);
    return s;
}

bool isLetter(char32_t c)
{
    return utf8::toLower(c) != utf8::toUpper(c) || c == U'ß';
}

// Accent-stripped form via the dictionary's alphabet table (ß → ss).
std::u32string stripAccents(const std::map<char32_t, char32_t> &base, const std::u32string &w)
{
    std::u32string out;
    for (char32_t c : w) {
        auto it = base.find(c);
        const char32_t b = it == base.end() ? c : it->second;
        if (b == U'ß') {
            out += U"ss";
        } else {
            out.push_back(b);
        }
    }
    return out;
}

enum Kind { Clean, Neighbour, Deletion, Insertion, Transposition, Accent, Double, DoubleMissed,
            DoubleExtra, KindCount };
const char *kindName[] = {"clean", "neighbour", "deletion", "insertion", "transposition",
                          "accent-less", "two typos", "double missed", "tapped twice"};

struct Stats {
    int total = 0, realWord = 0, top1 = 0, top3 = 0;
    int fixed = 0, wrong = 0;   // autocorrect applied the right / a wrong word
    void add(const Stats &o)
    {
        total += o.total; realWord += o.realWord; top1 += o.top1; top3 += o.top3;
        fixed += o.fixed; wrong += o.wrong;
    }
};

double pct(int a, int b)
{
    return b ? 100.0 * a / b : 0.0;
}

} // namespace

int main(int argc, char **argv)
{
    std::string layoutPath, oovPath;
    std::vector<std::string> dictPaths;
    int perKind = 1000;
    unsigned seed = 1;
    bool verbose = false, uniform = false, doubles = false;
    EngineParams params;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-l" && i + 1 < argc) layoutPath = argv[++i];
        else if (a == "-d" && i + 1 < argc) dictPaths.push_back(argv[++i]);
        else if (a == "-o" && i + 1 < argc) oovPath = argv[++i];
        else if (a == "-n" && i + 1 < argc) perKind = std::stoi(argv[++i]);
        else if (a == "-s" && i + 1 < argc) seed = std::stoul(argv[++i]);
        else if (a == "-u") uniform = true;
        else if (a == "-D") doubles = true;
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
    KeyModel keys = KeyModel::parse(ls.str());
    Engine engine(params);
    std::string error;
    for (const std::string &p : dictPaths) {
        if (!engine.addLanguage(p, &error)) {
            std::fprintf(stderr, "%s\n", error.c_str());
            return 1;
        }
    }
    if (keys.keys().empty() || engine.languageCount() == 0) {
        std::fprintf(stderr, "usage: typo-bench -l LAYOUT -d DICT [-d DICT]... [-o OOV] [-n N] [-s SEED]"
                             " [-u] [-D] [-P name=value]... [-v]\n");
        return 2;
    }
    engine.setLayout(keys);
    std::vector<double> prior(engine.languageCount(), 1.0);
    if (!uniform && prior.size() > 1) {
        prior.assign(prior.size(), 0.02 / (prior.size() - 1));
        prior[0] = 0.98;
    }
    const Dictionary &dict = engine.dictionary(0);

    std::map<char32_t, char32_t> base;
    for (uint32_t i = 0; i < dict.alphabetSize(); ++i)
        base[dict.letter(i).cp] = dict.letter(i).base;
    std::vector<Word> words, accented;
    dict.forEachWord([&](uint32_t node, const std::u32string &w) {
        const uint32_t attrs = dict.attributes(node);
        if (attrs & (format::AttrOffensive | format::AttrMisspelling | format::AttrAbbreviation))
            return;
        if (w.size() < 2 || !std::all_of(w.begin(), w.end(), [](char32_t c) {
                return isLetter(c) || c == U'\''; }))
            return;
        Word x{w, std::exp(dict.logProb(dict.node(node).freq))};
        words.push_back(x);
        if (stripAccents(base, w) != lower(w))
            accented.push_back(x);
    });
    auto sampler = [](const std::vector<Word> &v) {
        std::vector<double> w;
        for (const Word &x : v) w.push_back(x.weight);
        return std::discrete_distribution<size_t>(w.begin(), w.end());
    };
    auto pickWord = sampler(words);
    auto pickAccented = sampler(accented);
    std::mt19937 rng(seed);

    // Adjacent keys for the noise model (as typos.h: by key widths across and
    // key heights up and down, whatever the engine's tap model).
    std::map<char32_t, std::vector<char32_t>> adjacent;
    const KeyModel grid(keys.keys(), keys.sigma(), 1.0f);
    for (size_t k = 0; k < grid.keys().size(); ++k)
        for (int n : grid.neighbours(static_cast<int>(k)))
            adjacent[grid.keys()[k].ch].push_back(grid.keys()[n].ch);

    auto pos = [&](size_t n) { return std::uniform_int_distribution<size_t>(0, n - 1)(rng); };
    auto typo = [&](std::u32string w, Kind kind) -> std::u32string {
        w = lower(w);
        switch (kind) {
        case Neighbour:
            for (int tries = 0; tries < 10; ++tries) {
                const size_t i = pos(w.size());
                auto it = adjacent.find(w[i]);
                if (it == adjacent.end()) continue;
                w[i] = it->second[pos(it->second.size())];
                return w;
            }
            return {};
        case Deletion:
            w.erase(pos(w.size()), 1);
            return w;
        case Insertion: {
            const size_t i = pos(w.size());
            auto it = adjacent.find(w[i]);
            const char32_t c = (it == adjacent.end() || pos(2) == 0) ? w[i] : it->second[pos(it->second.size())];
            w.insert(w.begin() + i + pos(2), c);
            return w;
        }
        case DoubleMissed: {
            std::vector<size_t> at;
            for (size_t i = 0; i + 1 < w.size(); ++i)
                if (w[i] == w[i + 1] && isLetter(w[i])) at.push_back(i);
            if (at.empty()) return {};
            w.erase(at[pos(at.size())], 1);
            return w;
        }
        case DoubleExtra: {
            const size_t i = pos(w.size());
            if (!isLetter(w[i])) return {};
            w.insert(w.begin() + i, w[i]);
            return w;
        }
        case Transposition: {
            if (w.size() < 3) return {};
            const size_t i = pos(w.size() - 1);
            if (w[i] == w[i + 1]) return {};
            std::swap(w[i], w[i + 1]);
            return w;
        }
        default:
            return w;
        }
    };

    double totalUs = 0, maxUs = 0;
    int searches = 0;
    auto run = [&](const std::u32string &typed) {
        engine.startText({}, prior);
        const auto t0 = std::chrono::steady_clock::now();
        Result r = engine.process(tapsFor(typed));
        const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        totalUs += us;
        maxUs = std::max(maxUs, us);
        ++searches;
        return r;
    };

    Stats stats[KindCount];
    for (int kind = 0; kind < KindCount; ++kind) {
        if (kind >= DoubleMissed && !doubles)
            break;
        int made = 0;
        while (made < perKind) {
            if (kind == Accent && accented.empty()) break;
            const std::u32string target = kind == Accent ? accented[pickAccented(rng)].text
                                                         : words[pickWord(rng)].text;
            std::u32string typed;
            if (kind == Accent) {
                typed = lower(stripAccents(base, target));
            } else if (kind == Double) {
                if (target.size() < 5) continue;
                typed = typo(target, Kind(Neighbour + pos(4)));
                if (!typed.empty()) typed = typo(typed, Kind(Neighbour + pos(4)));
            } else {
                typed = typo(target, Kind(kind));
            }
            if (typed.size() < 2)   // one letter is too ambiguous to score
                continue;
            ++made;
            Stats &s = stats[kind];
            ++s.total;
            const Result r = run(typed);
            const bool same = lower(typed) == lower(target);
            if (kind != Clean && same)
                continue;   // e.g. a swap of two equal letters
            if (kind != Clean && r.typedIsWord) {
                ++s.realWord;   // the typo is itself a word: not fixable without context
                continue;
            }
            int rank = -1;
            for (size_t k = 0; k < r.suggestions.size() && k < 3; ++k)
                if (lower(r.suggestions[k].word) == lower(target)) { rank = static_cast<int>(k); break; }
            if (kind == Clean) {
                // Correct input: it must not be replaced.
                if (r.autocorrect >= 0 && lower(r.suggestions[r.autocorrect].word) != lower(target))
                    ++s.wrong;
                else
                    ++s.top1;
                continue;
            }
            if (rank == 0) ++s.top1;
            if (rank >= 0) ++s.top3;
            if (r.autocorrect >= 0) {
                if (lower(r.suggestions[r.autocorrect].word) == lower(target)) ++s.fixed;
                else ++s.wrong;
            }
            if (verbose && (rank != 0 || (r.autocorrect >= 0 && rank != 0)))
                std::printf("%-14s %-16s -> %-16s rank %d%s%s\n", kindName[kind], utf8::encode(typed).c_str(),
                            utf8::encode(target).c_str(), rank,
                            r.suggestions.empty() ? "" : ("  got " + utf8::encode(r.suggestions[0].word)).c_str(),
                            r.autocorrect >= 0 ? " (autocorrected)" : "");
        }
    }

    std::printf("%s: %zu words (%zu with accents), %zu languages%s, seed %u\n", dict.language().c_str(),
                words.size(), accented.size(), engine.languageCount(),
                uniform ? " (equally likely)" : "", seed);
    std::printf("%-14s %6s %8s %7s %7s %10s %10s\n", "typo", "cases", "realword", "top1", "top3",
                "autofixed", "autowrong");
    Stats all;
    for (int kind = 0; kind < KindCount; ++kind) {
        const Stats &s = stats[kind];
        if (s.total == 0) continue;
        if (kind == Clean) {
            std::printf("%-14s %6d %8s %7s %7s %10s %9.2f%%\n", kindName[kind], s.total, "", "", "", "",
                        pct(s.wrong, s.total));
            continue;
        }
        const int fixable = s.total - s.realWord;
        std::printf("%-14s %6d %7.1f%% %6.1f%% %6.1f%% %9.1f%% %9.1f%%\n", kindName[kind], s.total,
                    pct(s.realWord, s.total), pct(s.top1, fixable), pct(s.top3, fixable),
                    pct(s.fixed, fixable), pct(s.wrong, fixable));
        all.add(s);
    }
    const int fixable = all.total - all.realWord;
    std::printf("%-14s %6d %7.1f%% %6.1f%% %6.1f%% %9.1f%% %9.1f%%\n", "all typos", all.total,
                pct(all.realWord, all.total), pct(all.top1, fixable), pct(all.top3, fixable),
                pct(all.fixed, fixable), pct(all.wrong, fixable));

    if (!oovPath.empty()) {
        std::ifstream in(oovPath);
        std::string line;
        int total = 0, changed = 0, known = 0;
        std::vector<std::string> examples;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            const std::u32string w = utf8::decode(line);
            const Result r = run(w);
            if (r.typedIsWord) { ++known; continue; }
            ++total;
            if (r.autocorrect >= 0) {
                ++changed;
                if (examples.size() < 12)
                    examples.push_back(line + "→" + utf8::encode(r.suggestions[r.autocorrect].word));
            }
        }
        std::printf("unknown real words: %d, autocorrected %.1f%% (%d already known)\n", total,
                    pct(changed, total), known);
        if (verbose || !examples.empty()) {
            std::printf("  e.g.");
            for (const auto &e : examples) std::printf(" %s", e.c_str());
            std::printf("\n");
        }
    }
    std::printf("time per word: mean %.0f µs, max %.0f µs\n", totalUs / searches, maxUs);
    return 0;
}
