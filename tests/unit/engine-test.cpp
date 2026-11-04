// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

// Unit tests for the engine. Run with ctest; needs the built dictionaries.
//   engine-test DICT_DIR LAYOUT_DIR

#include "parrotfish/engine.h"
#include "parrotfish/learned.h"
#include "parrotfish/utf8.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
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

    // Tall keys (`pitch`): up and down far less likely in key widths, as
    // likely as across in key heights.
    const std::string tall = "pitch 1.67\n0 1 qwertyuiop\n0 1 asdfghjkl'\n1.5 1 zxcvbnm\n";
    const KeyModel w = KeyModel::parse(tall), h(w.keys(), w.sigma(), 1.0f);
    const int l = w.indexOf(U'l'), o = w.indexOf(U'o'), kk = w.indexOf(U'k');
    CHECK(w.keys().size() == 27 && std::fabs(w.keys()[l].height - 1.67f) < 1e-4);
    CHECK(w.probability(o, l) < 0.1 * w.probability(kk, l));
    CHECK(std::fabs(h.probability(o, l) - h.probability(kk, l)) < 1e-4);
    auto isNear = [](const KeyModel &m, int a, int b) {
        for (int n : m.neighbours(a)) if (n == b) return true;
        return false;
    };
    CHECK(!isNear(w, l, o) && isNear(h, l, o) && isNear(w, l, kk));
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
    // An infinitive with clitics (it.rules) stands, even at Normal.
    it.engine.setStrength(Strength::Normal);
    CHECK_EQ(it.space("fidarvi"), "fidarvi");
    CHECK_EQ(it.space("impressionarle"), "impressionarle");
    CHECK(it.engine.dictionary(0).isWordForm(u("ricordarmelo")));
    CHECK(!it.engine.dictionary(0).isWordForm(u("fralo")));   // fra is no infinitive
    CHECK_EQ(it.space("sottolineo"), "sottolineo");   // present, next to sottolineò (it.forms)
    it.engine.setStrength(Strength::Gentle);

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
    // Compounds of dictionary words stand (de.rules: compound ... unknown),
    // unless one change from a word (compoundNearBonus).
    de.engine.setStrength(Strength::Normal);
    CHECK_EQ(de.space("Hausschlüsselbund"), "Hausschlüsselbund");
    CHECK_EQ(de.space("beziehungweise"), "beziehungsweise");
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
    // German text it becomes `für` (Normal, the keyboard's default: Gentle
    // leaves it at confidence 0.89, which the benches say it should).
    Setup mix({"de", "en"});
    mix.engine.setStrength(Strength::Normal);
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
    // English text, then the Italian "Mi": a common word of a typing
    // language stays, even in Normal mode (testOtherLanguageMargin).
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

void testFirstWord()
{
    // A new text leans towards the language of the last text (×3, as the
    // plugin does): 0.6 of three languages, below autocorrectConfidence.
    auto space = [](const std::string &lean, Strength strength, const std::string &text,
                    const std::string &typed, const std::string &layoutLang = "en") {
        EngineParams p;
        p.strength = strength;
        p.corrector.completions = true;
        Engine e(p);
        for (const char *l : {"en", "it", "de"})
            e.addLanguage(dictDir + "/" + l + ".pfd");
        e.setLayout(layout(layoutLang));
        std::vector<double> prior(3, 1.0);
        if (e.languageIndex(lean) >= 0)
            prior[e.languageIndex(lean)] *= 3.0;
        e.startText(u(text), prior);
        const Result r = e.process(tapsFor(u(typed)));
        return r.autocorrect >= 0 ? s8(r.suggestions[r.autocorrect].word) : typed;
    };
    // The first word is corrected when the lean is right...
    CHECK_EQ(space("en", Strength::Gentle, "", "Tje"), "The");
    CHECK_EQ(space("en", Strength::Normal, "", "Exampke"), "Example");
    CHECK_EQ(space("it", Strength::Normal, "", "Ciap", "it"), "Ciao");
    // ...but not when it is another language, or there is no lean.
    CHECK_EQ(space("it", Strength::Gentle, "", "Tje"), "Tje");
    CHECK_EQ(space("", Strength::Gentle, "", "Tje"), "Tje");
    CHECK_EQ(space("en", Strength::Normal, "", "Ciap", "it"), "Ciap");

    // Gentle: two letters, one of them a neighbouring key (uj → un).
    CHECK_EQ(space("it", Strength::Gentle, "Ho visto ", "uj", "it"), "un");
}

