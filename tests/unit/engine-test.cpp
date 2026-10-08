// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

// Unit tests for the engine. Run with ctest; needs the built dictionaries.
//   engine-test DICT_DIR LAYOUT_DIR

#include "parrotfish/engine.h"
#include "parrotfish/utf8.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

using namespace parrotfish;

namespace {

int failures = 0;
std::string dictDir, layoutDir;

#define CHECK(cond) check((cond), #cond, __FILE__, __LINE__)
#define CHECK_EQ(a, b) checkEq((a), (b), #a, __FILE__, __LINE__)

void check(bool ok, const char *what, const char *file, int line)
{
    if (!ok) {
        std::printf("%s:%d: FAILED: %s\n", file, line, what);
        ++failures;
    }
}

void checkEq(const std::string &a, const std::string &b, const char *what, const char *file, int line)
{
    if (a != b) {
        std::printf("%s:%d: FAILED: %s is \"%s\", expected \"%s\"\n", file, line, what, a.c_str(), b.c_str());
        ++failures;
    }
}

std::u32string u(const std::string &s) { return utf8::decode(s); }
std::string s8(const std::u32string &s) { return utf8::encode(s); }

KeyModel layout(const std::string &lang)
{
    std::ifstream in(layoutDir + "/" + lang + ".layout");
    std::stringstream ss;
    ss << in.rdbuf();
    return KeyModel::parse(ss.str());
}

// An engine with the given languages (first = most likely) on a layout.
struct Setup {
    Engine engine;
    explicit Setup(const std::vector<std::string> &langs, const std::string &layoutLang = "")
    {
        for (const std::string &l : langs) {
            std::string error;
            if (!engine.addLanguage(dictDir + "/" + l + ".pfd", &error))
                std::printf("cannot load %s: %s\n", l.c_str(), error.c_str());
        }
        engine.setLayout(layout(layoutLang.empty() ? langs[0] : layoutLang));
        std::vector<double> prior(langs.size(), 0.01);
        prior[0] = 1.0;
        engine.startText({}, prior);
    }
    Result type(const std::string &typed) { return engine.process(tapsFor(u(typed))); }
    // What Space would commit.
    std::string space(const std::string &typed)
    {
        const Result r = type(typed);
        return r.autocorrect >= 0 ? s8(r.suggestions[r.autocorrect].word) : typed;
    }
    std::string first(const std::string &typed)
    {
        const Result r = type(typed);
        return r.suggestions.empty() ? std::string() : s8(r.suggestions[0].word);
    }
};

void testUtf8()
{
    CHECK_EQ(s8(u("perché città Straße")), "perché città Straße");
    CHECK(utf8::toLower(U'À') == U'à');
    CHECK(utf8::toUpper(U'ü') == U'Ü');
    CHECK(utf8::toLower(U'ß') == U'ß');
    CHECK(utf8::toUpper(U'ß') == U'ß');
    CHECK(utf8::isUpper(U'É'));
}

void testWords()
{
    const auto w = words(u("L'anno scorso, don't E-Mail perché’ -- 42 fine."));
    std::vector<std::string> got;
    for (const auto &x : w) got.push_back(s8(x));
    const std::vector<std::string> want = {"L'anno", "scorso", "don't", "E-Mail", "perché", "fine"};
    CHECK(got == want);
}

void testDictionary()
{
    Dictionary it;
    CHECK(it.open(dictDir + "/it.pfd"));
    CHECK_EQ(it.language(), "it");
    CHECK(it.find(u("perché")) != 0);
    CHECK(it.find(u("PERCHÉ")) != 0);
    std::u32string stored;
    CHECK(it.find(u("italia"), &stored) != 0);
    CHECK_EQ(s8(stored), "Italia");
    std::u32string target;
    const uint32_t n = it.find(u("perche"));
    CHECK(n != 0 && (it.attributes(n, &target) & format::AttrMisspelling));
    CHECK_EQ(s8(target), "perché");
    CHECK(it.find(u("uomo")) != 0);   // from the supplement

    Dictionary missing;
    std::string error;
    CHECK(!missing.open(dictDir + "/none.pfd", &error));
    CHECK(!error.empty());
}

void testKeyModel()
{
    const KeyModel k = layout("en");
    CHECK(k.keys().size() == 26);
    for (size_t intended = 0; intended < k.keys().size(); ++intended) {
        double sum = 0;
        for (size_t tapped = 0; tapped < k.keys().size(); ++tapped)
            sum += k.probability(static_cast<int>(tapped), static_cast<int>(intended));
        CHECK(std::fabs(sum - 1.0) < 1e-4);
    }
    const int g = k.indexOf(U'g');
    std::string near;
    for (int n : k.neighbours(g)) near.push_back(static_cast<char>(k.keys()[n].ch));
    for (char c : std::string("tyfhv"))
        CHECK(near.find(c) != std::string::npos);
    CHECK(near.find('a') == std::string::npos);
    CHECK(k.probability(g, g) > k.probability(k.indexOf(U'h'), g));
}

void testCorrections()
{
    Setup en({"en"});
    CHECK_EQ(en.space("teh"), "the");
    CHECK_EQ(en.space("Teh"), "The");
    CHECK_EQ(en.first("becuase"), "because");   // offered; Gentle is not sure enough
    CHECK_EQ(en.first("dont"), "don't");
    CHECK_EQ(en.space("form"), "form");     // valid words are never replaced
    CHECK_EQ(en.space("ot"), "ot");         // Gentle leaves 2-letter input alone

    Setup it({"it"});
    CHECK_EQ(it.space("perche"), "perché");
    CHECK_EQ(it.space("perche'"), "perché");
    CHECK_EQ(it.space("citta'"), "città");
    CHECK_EQ(it.space("e"), "e");           // e and è are both words
    CHECK_EQ(it.space("collaboro"), "collaboro");

    Setup de({"de"});
    CHECK_EQ(de.space("fuer"), "für");
    CHECK_EQ(de.space("strasse"), "Straße");
    CHECK_EQ(de.space("uberhaupt"), "überhaupt");
    CHECK_EQ(de.space("haus"), "Haus");     // capitalisation fix
    CHECK_EQ(de.space("mochte"), "mochte"); // a real word: möchte is only offered
    bool offered = false;
    for (const Suggestion &s : de.type("mochte").suggestions)
        offered |= s8(s.word) == "möchte";
    CHECK(offered);
}

void testOffensive()
{
    // f=0 words are valid but never suggested unprompted.
    Dictionary en;
    CHECK(en.open(dictDir + "/en.pfd"));
    std::u32string offensive;
    en.forEachWord([&](uint32_t node, const std::u32string &w) {
        if (offensive.empty() && w.size() >= 4 && (en.attributes(node) & format::AttrOffensive))
            offensive = w;
    });
    CHECK(!offensive.empty());
    Setup s({"en"});
    std::u32string typo = offensive;
    typo.pop_back();
    for (const Suggestion &x : s.type(s8(typo)).suggestions)
        CHECK(x.word != offensive);
}

void testLanguages()
{
    Setup s({"it", "en", "de"});
    s.engine.startText(u("Domani andiamo al mare con i bambini e poi mangiamo una pizza"));
    CHECK_EQ(s.engine.language(s.engine.tracker().leading()), "it");
    CHECK(s.engine.tracker().confidence() > 0.9);
    // One foreign word doesn't switch.
    s.engine.commit(u("meeting"));
    CHECK_EQ(s.engine.language(s.engine.tracker().leading()), "it");
    // A real switch does, within a few words.
    for (const auto &w : words(u("but then we decided to stay at home because of the weather")))
        s.engine.commit(w);
    CHECK_EQ(s.engine.language(s.engine.tracker().leading()), "en");

    // `fur` is English, so in uncertain text it is left alone; in clearly
    // German text it becomes `für`.
    Setup mix({"de", "en"});
    mix.engine.startText(u("Ich habe heute keine Zeit, aber morgen komme ich gerne vorbei"));
    CHECK_EQ(mix.space("fur"), "für");
    mix.engine.startText(u("The cat has soft fur and the dog has long"));
    CHECK_EQ(mix.space("fur"), "fur");
}

void testNextWords()
{
    Setup s({"en", "it", "de"});
    auto first = [&](const std::string &text) {
        s.engine.startText(u(text));
        const auto p = s.engine.predict(u(text));
        return p.empty() ? std::string() : s8(p[0].word);
    };
    CHECK_EQ(first("Thank "), "you");
    CHECK_EQ(first("Vielen "), "Dank");          // German nouns keep their capital
    CHECK(s.engine.predict(u("Hello wor")).empty());   // inside a word: nothing
    s.engine.startText(u("Ciao, come stai? "));
    const auto start = s.engine.predict(u("Ciao, come stai? "));
    CHECK(!start.empty() && utf8::isUpper(start[0].word[0]));   // sentence start
    // A word without its own pairs falls back to frequent words.
    CHECK(!s.engine.predict(u("Thank xqzzy ")).empty());
}

void testContext()
{
    // The previous word helps: "we can akso" → "also".
    Setup en({"en", "it", "de"});
    en.engine.startText(u("I think that we can "));
    CHECK_EQ(en.space("akso"), "also");
    // Complete words only at Space: "vogkio" is "voglio", not "vogliono".
    Setup it({"it", "en", "de"});
    it.engine.startText(u("Domani andiamo al mare, io "));
    CHECK_EQ(it.space("vogkio"), "voglio");
    // Completed verb forms are words: not corrected (riuscivi ≠ riuscivo).
    it.engine.startText(u("Ieri non "));
    CHECK_EQ(it.space("riuscivi"), "riuscivi");
    CHECK_EQ(it.space("riuscivate"), "riuscivate");
}

void testOtherLanguageWords()
{
    // English text, then the Italian "Mi": a word of a typing language is
    // never replaced by a different word, not even in Normal mode.
    EngineParams p;
    p.strength = Strength::Normal;
    Engine e(p);
    for (const char *l : {"en", "it", "de"})
        e.addLanguage(dictDir + "/" + l + ".pfd");
    e.setLayout(layout("en"));
    e.startText(u("I went to the station and waited for the train for an hour. "));
    const Result r = e.process(tapsFor(u("Mi")));
    CHECK(r.autocorrect < 0);
    // Accent fixes stay: "fur" in German text becomes "für".
    e.startText(u("Ich habe heute keine Zeit, aber morgen komme ich gerne vorbei "));
    const Result r2 = e.process(tapsFor(u("fur")));
    CHECK(r2.autocorrect >= 0 && s8(r2.suggestions[r2.autocorrect].word) == "für");
}

void testUncertainTaps()
{
    Setup s({"en"});
    // Touched e, lifted on w after "qu": only "que..." makes words.
    CHECK(s.engine.resolveTap(u("qu"), U'w', U'e') == U'e');
    // The lift key stays when it fits better.
    CHECK(s.engine.resolveTap(u("th"), U'e', U'r') == U'e');
    // No word starts either way: the lift key stays.
    CHECK(s.engine.resolveTap(u("xqz"), U'w', U'e') == U'w');
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 3) {
        std::fprintf(stderr, "usage: engine-test DICT_DIR LAYOUT_DIR\n");
        return 2;
    }
    dictDir = argv[1];
    layoutDir = argv[2];
    testUtf8();
    testWords();
    testDictionary();
    testKeyModel();
    testCorrections();
    testOffensive();
    testLanguages();
    testNextWords();
    testUncertainTaps();
    testContext();
    testOtherLanguageWords();
    if (failures)
        std::printf("%d check(s) failed\n", failures);
    else
        std::printf("all checks passed\n");
    return failures ? 1 : 0;
}
