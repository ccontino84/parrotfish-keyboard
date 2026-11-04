// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

// Correction and autocorrect in running text (milestone 1/MVP).
//
//   sentence-bench -l data/layouts/it.layout -t tests/data/tatoeba-it.txt
//                  -d build/data/it.pfd [-d OTHER.pfd]... [-n 2000] [-r 0.3]
//                  [-s SEED] [-m LANG] [-B] [-F] [-D] [-R] [-P name=value]... [-v]
//                  [-S slips.log [-M] | -U] [-N PCT] [-X] [-o OOV.txt [-u RATE]] [-L]
//                  [-T [-K]] [-W pairs.tsv [-w RATE] [-c N] [-A]] [-E] [-C DUMP [-G]] [-Y SETTINGS]...
//                  [-k START] [-Z | -J] [-O OUTCOMES]
//                  [-Q [-q name=value]... [-H LINES] [-x RATE] [-I N]] [-V LINES]
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
// no loaded dictionary knows; with -N also with one digit after a letter, mp3), a share -u (0.1) of the sentences' words,
// typed correctly: autocorrect must leave them alone.
//
// -T adds uncertain taps: a tap that touched one key and lifted on its
// neighbour, one of them the meant letter (either way round). The word
// shows the lift key, or the touch-down key if the word so far fits it
// better (Engine::resolveTap, as the handler). -K also gives the engine the
// tap's other key, from alpha70 on; without it only the letter shown.
//
// -W adds real misspellings (recieve, propio, Vorraussetzung) from a list:
// misspelling TAB correct [TAB source [TAB flags]] per line, # for comments.
// A word of the sentences that has some (ignoring case; a correction of two
// words matches two words of the sentence, alot for "a lot") is typed as
// one of them, picked at random, with no other slip: a share -w (1) of such
// words, at most -c (3) times each correct word, so that the few common
// words (teh for the) don't make up the sample. Flag accents-only marks a
// misspelling in accents only (citta, Strasse), counted apart. With -A the
// sentences are only context: each pair of the list is typed once, after
// the first three words of a sentence (in turn). The kinds "misspelling"
// and "missp. accents" are not part of "all typos"; the summary adds them,
// fixed (the correct word), wrong (changed to something else) and left.
//
// -L adds the results by typed length (2, 3-4, 5+ letters; typos of two
// words left out), for thresholds by length, and the same for the words
// typed with a capital after a word ("capital": names, German nouns).
//
// Words are split as bench::typedWords() does (typos.h): a final apostrophe
// stays only where the word with it is a dictionary word (po'). -E prints
// the words of each text as they would be typed, and nothing else.
//
// -C DUMP writes one JSON line for each word typed with a typo or a
// misspelling, and each word changed: typed, target, the outcome, the
// engine's Result::reason, confidence and unknownShare, the target's rank
// among the suggestions and the first 8 suggestions ([word, cost, log P of
// the word or null if no dictionary has it, changes, completion]); for
// tools/pf failures. Off by default (nothing is computed without it).
// -G: every word, also those typed right and left as they are.
//
// -Q learns as one user (Engine::learn, as the plugin): each word is kept
// as meant, but a word the engine got wrong (a typo left or fixed into
// another word) is kept as it came out a share -x of the time (0.25: the
// user didn't notice); -H LINES lines make one text (1), -q sets
// LearnParams. -V LINES: the first LINES lines are typed without typos and
// learned, not counted (with or without -Q: the same words measured). The
// learned words that aren't words of the texts (or the -o list) are typos
// learned: "learned typos". -I N (with -Q) prints a line every N words
// learned: the text, the words learned so far and per language the data
// (words: in use, new, pending; pairs; total; learned typos), and a line
// for each fading or trim (the rows of its language before and after, the
// time of that Learned::maintain(), run at each text's end); the changes
// are taken every 50 words learned, as the plugin saves them (rows of the
// largest save that isn't a rewrite).
//
// -Y name=value[,name=value...] (repeatable) replays the decision with
// other settings on each word's search (Engine::search once, then
// Engine::decide for each): only parameters isDecisionParam() lists
// (thresholds, margins, unknown*; strength unless Strong). Each adds a line
// summary[SETTINGS]: before the summary, the same numbers as a run with
// -P SETTINGS, for little more time than one run.
//
// -k START types the first START texts without the engine (their typos are
// still drawn, so that the rest get the same ones): with -n END, texts
// START+1..END, a shard of the run. -Z adds counts lines; -J (no other
// option) reads them on stdin and adds them up into the summary lines of
// the whole run (tools/pf shards long cases so).
//
// -O OUTCOMES writes a line for each word typed with a typo or a
// misspelling and each word changed, for each setting (0: the bench's, 1..:
// the -Y ones): setting, text, word, kind, typed, target, what Space gave,
// tab-separated (tools/pf diff pairs two runs' words).
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
#include <iostream>
#include <map>
#include <tuple>
#include <random>
#include <set>
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