void testStrong()
{
    // Strong: a valid word one slip away from a much likelier one is fixed
    // after a previous word; Normal never replaces a valid word.
    auto space = [](Strength strength, const std::string &text, const std::string &typed) {
        EngineParams p;
        p.strength = strength;
        p.corrector.completions = true;
        Engine e(p);
        for (const char *l : {"en", "it", "de"})
            e.addLanguage(dictDir + "/" + l + ".pfd");
        e.setLayout(layout("en"));
        std::vector<double> prior(3, 1.0);
        prior[e.languageIndex("en")] *= 3.0;
        e.startText(u(text), prior);
        const Result r = e.process(tapsFor(u(typed)));
        return r.autocorrect >= 0 ? s8(r.suggestions[r.autocorrect].word) : typed;
    };
    const std::string text = "I went to the station and waited for the train. I think he ";
    CHECK_EQ(space(Strength::Normal, text, "wad"), "wad");
    CHECK_EQ(space(Strength::Strong, text, "wad"), "was");
    CHECK_EQ(space(Strength::Strong, text, "was"), "was");
    CHECK_EQ(space(Strength::Strong, text, "said"), "said");
    // Not the first word of a sentence: there is no previous word.
    CHECK_EQ(space(Strength::Strong, "I went to the station and waited for the train. ", "Wad"), "Wad");
    // A single letter is never replaced (t → it was).
    CHECK_EQ(space(Strength::Normal, "I think ", "t"), "t");
    CHECK_EQ(space(Strength::Strong, "I think ", "p"), "p");

    // Only within one language: an Italian word in English text is not a
    // slip for an English word (bene: been), though each dictionary lists
    // the other's word, rarely.
    CHECK_EQ(space(Strength::Strong, "When do you leave ", "bene"), "bene");
    CHECK_EQ(space(Strength::Strong, "Have you added ", "bene"), "bene");
}

void testDigits()
{
    // A number row over the top letter row, as on the phone with it on.
    std::vector<Key> keys = layout("en").keys();
    float top = keys[0].y;
    for (const Key &k : keys)
        top = std::min(top, k.y);
    std::vector<Key> row;
    for (const Key &k : keys)
        if (k.y == top)
            row.push_back(k);
    std::sort(row.begin(), row.end(), [](const Key &a, const Key &b) { return a.x < b.x; });
    for (size_t i = 0; i < row.size() && i < 10; ++i)
        keys.push_back({U"1234567890"[i], row[i].x, top - row[i].height, row[i].width, row[i].height});
    EngineParams p;
    p.strength = Strength::Normal;
    p.corrector.completions = true;
    Engine e(p);
    e.addLanguage(dictDir + "/en.pfd");
    e.setLayout(KeyModel(keys, layout("en").sigma()));
    auto process = [&](const std::string &text, const std::string &typed) {
        e.startText(u(text), {});
        return e.process(tapsFor(u(typed)));
    };
    auto space = [&](const std::string &text, const std::string &typed) {
        const Result r = process(text, typed);
        return r.autocorrect >= 0 ? s8(r.suggestions[r.autocorrect].word) : typed;
    };
    // A slip onto the digit above a letter.
    CHECK_EQ(space("This is an ", "exampl3"), "example");
    CHECK_EQ(space("I don't know ", "ab9ut"), "about");
    // The digit is a letter typed on the number row, not an extra tap: tes5
    // is test (or rest), not yes.
    const Result r = process("I think this is the ", "tes5");
    CHECK(!r.suggestions.empty() && s8(r.suggestions[0].word) == "test");
    CHECK(std::none_of(r.suggestions.begin(), r.suggestions.end(),
                       [](const Suggestion &s) { return s8(s.word) == "yes"; }));
    // Only a letter in the digit's place: names and codes stay, and a
    // letter and a digit is always a code (a4 was at).
    for (const char *w : {"ps4", "h2o", "win7", "b2b", "a4", "f1", "i5", "g7"})
        CHECK_EQ(space("I bought a ", w), w);
}

void testOtherLanguageMargin()
{
    // English text with Italian and German loaded: a typo that happens to be
    // a rare word of another language is fixed; a common word of another
    // language, typed on purpose, stays.
    auto space = [](const std::string &text, const std::string &typed, double margin = 12.0) {
        EngineParams p;
        p.strength = Strength::Normal;
        p.corrector.completions = true;
        p.otherLanguageMargin = margin;
        Engine e(p);
        for (const char *l : {"en", "it", "de"})
            e.addLanguage(dictDir + "/" + l + ".pfd");
        e.setLayout(layout("en"));
        std::vector<double> prior(3, 1.0);
        prior[e.languageIndex("en")] *= 3.0;
        e.startText(u(text), prior);
        const Result r = e.process(tapsFor(u(typed)));
        return r.autocorrect >= 0 ? s8(r.suggestions[r.autocorrect].word) : typed;
    };
    const std::string text = "We went to the station and waited for the train. I think ";
    CHECK_EQ(space(text, "thier"), "their");   // Thier: a German surname
    CHECK_EQ(space(text, "tp"), "to");         // TP: an abbreviation
    CHECK_EQ(space(text, "thier", std::numeric_limits<double>::infinity()), "thier");   // the old block
    CHECK_EQ(space(text, "un"), "un");
    CHECK_EQ(space(text, "ti"), "ti");
    CHECK_EQ(space(text, "problema"), "problema");
}

