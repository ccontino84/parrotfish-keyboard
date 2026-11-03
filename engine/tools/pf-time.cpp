// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

// Time the engine as the keyboard runs it (all typing languages,
// completions on, Normal), on the phone or the PC:
//   pf-time -l LAYOUT -d DICT [-d DICT]... -t TEXT [-n LINES] [-p PAUSE_MS]
//           [-L LEARN_TEXT [-N LINES]] [-P name=value]...
// Each word of TEXT (one text a line) is typed letter by letter, taps on
// the keys; every letter's search is timed (Engine::process), and the next
// words before each word (Engine::predict). -p waits PAUSE_MS before each
// letter, as between taps (the CPU clock drops). -L first learns LEARN_TEXT
// as one user (Engine::learn, one text a line, at most -N lines), as the
// keyboard will with learning on. -P sets an engine parameter. Prints the
// times by word length.

#include "parrotfish/engine.h"
#include "parrotfish/utf8.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <thread>

using namespace parrotfish;

namespace {

std::vector<std::u32string> readLines(const std::string &path, int max)
{
    std::vector<std::u32string> out;
    std::ifstream in(path);
    for (std::string line; static_cast<int>(out.size()) < max && std::getline(in, line);)
        if (!line.empty() && line[0] != '#')
            out.push_back(utf8::decode(line));
    return out;
}

bool letters(const std::u32string &w)
{
    return std::none_of(w.begin(), w.end(), [](char32_t c) { return c >= U'0' && c <= U'9'; });
}

struct Times {
    std::vector<double> us;
    void print(const char *label) const
    {
        if (us.empty())
            return;
        std::vector<double> s = us;
        std::sort(s.begin(), s.end());
        double sum = 0;
        for (double v : s)
            sum += v;
        std::printf("%-12s %6zu  mean %7.0f  median %7.0f  95%% %7.0f  max %7.0f µs\n", label, s.size(),
                    sum / s.size(), s[s.size() / 2], s[s.size() * 95 / 100], s.back());
    }
};

} // namespace

int main(int argc, char **argv)
{
    std::string layoutPath, textPath, learnPath;
    std::vector<std::string> dictPaths, settings;
    int maxLines = 50, learnLines = 2000, pauseMs = 0;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "-l" && i + 1 < argc) layoutPath = argv[++i];
        else if (a == "-d" && i + 1 < argc) dictPaths.push_back(argv[++i]);
        else if (a == "-t" && i + 1 < argc) textPath = argv[++i];
        else if (a == "-n" && i + 1 < argc) maxLines = std::stoi(argv[++i]);
        else if (a == "-p" && i + 1 < argc) pauseMs = std::stoi(argv[++i]);
        else if (a == "-L" && i + 1 < argc) learnPath = argv[++i];
        else if (a == "-N" && i + 1 < argc) learnLines = std::stoi(argv[++i]);
        else if (a == "-P" && i + 1 < argc) settings.push_back(argv[++i]);
    }
    std::ifstream lf(layoutPath);
    std::stringstream ls;
    ls << lf.rdbuf();
    std::string error;
    const KeyModel keys = KeyModel::parse(ls.str(), &error);
    EngineParams params;
    params.corrector.completions = true;
    params.strength = Strength::Normal;
    for (const std::string &p : settings)
        if (!setParam(params, p)) {
            std::fprintf(stderr, "unknown parameter %s\n", p.c_str());
            return 2;
        }
    Engine engine(params);
    for (const std::string &d : dictPaths)
        if (!engine.addLanguage(d, &error))
            std::fprintf(stderr, "%s\n", error.c_str());
    const std::vector<std::u32string> text = readLines(textPath, maxLines);
    if (keys.keys().empty() || engine.languageCount() == 0 || text.empty()) {
        std::fprintf(stderr, "usage: pf-time -l LAYOUT -d DICT [-d DICT]... -t TEXT [-n LINES] [-p PAUSE_MS]"
                             " [-L LEARN_TEXT [-N LINES]]\n");
        return 2;
    }
    engine.setLayout(keys);

    Learned learned;
    if (!learnPath.empty()) {
        const auto t0 = std::chrono::steady_clock::now();
        size_t n = 0;
        for (const std::u32string &line : readLines(learnPath, learnLines)) {
            std::u32string context;
            for (const std::u32string &w : words(line)) {
                if (letters(w)) {
                    engine.startText(context);
                    engine.learn(learned, context, w);
                    ++n;
                }
                context += w + U' ';
            }
            learned.endText();
        }
        engine.setLearned(&learned);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        size_t counted = 0, isNew = 0, pairs = 0;
        for (const std::string &code : learned.languages()) {
            const Learned::Language *l = learned.language(code);
            pairs += l->pairs;
            for (const auto &w : l->words)
                (w.second.isNew && !w.second.pending ? isNew : counted) += 1;
        }
        std::printf("learned %zu words typed in %.0f ms: %zu words, %zu new, %zu pairs\n", n, ms, counted, isNew,
                    pairs);
    }

    std::map<size_t, Times> byLength;
    Times all, next;
    for (const std::u32string &line : text) {
        std::u32string context;
        for (const std::u32string &w : words(line)) {
            if (!letters(w)) {
                context += w + U' ';
                continue;
            }
            engine.startText(context);
            auto t0 = std::chrono::steady_clock::now();
            engine.predict(context, 10);
            next.us.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count());
            for (size_t k = 1; k <= w.size(); ++k) {
                if (pauseMs > 0)
                    std::this_thread::sleep_for(std::chrono::milliseconds(pauseMs));
                const std::vector<Tap> taps = tapsFor(w.substr(0, k));
                t0 = std::chrono::steady_clock::now();
                engine.process(taps);
                const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
                byLength[std::min<size_t>(k, 10)].us.push_back(us);
                all.us.push_back(us);
            }
            context += w + U' ';
        }
    }
    for (const auto &e : byLength) {
        char label[32];
        std::snprintf(label, sizeof label, "%zu letters%s", e.first, e.first == 10 ? "+" : "");
        e.second.print(label);
    }
    all.print("all letters");
    next.print("next word");
    return 0;
}