// Words by typed length, for -L (thresholds by length).
enum LengthGroup { Len2, Len34, Len5, LengthCount };
const char *const lengthNames[] = {"2", "3-4", "5+"};

struct LengthStats {
    int clean = 0, cleanWrong = 0, unknown = 0, unknownWrong = 0, fixable = 0, fixed = 0, wrong = 0;
};

double pct(int a, int b)
{
    return b ? 100.0 * a / b : 0.0;
}

bool isMisspelledKind(int kind)
{
    return kind == Misspelled || kind == MisspelledAccents;
}

// The counts of one run of the decision (the bench's own, or a -Y variant).
struct Tally {
    Stats stats[KindCount];
    GroupStats groups[GroupCount];
    LengthStats lengths[LengthCount], capitals;
    int typedWords = 0, wrongBefore = 0, wrongAfter = 0;
};

// The counts behind the summary line (-Z), and back (-J).
std::vector<long> summaryCounts(const Tally &t, bool misspellings)
{
    std::vector<long> v = {misspellings ? 1 : 0, t.typedWords, t.wrongBefore, t.wrongAfter};
    for (const Stats &s : t.stats)
        v.insert(v.end(), {s.total, s.realWord, s.top1, s.top3, s.fixed, s.wrong});
    return v;
}

bool addCounts(Tally &t, bool *misspellings, const std::vector<long> &v)
{
    if (v.size() != 4 + 6 * KindCount)
        return false;
    *misspellings = *misspellings || v[0];
    t.typedWords += v[1]; t.wrongBefore += v[2]; t.wrongAfter += v[3];
    for (int k = 0; k < KindCount; ++k) {
        Stats &s = t.stats[k];
        const long *x = &v[4 + 6 * k];
        s.total += x[0]; s.realWord += x[1]; s.top1 += x[2]; s.top3 += x[3]; s.fixed += x[4]; s.wrong += x[5];
    }
    return true;
}

// The summary line's numbers (after "summary: "), from the counts.
std::string summaryText(const Tally &t, bool misspellings)
{
    const Stats *stats = t.stats;
    Stats all;
    for (int k = 0; k < KindCount; ++k) {
        if (k == Clean || k == Unknown || isMisspelledKind(k))
            continue;
        const Stats &s = stats[k];
        all.total += s.total; all.realWord += s.realWord; all.top1 += s.top1; all.top3 += s.top3;
        all.fixed += s.fixed; all.wrong += s.wrong;
    }
    const int fixable = all.total - all.realWord;
    auto misspText = [&](int k) {
        const Stats &s = stats[k];
        const int n = s.total - s.realWord;
        char buf[160];
        std::snprintf(buf, sizeof buf, "fixed %.1f%% wrong %.1f%% left %.1f%%", pct(s.fixed, n), pct(s.wrong, n),
                      pct(n - s.fixed - s.wrong, n));
        return std::string(buf);
    };
    char buf[512];
    std::snprintf(buf, sizeof buf, "typos fixed %.1f%% wrong %.1f%%, correct changed %.2f%%, unknown changed %.2f%%,"
                  " WER %.2f%% -> %.2f%%", pct(all.fixed, fixable), pct(all.wrong, fixable),
                  pct(stats[Clean].wrong, stats[Clean].total), pct(stats[Unknown].wrong, stats[Unknown].total),
                  pct(t.wrongBefore, t.typedWords), pct(t.wrongAfter, t.typedWords));
    std::string out = buf;
    if (misspellings)
        out += ", misspellings " + misspText(Misspelled) + ", accents only " + misspText(MisspelledAccents);
    return out;
}

bool isMisspelled(int kind)
{
    return kind == Misspelled || kind == MisspelledAccents;
}

// A real misspelling of a word (-W).
struct Misspelling {
    std::u32string typed;
    bool accentsOnly = false;
};

// One line of the -W list: misspelling TAB correct [TAB source [TAB flags]].
bool parsePair(const std::string &line, std::u32string *typed, std::u32string *correct, bool *accentsOnly)
{
    std::vector<std::string> f;
    std::istringstream in(line);
    for (std::string x; std::getline(in, x, '\t');)
        f.push_back(x);
    if (f.size() < 2 || f[0].empty() || f[1].empty())
        return false;
    *typed = utf8::decode(f[0]);
    *correct = utf8::decode(f[1]);
    *accentsOnly = f.size() > 3 && ("," + f[3] + ",").find(",accents-only,") != std::string::npos;
    return true;
}

// -J: counts lines on stdin, added up per setting, as summary lines.
int joinCounts()
{
    std::vector<std::string> order;
    std::map<std::string, Tally> sums;
    bool misspellings = false;
    for (std::string line; std::getline(std::cin, line);) {
        if (line.compare(0, 6, "counts") != 0)
            continue;
        const size_t colon = line.find(": ");
        const std::string key = line.substr(6, colon - 6);
        std::istringstream in(line.substr(colon + 2));
        std::vector<long> v;
        for (long x; in >> x;)
            v.push_back(x);
        if (!sums.count(key))
            order.push_back(key);
        if (!addCounts(sums[key], &misspellings, v))
            return 2;
    }
    for (const std::string &key : order)
        if (!key.empty())
            std::printf("summary%s: %s\n", key.c_str(), summaryText(sums[key], misspellings).c_str());
    if (sums.count(""))
        std::printf("summary: %s\n", summaryText(sums[""], misspellings).c_str());
    return 0;
}

} // namespace