void testSplits()
{
    // Two words typed as one: the space left out or typed as a key above
    // the spacebar. A word of a typing language is never split.
    EngineParams p;
    p.strength = Strength::Normal;
    p.corrector.completions = true;
    Engine e(p);
    for (const char *l : {"en", "it", "de"})
        e.addLanguage(dictDir + "/" + l + ".pfd");
    e.setLayout(layout("en"));
    std::vector<double> prior(3, 1.0);
    prior[e.languageIndex("en")] *= 3.0;
    auto space = [&](const std::string &text, const std::string &typed) {
        e.startText(u(text), prior);
        const Result r = e.process(tapsFor(u(typed)));
        return r.autocorrect >= 0 ? s8(r.suggestions[r.autocorrect].word) : typed;
    };
    const std::string text = "We went to the station and waited for the train. I think ";
    CHECK_EQ(space(text, "thisis"), "this is");
    CHECK_EQ(space(text, "thisnis"), "this is");   // n: a key above the spacebar
    CHECK_EQ(space(text, "into"), "into");          // a word: never split
    CHECK_EQ(space(text, "together"), "together");
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
    // Touched f, lifted on d after "le": "led..." fits better then, so the
    // word shows "ledt"; with both keys kept, the whole word is "left",
    // surer than from the letters shown alone.
    CHECK(s.engine.resolveTap(u("le"), U'd', U'f') == U'd');
    s.engine.startText(u("I think that "));
    std::vector<Tap> taps = tapsFor(u("ledt"));
    const Result shown = s.engine.process(taps);
    taps[2].initial = U'f';
    const Result r = s.engine.process(taps);
    CHECK(r.autocorrect >= 0 && s8(r.suggestions[r.autocorrect].word) == "left");
    CHECK(r.confidence > shown.confidence + 0.05);
    // The same tap, shown as the touch-down key (chLifted false): the lift
    // key keeps its weight.
    std::vector<Tap> shownTouched = tapsFor(u("left"));
    shownTouched[2].initial = U'd';
    shownTouched[2].chLifted = false;
    CHECK(s.engine.process(shownTouched).autocorrect < 0);   // left is a word: stays
}

} // namespace

void testOldSpellings()
{
    // Brazilian Portuguese: the AOSP list predates the 2009 reform
    // (data/rules/pt_BR.respellings). The current spelling stays, the old
    // one becomes it.
    Setup br({"pt_BR", "en"}, "en");
    br.engine.startText(u("Eu tenho "), {1.0, 0.01});
    br.engine.setStrength(Strength::Normal);
    for (const char *w : {"ideias", "cinquenta", "voo", "heroico", "assembleia", "frequente"})
        CHECK_EQ(br.space(w), w);
    CHECK_EQ(br.space("idéias"), "ideias");
    CHECK_EQ(br.space("cinqüenta"), "cinquenta");
    CHECK_EQ(br.space("vôo"), "voo");
    CHECK_EQ(br.space("heróico"), "heroico");
    CHECK_EQ(br.space("papéis"), "papéis");     // oxytone: keeps its accent
    // The past of -ar verbs (pt_BR.paradigms), with c/g/ç spelt before e.
    for (const char *w : {"telefonei", "instalei", "fiquei", "paguei", "comecei"})
        CHECK_EQ(br.space(w), w);
    // European Portuguese: both spellings of silent consonants in use.
    Setup pt({"pt_PT", "en"}, "en");
    pt.engine.startText(u("Eu tenho a "), {1.0, 0.01});
    pt.engine.setStrength(Strength::Normal);
    for (const char *w : {"perceção", "percepção", "ação", "acção", "facto", "contacto"})
        CHECK_EQ(pt.space(w), w);
    CHECK_EQ(pt.space("heróico"), "heroico");
    // German: spellings of before 1996 from older texts.
    Setup de({"de", "en"}, "de");
    de.engine.startText(u("Ich weiß, "), {1.0, 0.01});
    de.engine.setStrength(Strength::Normal);
    CHECK_EQ(de.space("daß"), "dass");
    CHECK_EQ(de.space("Fluß"), "Fluss");
    CHECK_EQ(de.space("Fuß"), "Fuß");           // long vowel: ß stays
    // Romanian: î inside a word before 1993.
    Setup ro({"ro", "en"}, "en");
    ro.engine.setStrength(Strength::Normal);
    CHECK_EQ(ro.space("cînd"), "când");
    // Spanish: imperatives with pronouns take an accent; the list had some
    // without (es.respellings), others only in conversation (one-source
    // tatoeba). Verb forms beside a noun with an accent stay (titulo).
    Setup es({"es", "en"}, "en");
    es.engine.startText(u("Por favor, "), {1.0, 0.01});
    es.engine.setStrength(Strength::Normal);
    for (const char *w : {"muéstrame", "déjame", "cuéntame", "titulo", "calculo"})
        CHECK_EQ(es.space(w), w);
    CHECK_EQ(es.space("muestrame"), "muéstrame");
    CHECK_EQ(es.space("olvidate"), "olvidate");   // olvídate (rarer than olvidaré) is offered
    CHECK_EQ(es.space("dejame"), "déjame");
}

