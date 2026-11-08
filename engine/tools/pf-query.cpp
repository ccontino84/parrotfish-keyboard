// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

// Try the corrector by hand:
//   pf-query -l data/layouts/phone-qwerty.layout -d build/data/en.pfd teh wrk helo
// Words come from the arguments, or one per line from stdin.

#include "parrotfish/corrector.h"
#include "parrotfish/utf8.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace parrotfish;

int main(int argc, char **argv)
{
    std::string layoutPath, dictPath;
    std::vector<std::string> words;
    bool completions = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-l" && i + 1 < argc)
            layoutPath = argv[++i];
        else if (a == "-d" && i + 1 < argc)
            dictPath = argv[++i];
        else if (a == "-c")
            completions = true;
        else
            words.push_back(a);
    }
    if (layoutPath.empty() || dictPath.empty()) {
        std::fprintf(stderr, "usage: pf-query -l LAYOUT -d DICT [-c] [WORD...]\n");
        return 2;
    }
    std::ifstream lf(layoutPath);
    std::stringstream ls;
    ls << lf.rdbuf();
    std::string error;
    KeyModel keys = KeyModel::parse(ls.str(), &error);
    if (keys.keys().empty()) {
        std::fprintf(stderr, "%s: %s\n", layoutPath.c_str(), error.c_str());
        return 1;
    }
    Dictionary dict;
    if (!dict.open(dictPath, &error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    CorrectorParams params;
    params.completions = completions;
    Corrector corrector(dict, keys, params);

    auto run = [&](const std::string &w) {
        auto t0 = std::chrono::steady_clock::now();
        auto cands = corrector.search(tapsFor(utf8::decode(w)));
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - t0).count();
        const uint32_t node = dict.find(utf8::decode(w));
        const bool valid = node && !(dict.attributes(node) & format::AttrMisspelling);
        std::printf("%s%s  (%lld µs)\n", w.c_str(), valid ? " [word]" : "", static_cast<long long>(us));
        for (const Candidate &c : cands)
            std::printf("  %-20s %6.2f  tap %5.2f word %5.2f  e%d%s%s\n",
                        utf8::encode(c.word).c_str(), c.cost, c.tapCost, c.wordCost, c.edits,
                        c.extraLetters ? " +" : "", c.viaTarget ? " →" : "");
    };
    if (words.empty()) {
        std::string line;
        while (std::getline(std::cin, line))
            if (!line.empty())
                run(line);
    } else {
        for (const auto &w : words)
            run(w);
    }
    return 0;
}