int main(int argc, char **argv)
{
    std::string layoutPath, textPath;
    std::vector<std::string> dictPaths;
    int maxSentences = 2000;
    double rate = 0.3;
    unsigned seed = 1;
    bool verboseOpt = false, breakdown = false, byLength = false, firstOnly = false, doubles = false, realWords = false;
    std::string remembered, slipsPath, oovPath, misspPath, dumpPath, outcomesPath;
    bool dumpAll = false;   // -G: -C dumps every word, not only slips and changes
    std::vector<std::string> variantSpecs;
    bool uniform = false, mirrored = false, extraKinds = false, uncertainTaps = false, keepBoth = false;
    int numberPct = 0;
    double unknownRate = 0.1, misspRate = 1.0;
    int misspCap = 3;
    bool pairsAlone = false, echoWords = false, printCounts = false;
    int skipTexts = 0;
    bool learn = false;
    int textLines = 1, warmTexts = 0;
    long checkpointEvery = 0;
    double typoKeepRate = 0.25;
    LearnParams learnParams;
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
        else if (a == "-L") byLength = true;
        else if (a == "-F") firstOnly = breakdown = true;
        else if (a == "-D") doubles = true;
        else if (a == "-R") realWords = true;
        else if (a == "-v") verboseOpt = true;
        else if (a == "-S" && i + 1 < argc) slipsPath = argv[++i];
        else if (a == "-U") uniform = true;
        else if (a == "-M") mirrored = true;
        else if (a == "-N" && i + 1 < argc) numberPct = std::stoi(argv[++i]);
        else if (a == "-X") extraKinds = true;
        else if (a == "-T") uncertainTaps = true;
        else if (a == "-K") keepBoth = true;
        else if (a == "-o" && i + 1 < argc) oovPath = argv[++i];
        else if (a == "-u" && i + 1 < argc) unknownRate = std::stod(argv[++i]);
        else if (a == "-W" && i + 1 < argc) misspPath = argv[++i];
        else if (a == "-w" && i + 1 < argc) misspRate = std::stod(argv[++i]);
        else if (a == "-c" && i + 1 < argc) misspCap = std::stoi(argv[++i]);
        else if (a == "-A") pairsAlone = true;
        else if (a == "-E") echoWords = true;
        else if (a == "-C" && i + 1 < argc) dumpPath = argv[++i];
        else if (a == "-G") dumpAll = true;
        else if (a == "-O" && i + 1 < argc) outcomesPath = argv[++i];
        else if (a == "-Y" && i + 1 < argc) variantSpecs.push_back(argv[++i]);
        else if (a == "-k" && i + 1 < argc) skipTexts = std::stoi(argv[++i]);
        else if (a == "-Q") learn = true;
        else if (a == "-H" && i + 1 < argc) textLines = std::max(1, std::stoi(argv[++i]));
        else if (a == "-V" && i + 1 < argc) warmTexts = std::stoi(argv[++i]);
        else if (a == "-x" && i + 1 < argc) typoKeepRate = std::stod(argv[++i]);
        else if (a == "-I" && i + 1 < argc) checkpointEvery = std::stol(argv[++i]);
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
    // -Y: decision settings replayed on each word's search (only the
    // parameters Engine::decide alone reads).
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
    if (keys.keys().empty() || engine.languageCount() == 0 || !tf || (pairsAlone && misspPath.empty())) {
        std::fprintf(stderr, "usage: sentence-bench -l LAYOUT -t SENTENCES -d DICT [-d DICT]... [-n N] [-r RATE]"
                             " [-s SEED] [-m LANG] [-B] [-F] [-D] [-R] [-P name=value]... [-v]"
                             " [-W PAIRS [-w RATE] [-c N] [-A]]\n");
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
    // What the handler types as one word: letters and apostrophes, and with
    // a number row one digit after a letter (mp3, h3llo).
    const bool digits = typos.hasDigits();
    auto isPlain = [digits](const std::u32string &w) {
        if (w.size() < 2)
            return false;
        int n = 0;
        for (size_t i = 0; i < w.size(); ++i)
            if (w[i] >= U'0' && w[i] <= U'9') {
                if (!digits || i == 0 || ++n > 1)
                    return false;
            } else if (!isLetter(w[i]) && w[i] != U'\'') {
                return false;
            }
        return true;
    };
    // Unknown words: 3+ characters typed as one word (no apostrophe), in no
    // loaded dictionary.
    std::vector<std::u32string> unknown;
    if (!oovPath.empty()) {
        std::ifstream of(oovPath);
        for (std::string w; std::getline(of, w);) {
            if (!w.empty() && w[0] == '#')
                continue;
            const std::u32string u = utf8::decode(w);
            if (u.size() < 3 || !isPlain(u) || u.find(U'\'') != std::u32string::npos)
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
    // Real misspellings by their correct word or words (lowercase); in the
    // list's order, -A types them all.
    std::map<std::u32string, std::vector<Misspelling>> misspellings;
    std::vector<std::pair<std::u32string, Misspelling>> pairs;   // correct, misspelling
    if (!misspPath.empty()) {
        std::ifstream mf(misspPath);
        for (std::string l; std::getline(mf, l);) {
            std::u32string typed, correct;
            Misspelling m;
            if (l.empty() || l[0] == '#' || !parsePair(l, &typed, &correct, &m.accentsOnly) || !isPlain(typed))
                continue;
            m.typed = typed;
            misspellings[lower(correct)].push_back(m);
            pairs.emplace_back(correct, m);
        }
        if (pairs.empty()) {
            std::fprintf(stderr, "-W %s: no misspellings\n", misspPath.c_str());
            return 2;
        }
    }
    std::map<std::u32string, int> misspUsed;   // times each correct word was misspelled (-c)

    // -C: one JSON line per typed word that had a typo or misspelling, or
    // was changed: what was typed and meant, the outcome, the engine's
    // reason and the candidates (pf failures sorts the failures by cause).
    std::FILE *dump = nullptr;
    if (!dumpPath.empty() && !(dump = std::fopen(dumpPath.c_str(), "w"))) {
        std::fprintf(stderr, "-C %s: cannot write\n", dumpPath.c_str());
        return 2;
    }
    std::FILE *outcomes = nullptr;
    if (!outcomesPath.empty() && !(outcomes = std::fopen(outcomesPath.c_str(), "w"))) {
        std::fprintf(stderr, "-O %s: cannot write\n", outcomesPath.c_str());
        return 2;
    }
    auto json = [](const std::u32string &w) {
        std::string out = "\"";
        for (char c : utf8::encode(w)) {
            if (c == '"' || c == '\\')
                out += '\\';
            out += c;
        }
        return out + "\"";
    };
    // The best log probability of a word (each word of a two-word target)
    // in the loaded dictionaries, as a word (not a listed misspelling); 1 if
    // some word is in none of them.
    auto wordLogProb = [&](const std::u32string &words) {
        double total = 0;
        std::u32string w;
        for (size_t i = 0; i <= words.size(); ++i) {
            if (i < words.size() && words[i] != U' ') {
                w.push_back(words[i]);
                continue;
            }
            double best = 1;
            for (size_t d = 0; d < engine.languageCount(); ++d) {
                const Dictionary &dict = engine.dictionary(d);
                for (const std::u32string &v : {w, lower(w)}) {
                    const uint32_t node = dict.find(v);
                    if (node && !(dict.attributes(node) & format::AttrMisspelling)) {
                        const double lp = dict.logProb(dict.node(node).freq);
                        best = best > 0 ? lp : std::max(best, lp);
                    }
                }
            }
            if (best > 0)
                return 1.0;
            total += best;
            w.clear();
        }
        return total;
    };

    // Typo kinds to pick from.
    std::vector<Kind> kinds;
    for (int k = Neighbour; k <= (doubles ? DoubleExtra : Double); ++k)
        kinds.push_back(Kind(k));
    if (uncertainTaps)
        kinds.push_back(Uncertain);
    if (extraKinds) {
        kinds.push_back(Joined);
        kinds.push_back(SpaceLetter);
        if (typos.hasDigits())
            kinds.push_back(NumberRow);
    }
    // Keys above the spacebar (phone QWERTY, QWERTZ: c to m).
    const std::u32string aboveSpace = U"cvbnm";
    std::uniform_real_distribution<double> coin(0.0, 1.0);

    std::vector<Tally> tallies(1 + variants.size());   // the bench's, then each -Y variant's
    double totalUs = 0, maxUs = 0;
    int searches = 0, sentences = 0;
    // The texts: the sentences, or with -A one for each pair: three words of
    // a sentence, then the pair's correct word or words (one entry).
    // ends[i]: word i ends a sentence (. ! ? … after it, before the next
    // word): the context then has ". " after it, so that the next word is
    // a sentence start for the engine, as on the phone.
    std::vector<std::vector<std::u32string>> texts;
    std::vector<std::vector<bool>> textEnds;
    for (std::string line; static_cast<int>(texts.size()) < maxSentences && std::getline(tf, line);) {
        if (line.empty() || line[0] == '#')
            continue;
        std::u32string text = utf8::decode(line);
        for (char32_t &c : text)
            if (c == 0x2019 || c == 0x02BC)
                c = U'\'';   // as words() reads them
        texts.push_back(bench::typedWords(text, engine));
        std::vector<bool> ends(texts.back().size(), false);
        size_t pos = 0;
        for (size_t i = 0; i < ends.size(); ++i) {
            const size_t at = text.find(texts.back()[i], pos);
            if (at == std::u32string::npos)
                break;
            pos = at + texts.back()[i].size();
            const size_t next = i + 1 < ends.size() ? text.find(texts.back()[i + 1], pos) : std::u32string::npos;
            const std::u32string gap = text.substr(pos, next == std::u32string::npos ? 0 : next - pos);
            ends[i] = gap.find_first_of(U".!?\u2026") != std::u32string::npos;
        }
        textEnds.push_back(ends);
    }
    if (echoWords) {   // -E: the words as typed, one text a line
        for (const std::vector<std::u32string> &t : texts) {
            std::string line;
            for (const std::u32string &w : t)
                line += (line.empty() ? "" : " | ") + utf8::encode(w);
            std::printf("%s\n", line.c_str());
        }
        return 0;
    }
    if (pairsAlone && !texts.empty()) {
        std::vector<std::vector<std::u32string>> alone;
        for (size_t p = 0; p < pairs.size(); ++p) {
            const std::vector<std::u32string> &t = texts[p % texts.size()];
            alone.emplace_back(t.begin(), t.begin() + std::min<size_t>(3, t.size()));
            alone.back().push_back(pairs[p].first);
        }
        texts.swap(alone);
        textEnds.assign(texts.size(), {});
        for (size_t i = 0; i < texts.size(); ++i)
            textEnds[i].assign(texts[i].size(), false);
    }
    Learned learned(learnParams);
    if (learn)
        engine.setLearned(&learned);
    std::mt19937 learnRng(seed);   // apart: the typos stay the same with -Q
    std::set<std::u32string> textWords;   // the texts' words and the -o list: not typos
    for (const std::vector<std::u32string> &t : texts)
        for (const std::u32string &w : t)
            textWords.insert(lower(w));
    for (const std::u32string &w : unknown)
        textWords.insert(lower(w));
    // -I: the learned data over time.
    long keptWords = 0, fades = 0, trims = 0;
    size_t maxSaveRows = 0;
    size_t currentText = 0;
    auto rowsOf = [](const Learned::Language *l) { return l ? l->words.size() + l->pairs : 0; };
    auto checkpoint = [&]() {
        std::printf("checkpoint: text %zu, kept %ld, fades %ld, trims %ld, largest save %zu rows\n", currentText,
                    keptWords, fades, trims, maxSaveRows);
        for (const std::string &code : learned.languages()) {
            const Learned::Language *l = learned.language(code);
            size_t used = 0, isNew = 0, pending = 0, typosLearned = 0;
            for (const auto &w : l->words) {
                (w.second.pending ? pending : w.second.isNew ? isNew : used) += 1;
                if (!w.second.pending && w.second.isNew && !textWords.count(w.first))
                    ++typosLearned;
            }
            std::printf("checkpoint %s: typed %.0f, words %zu (%zu used, %zu new, %zu pending), pairs %zu, "
                        "total %.0f, learned typos %zu\n", code.c_str(), l->typed, l->words.size(), used, isNew,
                        pending, l->pairs, l->total, typosLearned);
        }
        std::fflush(stdout);
    };
    // Keeps `word` (one word or more) after `context`, as the plugin would.
    // Learned::maintain(), with -I a line for each fading or trim: the rows
    // of its language before and after, the time it took.
    auto maintain = [&]() {
        if (checkpointEvery <= 0 || !learned.maintenanceDue()) {
            learned.maintain();
            return;
        }
        std::map<std::string, std::tuple<double, size_t, size_t>> was;
        for (const std::string &code : learned.languages()) {
            const Learned::Language *l = learned.language(code);
            was[code] = std::make_tuple(l->nextFade, l->words.size(), l->pairs);
        }
        const auto t0 = std::chrono::steady_clock::now();
        learned.maintain();
        const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        for (const auto &e : was) {
            const Learned::Language *l = learned.language(e.first);
            const bool faded = l->nextFade != std::get<0>(e.second);
            if (!faded && l->words.size() >= std::get<1>(e.second) && l->pairs >= std::get<2>(e.second))
                continue;
            ++(faded ? fades : trims);
            std::printf("event: %s %s at typed %.0f, rows %zu -> %zu (words %zu, pairs %zu), %.0f µs\n",
                        faded ? "fade" : "trim", e.first.c_str(), l->typed,
                        std::get<1>(e.second) + std::get<2>(e.second), rowsOf(l), l->words.size(), l->pairs, us);
        }
    };
    auto keep = [&](const std::u32string &ctx, const std::u32string &word) {
        std::u32string before = ctx;
        for (const std::u32string &w : words(word)) {
            if (checkpointEvery <= 0) {
                engine.learn(learned, before, w);
                before += w + U' ';
                continue;
            }
            engine.learn(learned, before, w);
            before += w + U' ';
            if (++keptWords % 50 == 0) {
                const Learned::Changes c = learned.takeChanges();
                if (c.rewrite.empty())
                    maxSaveRows = std::max(maxSaveRows, c.words.size() + c.pairs.size());
            }
            if (keptWords % checkpointEvery == 0)
                checkpoint();
        }
    };
    for (size_t ti = 0; ti < texts.size(); ++ti) {
        currentText = ti;
        if (learn && ti > 0 && ti % static_cast<size_t>(textLines) == 0) {
            learned.endText();
            maintain();   // as the plugin when the keyboard closes
        }
        if (static_cast<int>(ti) < warmTexts) {   // -V: typed and learned, not counted
            std::u32string context;
            for (size_t wi = 0; wi < texts[ti].size(); ++wi) {
                if (learn) {
                    engine.startText(context, prior);
                    keep(context, texts[ti][wi]);
                }
                context += texts[ti][wi] + (textEnds[ti][wi] ? U". " : U" ");
            }
            continue;
        }
        ++sentences;
        std::vector<std::u32string> ws = texts[ti];
        std::vector<bool> ends = textEnds[ti];
        std::vector<bool> isUnknown(ws.size(), false);
        if (!unknown.empty() && !pairsAlone) {
            for (size_t i = 0; i <= ws.size(); ++i)
                if (coin(typos.rng()) < unknownRate) {
                    ws.insert(ws.begin() + i, unknown[typos.pos(unknown.size())]);
                    isUnknown.insert(isUnknown.begin() + i, true);
                    ends.insert(ends.begin() + i, false);
                    ++i;
                }
        }
        // What the context gets after word wi.
        auto after = [&](size_t wi) { return std::u32string(ends[wi] ? U". " : U" "); };
        std::u32string context;
        for (size_t wi = 0; wi < ws.size(); ++wi) {
            std::u32string target = ws[wi];
            const bool forcedHere = pairsAlone && wi + 1 == ws.size();
            if (pairsAlone && !forcedHere) {   // context only
                context += target + after(wi);
                continue;
            }
            const bool plain = forcedHere || isPlain(target);
            if (plain) {
                Kind kind = isUnknown[wi] ? Unknown : Clean;
                std::u32string typed = target;
                std::u32string next;   // the word typed with this one (Joined, SpaceLetter)
                // Uncertain: the letter with two keys, the key not shown, and
                // whether the shown one is where the finger lifted.
                int uncertainAt = -1;
                char32_t uncertainOther = 0;
                bool shownLifted = true;
                // A real misspelling (-W): of this word, or of it and the next.
                const Misspelling *missp = nullptr;
                if (forcedHere) {
                    missp = &pairs[ti].second;
                } else if (!misspellings.empty() && kind == Clean) {
                    auto it = misspellings.end();
                    bool two = false;
                    if (wi + 1 < ws.size() && isPlain(ws[wi + 1]) && !isUnknown[wi + 1]) {
                        it = misspellings.find(lower(target + U" " + ws[wi + 1]));
                        two = it != misspellings.end();
                    }
                    if (!two)
                        it = misspellings.find(lower(target));
                    if (it != misspellings.end() && misspUsed[it->first] < misspCap && coin(typos.rng()) < misspRate) {
                        ++misspUsed[it->first];
                        missp = &it->second[typos.pos(it->second.size())];
                        if (two)
                            target += U" " + ws[++wi];
                    }
                }
                if (missp) {
                    kind = missp->accentsOnly ? MisspelledAccents : Misspelled;
                    typed = missp->typed;
                    // The capital of a sentence start, typed with Shift.
                    if (utf8::isUpper(target[0]))
                        typed[0] = utf8::toUpper(typed[0]);
                }
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
                    } else if (kind == Uncertain) {
                        const size_t i = typos.pos(target.size());
                        const char32_t meant = utf8::toLower(target[i]);
                        const char32_t other = typos.wrongKey(meant);
                        if (other == 0) {
                            typed = typos.apply(target, kind = Neighbour);
                        } else {
                            const bool liftOnMeant = coin(typos.rng()) < 0.5;
                            const char32_t lifted = liftOnMeant ? meant : other;
                            const char32_t touched = liftOnMeant ? other : meant;
                            engine.startText(context, prior);
                            const char32_t shown = engine.resolveTap(lower(target.substr(0, i)), lifted, touched);
                            typed = target;
                            typed[i] = utf8::isUpper(target[i]) ? utf8::toUpper(shown) : shown;
                            uncertainAt = static_cast<int>(i);
                            shownLifted = shown == lifted;
                            uncertainOther = shownLifted ? touched : lifted;
                        }
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
                if ((firstOnly && !context.empty()) || static_cast<int>(ti) < skipTexts) {
                    context += target + after(wi);
                    continue;   // the same typos as without -F; -k: another shard's text
                }
                engine.startText(context, prior);
                const auto t0 = std::chrono::steady_clock::now();
                std::vector<Tap> taps = tapsFor(typed);
                if (keepBoth && uncertainAt >= 0 && uncertainAt < static_cast<int>(taps.size())) {
                    taps[uncertainAt].initial = uncertainOther;
                    taps[uncertainAt].chLifted = shownLifted;
                }
                // With -Y the search once, then the decision for each setting.
                Searched found;
                Result r0, rv;
                if (variants.empty()) {
                    r0 = engine.process(taps);
                } else {
                    found = engine.search(taps);
                    r0 = engine.decide(found, params);
                }
                const double us = std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - t0).count();
                totalUs += us;
                maxUs = std::max(maxUs, us);
                ++searches;

                for (size_t v = 0; v <= variants.size(); ++v) {
                const Result &r = v == 0 ? r0 : (rv = engine.decide(found, variants[v - 1]));
                Tally &tally = tallies[v];
                Stats *stats = tally.stats;
                GroupStats *groups = tally.groups;
                LengthStats *lengths = tally.lengths;
                LengthStats &capitals = tally.capitals;
                int &typedWords = tally.typedWords, &wrongBefore = tally.wrongBefore, &wrongAfter = tally.wrongAfter;
                const bool verbose = verboseOpt && v == 0;
                Stats &s = stats[kind];
                ++s.total;
                // A leading digit is typed as it is, and so are the letters
                // after it (the handler: 2nd, 5om).
                const bool fired = r.autocorrect >= 0 && !(typed[0] >= U'0' && typed[0] <= U'9');
                const std::u32string got = fired ? r.suggestions[r.autocorrect].word : typed;
                if (outcomes && ((kind != Clean && kind != Unknown) || lower(got) != lower(target)))
                    std::fprintf(outcomes, "%zu\t%zu\t%zu\t%s\t%s\t%s\t%s\n", v, ti, wi, kindName(kind),
                                 utf8::encode(typed).c_str(), utf8::encode(target).c_str(), utf8::encode(got).c_str());
                if (dump && v == 0 && (dumpAll || (kind != Clean && kind != Unknown) || lower(got) != lower(target))) {
                    int rank = -1;
                    for (size_t k = 0; k < r.suggestions.size() && rank < 0; ++k)
                        if (lower(r.suggestions[k].word) == lower(target))
                            rank = static_cast<int>(k);
                    const double lp = wordLogProb(target);
                    std::fprintf(dump, "{\"s\":%zu,\"w\":%zu,\"kind\":\"%s\",\"typed\":%s,\"target\":%s,\"got\":%s,"
                                       "\"fired\":%s,\"valid\":%s,\"capital\":%s,\"conf\":%.4f,\"unknown\":%.4f,"
                                       "\"reason\":\"%s\",\"target_lp\":%s,\"target_rank\":%d,\"cands\":[",
                                 ti, wi, kindName(kind), json(typed).c_str(), json(target).c_str(),
                                 json(got).c_str(), fired ? "true" : "false",
                                 r.typedIsWord ? "true" : "false",
                                 !context.empty() && context.compare(context.size() - 2, 2, U". ") != 0 && utf8::isUpper(typed[0]) ? "true" : "false",
                                 r.confidence, r.unknownShare, reasonName(r.reason),
                                 lp > 0 ? "null" : std::to_string(lp).c_str(), rank);
                    for (size_t k = 0; k < r.suggestions.size() && k < 8; ++k) {
                        const Suggestion &c = r.suggestions[k];
                        const double clp = wordLogProb(c.word);
                        std::fprintf(dump, "%s[%s,%.3f,%s,%d,%d]", k ? "," : "", json(c.word).c_str(), c.cost,
                                     clp > 0 ? "null" : std::to_string(clp).c_str(), c.changes, c.completion ? 1 : 0);
                    }
                    std::fprintf(dump, "]}\n");
                }
                const int group = context.empty() ? First : typed.size() <= 2 ? Short : Rest;
                GroupStats &g = groups[group];
                char tag[48] = "";
                if (breakdown)
                    std::snprintf(tag, sizeof tag, " [%s, %.2f]", groupNames[group], r.confidence);
                ++typedWords;
                if (lower(typed) != lower(target)) ++wrongBefore;
                if (lower(got) != lower(target)) ++wrongAfter;
                // By length, and the words typed with a capital after a
                // word (names, German nouns) once more.
                for (LengthStats *lp : {&lengths[typed.size() <= 2 ? Len2 : typed.size() <= 4 ? Len34 : Len5],
                                        !context.empty() && utf8::isUpper(typed[0]) ? &capitals : nullptr}) {
                    if (!lp)
                        continue;
                    LengthStats &ls = *lp;
                    if (kind == Clean) {
                        ++ls.clean;
                        if (lower(got) != lower(target)) ++ls.cleanWrong;
                    } else if (kind == Unknown) {
                        ++ls.unknown;
                        if (lower(got) != lower(target)) ++ls.unknownWrong;
                    } else if (!isMisspelled(kind) && !r.typedIsWord && target.find(U' ') == std::u32string::npos) {
                        ++ls.fixable;
                        if (fired && lower(got) == lower(target)) ++ls.fixed;
                        if (fired && lower(got) != lower(target)) ++ls.wrong;
                    }
                }
                if (kind == Clean) {
                    ++g.clean;
                } else if (kind != Unknown && !isMisspelled(kind) && !r.typedIsWord) {
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
                }   // variants
                if (learn) {
                    // The user notices a wrong word, mostly: kept as meant.
                    const bool fired = r0.autocorrect >= 0 && !(typed[0] >= U'0' && typed[0] <= U'9');
                    const std::u32string got = fired ? r0.suggestions[r0.autocorrect].word : typed;
                    const bool unnoticed = lower(got) != lower(target)
                                           && std::uniform_real_distribution<double>(0, 1)(learnRng) < typoKeepRate;
                    keep(context, unnoticed ? got : target);
                }
            } else if (learn) {
                keep(context, target);
            }
            context += target + after(wi);
        }
    }

    const Stats *stats = tallies[0].stats;
    const GroupStats *groups = tallies[0].groups;
    const LengthStats *lengths = tallies[0].lengths, &capitals = tallies[0].capitals;
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
        if (isMisspelled(k))
            continue;   // apart from the slips
        all.total += s.total; all.realWord += s.realWord; all.top1 += s.top1; all.top3 += s.top3;
        all.fixed += s.fixed; all.wrong += s.wrong;
    }
    const int fixable = all.total - all.realWord;
    std::printf("%-14s %6d %7.1f%% %6.1f%% %6.1f%% %9.1f%% %9.1f%%\n", "all typos", all.total,
                pct(all.realWord, all.total), pct(all.top1, fixable), pct(all.top3, fixable),
                pct(all.fixed, fixable), pct(all.wrong, fixable));
    // Real misspellings: fixed, wrong, left (of those no loaded language
    // takes as a word).
    auto misspText = [&](int k) {
        const Stats &s = stats[k];
        const int n = s.total - s.realWord;
        char buf[160];
        std::snprintf(buf, sizeof buf, "fixed %.1f%% wrong %.1f%% left %.1f%%", pct(s.fixed, n), pct(s.wrong, n),
                      pct(n - s.fixed - s.wrong, n));
        return std::string(buf);
    };
    if (!misspPath.empty()) {
        for (int k : {Misspelled, MisspelledAccents})
            std::printf("%-14s %6d typed, %d words in some language, of the rest %s\n", kindName(k), stats[k].total,
                        stats[k].realWord, misspText(k).c_str());
        const long used = std::count_if(misspUsed.begin(), misspUsed.end(), [](const auto &u) { return u.second > 0; });
        std::printf("misspelled words: %ld, of %zu in the list\n", pairsAlone ? long(misspellings.size()) : used,
                    misspellings.size());
    }
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
    if (byLength) {
        for (int k = 0; k <= LengthCount; ++k) {
            const LengthStats &l = k < LengthCount ? lengths[k] : capitals;
            std::printf("%-10s typos %5d fixed %5.1f%% wrong %4.1f%%, correct %6d changed %.2f%%,"
                        " unknown %5d changed %.2f%%\n",
                        k < LengthCount ? (std::string("length ") + lengthNames[k]).c_str() : "capital",
                        l.fixable, pct(l.fixed, l.fixable),
                        pct(l.wrong, l.fixable), l.clean, pct(l.cleanWrong, l.clean), l.unknown,
                        pct(l.unknownWrong, l.unknown));
        }
    }
    if (learn) {
        size_t newWords = 0, pending = 0, pairs = 0, used = 0;
        std::vector<std::string> typosLearned;
        for (const std::string &code : learned.languages()) {
            const Learned::Language *l = learned.language(code);
            pairs += l->pairs;
            for (const auto &w : l->words) {
                (w.second.pending ? pending : w.second.isNew ? newWords : used) += 1;
                if (!w.second.pending && w.second.isNew && !textWords.count(w.first))
                    typosLearned.push_back(utf8::encode(w.second.spelling));
            }
        }
        std::string sample;
        for (size_t k = 0; k < typosLearned.size() && k < 10; ++k)
            sample += " " + typosLearned[k];
        std::printf("learned: %zu words counted, %zu new words (%zu pending), %zu pairs; learned typos %zu%s\n",
                    used, newWords, pending, pairs, typosLearned.size(), sample.c_str());
    }
    std::printf("time per word: mean %.0f µs, max %.0f µs\n", totalUs / searches, maxUs);
    for (size_t v = 0; v < tallies.size() && printCounts; ++v) {
        std::string line;
        for (long x : summaryCounts(tallies[v], !misspPath.empty()))
            line += " " + std::to_string(x);
        std::printf("counts%s:%s\n", v ? ("[" + variantSpecs[v - 1] + "]").c_str() : "", line.c_str());
    }
    for (size_t v = 1; v < tallies.size(); ++v)
        std::printf("summary[%s]: %s\n", variantSpecs[v - 1].c_str(), summaryText(tallies[v], !misspPath.empty()).c_str());
    std::printf("summary: %s\n", summaryText(tallies[0], !misspPath.empty()).c_str());
    return 0;
}