void testMoreLanguages()
{
    // Danish: aa, oe, ae stand for å, ø, æ, and a plain o reaches ø.
    Setup da({"da", "en"}, "en");
    CHECK_EQ(da.space("paa"), "på");
    CHECK_EQ(da.first("sondag"), "søndag");
    // Missing from the AOSP list, added by the supplement: never "I".
    CHECK_EQ(da.space("vi"), "vi");
    // Romanian: cedilla letters match the comma-below ones of the list.
    Setup ro({"ro"}, "en");
    CHECK_EQ(ro.first("aşa"), "așa");
    // Spanish: a plain verb form stays (acelero, "I accelerate").
    Setup es({"es"}, "en");
    CHECK_EQ(es.space("acelero"), "acelero");
    CHECK_EQ(es.space("manana"), "mañana");

    // French on AZERTY, where ç has no key: typed as c, it comes back.
    Setup fr({"fr", "en"}, "phone-azerty");
    fr.engine.startText(u("Je ne sais pas, mais "), {1.0, 0.01});
    fr.engine.setStrength(Strength::Normal);
    CHECK_EQ(fr.space("ca"), "ça");
    CHECK_EQ(fr.space("garcon"), "garçon");
    CHECK_EQ(fr.space("jai"), "j'ai");   // English has "jai": only an apostrophe added

    // Finnish: forms the list lacks, made by the rules of fi.rules, stand.
    Setup fi({"fi", "en"}, "phone-nordic");
    fi.engine.setStrength(Strength::Normal);
    const Dictionary &fd = fi.engine.dictionary(0);
    CHECK(fd.isWordForm(u("kännykkänsä")));     // kännykkä + nsä
    CHECK(fd.isWordForm(u("Professorinsa")));   // professori + nsa
    CHECK(fd.isWordForm(u("parkkipaikkaa")));   // parkki + paikkaa
    CHECK(fd.isWordForm(u("juna-asemalta")));
    CHECK(!fd.isWordForm(u("talo")));           // a word of the list
    CHECK(!fd.isWordForm(u("vani")));           // stem too short
    CHECK(!fd.isWordForm(u("tomni")));          // possessive after a consonant
    CHECK(!fd.isWordForm(u("kirjsn")));
    CHECK_EQ(fi.space("kännykkänsä"), "kännykkänsä");
    CHECK_EQ(fi.space("parkkipaikkaa"), "parkkipaikkaa");
    CHECK_EQ(fi.space("kirjsn"), "kirjan");
    // Other languages have no such rules.
    CHECK(!fi.engine.dictionary(1).hasWordForms());

    // The typing languages can be replaced.
    Engine e;
    CHECK(e.addLanguage(dictDir + "/en.pfd"));
    e.clearLanguages();
    CHECK(e.languageCount() == 0);
    CHECK(e.process(tapsFor(u("teh"))).suggestions.empty());
    CHECK(e.addLanguage(dictDir + "/it.pfd"));
    e.setLayout(layout("it"));
    CHECK_EQ(e.language(0), "it");
    CHECK(!e.process(tapsFor(u("ciap"))).suggestions.empty());
}

void testSingleLetterCase()
{
    Setup s({"en", "it", "de"});
    for (Strength st : {Strength::Normal, Strength::Gentle, Strength::Strong}) {
        s.engine.setStrength(st);
        s.engine.startText(u("Yesterday after work "));
        Result r = s.engine.process(tapsFor(u("i")));
        CHECK(r.autocorrect == 0 && r.suggestions[0].word == u("I"));
        // Italian text: i is "the".
        s.engine.startText(u("Ieri dopo il lavoro con "));
        r = s.engine.process(tapsFor(u("i")));
        CHECK(r.autocorrect < 0);
        // Other single letters stay.
        s.engine.startText(u("Yesterday after work "));
        CHECK(s.engine.process(tapsFor(u("u"))).autocorrect < 0);
    }
    s.engine.setStrength(Strength::Normal);
}

void testContractions()
{
    Setup s({"en"});
    for (Strength st : {Strength::Normal, Strength::Strong}) {
        s.engine.setStrength(st);
        for (const auto &c : std::vector<std::pair<const char *, const char *>>{
                 {"cant", "can't"}, {"wont", "won't"}, {"whats", "what's"}}) {
            s.engine.startText(u("Yesterday after work "));
            Result r = s.engine.process(tapsFor(u(c.first)));
            CHECK(r.autocorrect == 0 && r.suggestions[0].word == u(c.second));
        }
        // Words in their own right stay (their contraction is a suggestion).
        for (const char *w : {"id", "ill", "lets"}) {
            s.engine.startText(u("Yesterday after work "));
            CHECK(s.engine.process(tapsFor(u(w))).autocorrect < 0);
        }
        // A listed misspelling typed as it is: its word, not a slip onto a
        // more frequent one (im: in).
        s.engine.startText(u("Yesterday after work "));
        Result r = s.engine.process(tapsFor(u("im")));
        CHECK(r.autocorrect == 0 && r.suggestions[0].word == u("I'm"));
    }
    s.engine.setStrength(Strength::Normal);
    // Everyday frequencies (find-boosts.py, build-dict.py): written-text
    // function words come down, conversation words go up. AOSP: in 210,
    // I'm 116; Tatoeba in:I'm 3.5:1, about 15-20 steps.
    {
        const Dictionary &d = s.engine.dictionary(0);
        auto freq = [&](const char *w) {
            const uint32_t n = d.findExact(u(w));
            return n ? static_cast<int>(d.node(n).freq) : -1;
        };
        CHECK(freq("in") < 210);
        CHECK(freq("in") - freq("I'm") <= 25);
    }

    // German text: im is a word.
    Setup de({"de", "en"});
    de.engine.setStrength(Strength::Normal);
    de.engine.startText(u("Wir waren gestern "));
    CHECK(de.engine.process(tapsFor(u("im"))).autocorrect < 0);
    // Everyday frequencies (find-boosts.py): süß is common in conversation
    // (Tatoeba aus:süß 40:1, the AOSP list e^10:1), so suss is not taken for
    // a slip onto aus.
    de.engine.startText(u("Das ist "));
    CHECK(de.space("suss") != "aus");

    // Letters that are a word only with a capital (the name Jaime, the
    // acronym MN) are not capitalised when another reading is likelier:
    // j'aime; m'n is offered, mn stays. A word stored with its capital is
    // still capitalised (haus: Haus).
    Setup fr({"fr", "en"});
    fr.engine.setStrength(Strength::Normal);
    fr.engine.startText(u("Je pense que "));
    CHECK_EQ(fr.space("jaime"), "j'aime");
    Setup nl({"nl", "en"}, "en");
    nl.engine.setStrength(Strength::Normal);
    nl.engine.startText(u("Ik ga met "));
    const Result mn = nl.engine.process(tapsFor(u("mn")));
    CHECK(mn.autocorrect < 0);
    CHECK(std::any_of(mn.suggestions.begin(), mn.suggestions.end(),
                      [](const Suggestion &x) { return x.word == u("m'n"); }));
    Setup de2({"de", "en"});
    de2.engine.setStrength(Strength::Normal);
    de2.engine.startText(u("Ich gehe ins "));
    CHECK_EQ(de2.space("haus"), "Haus");
}

void testUnknownWords()
{
    // The letter model: a string shaped like the language's words scores
    // better than one with unlikely letters.
    Dictionary en, it;
    CHECK(en.open(dictDir + "/en.pfd") && en.hasLetterModel());
    CHECK(it.open(dictDir + "/it.pfd") && it.hasLetterModel());
    CHECK(en.letterCost(u("norn")) + 3 < en.letterCost(u("bprn")));
    CHECK(en.letterCost(u("kowalsky")) + 10 < en.letterCost(u("xqztkwvb")));
    CHECK(it.letterCost(u("ugo")) + 2 < en.letterCost(u("ugo")));   // an Italian name
    CHECK(en.letterCost(u("nörn")) > en.letterCost(u("norn")));     // a letter it doesn't know
    CHECK(en.letterMeanCost() > 1.0 && en.letterMeanCost() < 3.0);

    // Words missing from the dictionaries: a name stays as typed, the same
    // slip in unlikely letters is still fixed, and so are typos. Long
    // letters one change from a word are that word misspelled (recieve);
    // a long name further from one stays (Wiggin), and so does one that is
    // two words joined (Welland). The letter model alone: without the cost
    // of a capital on words stored in lower case (testCapitalNames).
    EngineParams p;
    p.strength = Strength::Normal;
    p.capitalLowerCost = p.capitalLowerCostNouns = 0;
    Engine e(p);
    for (const char *l : {"en", "it", "de"})
        e.addLanguage(dictDir + "/" + l + ".pfd");
    e.setLayout(layout("en"));
    std::vector<double> prior(3, 1.0);
    prior[e.languageIndex("en")] *= 3.0;
    auto space = [&](const std::string &typed) {
        e.startText(u("Yesterday I went to see "), prior);
        const Result r = e.process(tapsFor(u(typed)));
        return r.autocorrect >= 0 ? s8(r.suggestions[r.autocorrect].word) : typed;
    };
    CHECK_EQ(space("Norn"), "Norn");       // Born with a fixed cost per letter
    CHECK_EQ(space("Farrer"), "Farrer");   // Career
    CHECK_EQ(space("Wiggin"), "Wiggin");   // Within
    CHECK_EQ(space("Welland"), "Welland"); // Well and
    CHECK_EQ(space("Bprn"), "Born");
    CHECK_EQ(space("Bqttle"), "Battle");
    CHECK_EQ(space("hpuse"), "house");
    CHECK_EQ(space("becuase"), "because");
    CHECK_EQ(space("recieve"), "receive");
    CHECK_EQ(space("goverment"), "government");
    CHECK_EQ(space("occured"), "occurred");

    // Long letters two changes from a word (two slips): that word, even
    // when they look like the language's (conoleanno in Italian).
    std::vector<double> itPrior(3, 1.0);
    itPrior[e.languageIndex("it")] *= 3.0;
    e.setLayout(layout("it"));
    e.startText(u("Domani è il mio "), itPrior);
    const Result r = e.process(tapsFor(u("conoleanno")));
    CHECK(r.autocorrect >= 0 && r.suggestions[r.autocorrect].word == u("compleanno"));
}

void testCapitalNames()
{
    // A capital after a word: a name, left alone (cant, wont and whats are
    // stored as misspellings of can't, won't, what's); at a sentence start
    // it is only the sentence's capital. Slips in unlikely letters are still
    // fixed, and so are German nouns.
    EngineParams p;
    p.strength = Strength::Normal;
    Engine e(p);
    for (const char *l : {"en", "it", "de"})
        e.addLanguage(dictDir + "/" + l + ".pfd");
    e.setLayout(layout("en"));
    std::vector<double> prior(3, 1.0);
    prior[e.languageIndex("en")] *= 3.0;
    auto space = [&](const std::string &before, const std::string &typed) {
        e.startText(u(before), prior);
        const Result r = e.process(tapsFor(u(typed)));
        return r.autocorrect >= 0 ? s8(r.suggestions[r.autocorrect].word) : typed;
    };
    CHECK_EQ(space("Yesterday I met Mr ", "Cant"), "Cant");
    CHECK_EQ(space("Yesterday I met Mr ", "Wont"), "Wont");
    CHECK_EQ(space("Yesterday I met Mr ", "Whats"), "Whats");
    CHECK_EQ(space("", "Cant"), "Can't");
    CHECK_EQ(space("It was late. ", "Cant"), "Can't");
    CHECK_EQ(space("I really ", "cant"), "can't");
    for (const char *name : {"Bartle", "Vincenzi", "Norn"})
        CHECK_EQ(space("Yesterday I went to see ", name), name);
    // A slip in a word stored with its capital is fixed (Oreland: Ireland,
    // a name too).
    CHECK_EQ(space("Yesterday I went to see ", "Lomdon"), "London");
    CHECK_EQ(space("Yesterday I went to see ", "Bqttle"), "Battle");
    CHECK_EQ(space("Yesterday I went to see ", "Hpuse"), "House");
    // All capitals: not a name's capital.
    CHECK_EQ(space("Yesterday I really ", "CANT"), "CAN'T");

    // German text: a capital is every noun's. A slip in one is fixed; a
    // word stored in lower case is less likely (Hinger: Hunger more than
    // hinter).
    std::vector<double> de(3, 1.0);
    de[e.languageIndex("de")] *= 3.0;
    e.setLayout(layout("de"));
    e.startText(u("Gestern war ich mit meinem "), de);
    Result r = e.process(tapsFor(u("Freumd")));
    CHECK(r.autocorrect >= 0 && r.suggestions[r.autocorrect].word == u("Freund"));
    e.startText(u("Ich habe "), de);
    r = e.process(tapsFor(u("Hinger")));
    CHECK(r.autocorrect < 0 && !r.suggestions.empty() && r.suggestions[0].word == u("Hunger"));
    // A rare name typed in lower case (Suess) does not keep the letters
    // as a word: suess is süß.
    e.startText(u("Der Kuchen ist wirklich sehr "), de);
    r = e.process(tapsFor(u("suess")));
    CHECK(r.autocorrect >= 0 && r.suggestions[r.autocorrect].word == u("süß"));
    // ok (stored as OK, rare in written text) stays: confidence 0.58-0.68
    // for in/il, below the two-letter threshold. A guard for step 6g.
    e.startText(u("Ja, alles "), de);
    r = e.process(tapsFor(u("ok")));
    CHECK(r.autocorrect < 0);
    std::vector<double> it(3, 1.0);
    it[e.languageIndex("it")] *= 3.0;
    e.setLayout(layout("it"));
    e.startText(u("Tutto "), it);
    r = e.process(tapsFor(u("ok")));
    CHECK(r.autocorrect < 0);
}

void testSlipShortcuts()
{
    // Strong: a valid word that AOSP lists as mostly a slip (foe: for) is
    // fixed where the previous word is known to be followed by the fix,
    // and left alone elsewhere; contractions (id: I'd) are never fixed.
    Setup s({"en", "it", "de"});
    s.engine.setStrength(Strength::Strong);
    auto space = [&](const std::string &before, const std::string &typed) {
        s.engine.startText(u(before));
        return s.space(typed);
    };
    CHECK_EQ(space("Thanks ", "foe"), "for");
    CHECK_EQ(space("I was waiting ", "foe"), "for");
    CHECK_EQ(space("He was my worst ", "foe"), "foe");
    CHECK_EQ(space("a friend or a ", "foe"), "foe");
    CHECK_EQ(space("I think ", "id"), "id");
    CHECK_EQ(space("Yesterday after work ", "ill"), "ill");
    // Normal never replaces a valid word.
    s.engine.setStrength(Strength::Normal);
    CHECK_EQ(space("Thanks ", "foe"), "foe");
}

void testFinalApostrophe()
{
    // Italian po' is a word (find-supplement.py: final-apostrophe), not
    // a slip for può; perche' still becomes perché.
    Setup s({"it", "en", "de"});
    s.engine.setStrength(Strength::Normal);
    // (Contexts long enough for the language to be sure: autocorrect
    // waits until then.)
    s.engine.startText(u("Oggi sono andato al mare e ho aspettato un "));
    CHECK_EQ(s.space("po'"), "po'");
    s.engine.startText(u("Oggi sono andato al mare ma non so "));
    CHECK_EQ(s.space("perche'"), "perché");
}

void testStrengths()
{
    // The strengths share one model; Gentle asks a larger share of the
    // probability (freind: friend at about 0.7).
    Setup s({"en"});
    s.engine.setStrength(Strength::Normal);
    s.engine.startText(u("Yesterday after work I "));
    CHECK_EQ(s.space("freind"), "friend");
    s.engine.setStrength(Strength::Gentle);
    s.engine.startText(u("Yesterday after work I "));
    CHECK_EQ(s.space("freind"), "freind");
    CHECK_EQ(s.space("becuase"), "because");
    s.engine.setStrength(Strength::Normal);
}

void testReasons()
{
    // Result::reason: which decision ended Engine::process (for tools).
    Setup s({"en"});
    auto reason = [&](const std::string &before, const std::string &typed) {
        s.engine.startText(u(before));
        return std::string(reasonName(s.type(typed).reason));
    };
    s.engine.setStrength(Strength::Normal);
    CHECK_EQ(reason("Yesterday after work I ", "freind"), "applied");
    CHECK_EQ(reason("Yesterday after work I ", "form"), "typed-is-word");
    s.engine.setStrength(Strength::Gentle);
    CHECK_EQ(reason("Yesterday after work I ", "freind"), "below-gentle");
    s.engine.setStrength(Strength::Strong);
    CHECK_EQ(reason("He was my worst ", "foe"), "slip-shortcut");
    s.engine.setStrength(Strength::Off);
    CHECK_EQ(reason("Yesterday after work I ", "freind"), "strength-off");
    Setup de({"de"});
    de.engine.setStrength(Strength::Normal);
    CHECK_EQ(std::string(reasonName(de.type("haus").reason)), "case-fix");
}

void testLearnedRules()
{
    CHECK(learnableNewWord(u("Mitton")));
    CHECK(learnableNewWord(u("NASA")));
    CHECK(!learnableNewWord(u("NASAX")));
    CHECK(!learnableNewWord(u("mp3")));
    CHECK(!learnableNewWord(u("x")));

    // Learned after 3 different texts, not 3 uses in one.
    Learned l;
    for (int k = 0; k < 3; ++k)
        l.add("en", u("Mitton"), false, u("met"), true);
    CHECK(l.language("en")->word(u("mitton")) == nullptr);
    l.endText();
    l.add("en", u("Mitton"), false, u("met"), true);
    CHECK(l.language("en")->word(u("mitton")) == nullptr);
    l.endText();
    l.add("en", u("Mitton"), false, u("met"), true);
    const Learned::Word *w = l.language("en")->word(u("mitton"));
    CHECK(w != nullptr && w->isNew && s8(w->spelling) == "Mitton");
    CHECK(l.isLearnedNew(u("MITTON")));
    CHECK(l.language("it") == nullptr);   // per language
    // No pairs with it while pending; then pairs between real words.
    CHECK(l.language("en")->next.count(u("met")) == 1);
    CHECK(l.language("en")->next.at(u("met")).at(u("mitton")) == 1);
    l.add("en", u("said"), true, u("Mitton"), false);
    CHECK(l.language("en")->next.at(u("mitton")).at(u("said")) == 1);
    l.forget(u("mitton"));
    CHECK(!l.isLearnedNew(u("Mitton")));
    CHECK(l.language("en")->next.count(u("mitton")) == 0);
    CHECK(l.language("en")->next.count(u("met")) == 0);

    // Too slow: the third text comes after the window, the count starts over.
    LearnParams p;
    p.newWordWindow = 100;
    Learned slow(p);
    slow.add("en", u("zorbel"), false, u(""), false);
    slow.endText();
    slow.add("en", u("zorbel"), false, u(""), false);
    for (int k = 0; k < 120; ++k)
        slow.add("en", u("the"), true, u(""), false);
    slow.endText();
    slow.add("en", u("zorbel"), false, u(""), false);
    CHECK(slow.language("en")->word(u("zorbel")) == nullptr);
    // A sentence start's capital is not the word's.
    slow.endText();
    slow.add("en", u("zorbel"), false, u("a"), true);
    slow.endText();
    slow.add("en", u("Zorbel"), false, u(""), false);
    w = slow.language("en")->word(u("zorbel"));
    CHECK(w != nullptr && s8(w->spelling) == "zorbel");

    // Fading by words typed.
    LearnParams f;
    f.fadeEvery = 10;
    f.fade = 0.5;
    Learned fading(f);
    fading.add("en", u("rare"), true, u(""), false);
    for (int k = 0; k < 30; ++k)
        fading.add("en", u("the"), true, u(""), false);
    CHECK(fading.language("en")->word(u("rare")) != nullptr);   // scheduled only
    CHECK(fading.maintenanceDue());
    fading.takeChanges();
    fading.maintain();   // three fadings due (typed 31)
    CHECK(!fading.maintenanceDue());
    CHECK(fading.language("en")->word(u("rare")) == nullptr);   // 1 → 0.5 → 0.25: gone
    CHECK(fading.language("en")->word(u("the")) != nullptr);
    CHECK(fading.takeChanges().rewrite.count("en") == 1);
    CHECK(fading.language("en")->nextFade == 40);

    // Trimmed over the cap: the least used, then the one kept longest ago.
    LearnParams t;
    t.maxWords = 2;
    Learned trimmed(t);
    for (const char *w : {"bbb", "aaa", "ccc", "ccc"})
        trimmed.add("en", u(w), true, u(""), false);
    trimmed.maintain();   // 3 words: over 2 + 10%
    CHECK(trimmed.language("en")->words.size() == 2);
    CHECK(trimmed.language("en")->words.count(u("bbb")) == 0);   // as often as aaa, kept before it
    CHECK(trimmed.language("en")->words.count(u("aaa")) == 1);

    // Changes for storage, and loading them back.
    Learned a;
    a.add("en", u("the"), true, u(""), false);
    a.add("en", u("house"), true, u("the"), true);
    Learned::Changes c = a.takeChanges();
    CHECK(c.words.size() == 2 && c.pairs.size() == 2 && c.languages.count("en") == 1 && !c.cleared);
    CHECK(a.takeChanges().empty());
    a.forget(u("house"));
    c = a.takeChanges();
    CHECK(c.words.count({"en", u("house")}) == 1 && c.pairs.count(std::make_tuple(std::string("en"), u("the"), u("house"))) == 1);
    for (int k = 0; k < 3; ++k) {
        a.add("en", u("Mitton"), false, u("the"), true);
        a.endText();
    }
    Learned b;
    for (const auto &w : a.language("en")->words)
        b.restoreWord("en", w.first, w.second);
    for (const auto &p : a.language("en")->next)
        for (const auto &n : p.second)
            b.restorePair("en", p.first, n.first, n.second);
    b.restoreLanguage("en", a.language("en")->typed, a.language("en")->nextFade);
    b.finishRestore();
    CHECK(b.isLearnedNew(u("mitton")));
    CHECK(b.language("en")->total == a.language("en")->total);
    CHECK(b.language("en")->pairs == a.language("en")->pairs);
    CHECK(b.takeChanges().empty());
    a.clear();
    CHECK(a.takeChanges().cleared);
}

void testLearnedEngine()
{
    Setup s({"en", "it", "de"});
    Learned l;
    s.engine.setLearned(&l);
    s.engine.startText(u("I met "));
    const std::string before = s.space("zorbel");
    // The user keeps typing it, in three texts.
    for (int k = 0; k < 3; ++k) {
        CHECK(!s.engine.isKnown(u("zorbel")));
        l.add("en", u("I"), true, u(""), false);
        l.add("en", u("met"), true, u("I"), true);
        l.add("en", u("zorbel"), false, u("met"), true);
        l.add("en", u("today"), true, u("zorbel"), false);
        l.endText();
    }
    s.engine.startText(u("I met "));
    CHECK_EQ(s.space("zorbel"), "zorbel");   // a word now: left alone
    // Suggested while typed, and as a correction.
    const Result r = s.type("zorb");
    bool suggested = false;
    for (const Suggestion &g : r.suggestions)
        suggested = suggested || s8(g.word) == "zorbel";
    CHECK(suggested);
    CHECK_EQ(s.space("zprbel"), "zorbel");
    // Next words: after "met", zorbel comes up; after zorbel, today.
    const auto next = s.engine.predict(u("I met "));
    CHECK(!next.empty() && std::any_of(next.begin(), next.end(), [](const Suggestion &g) { return s8(g.word) == "zorbel"; }));
    const auto after = s.engine.predict(u("I met zorbel "));
    CHECK(!after.empty() && s8(after[0].word) == "today");
    // A word close to a common one: corrected until learned, then left.
    s.engine.startText(u("I met "));
    CHECK_EQ(s.space("tomorow"), "tomorrow");
    for (int k = 0; k < 3; ++k) {
        l.add("en", u("tomorow"), false, u("met"), true);
        l.endText();
    }
    CHECK_EQ(s.space("tomorow"), "tomorow");
    CHECK_EQ(s.space("tomorrow"), "tomorrow");
    // Forgotten: as before.
    l.forget(u("tomorow"));
    CHECK_EQ(s.space("tomorow"), "tomorrow");
    l.forget(u("zorbel"));
    s.engine.startText(u("I met "));
    CHECK_EQ(s.space("zorbel"), before);
    // Without learned data the engine is as it was.
    s.engine.setLearned(nullptr);
    CHECK_EQ(s.space("zorbel"), before);
}

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
    testFirstWord();
    testStrong();
    testDigits();
    testOtherLanguageMargin();
    testSingleLetterCase();
    testContractions();
    testSplits();
    testUnknownWords();
    testCapitalNames();
    testStrengths();
    testSlipShortcuts();
    testFinalApostrophe();
    testReasons();
    testMoreLanguages();
    testOldSpellings();
    testLearnedRules();
    testLearnedEngine();
    if (failures)
        std::printf("%d check(s) failed\n", failures);
    else
        std::printf("all checks passed\n");
    return failures ? 1 : 0;
}
