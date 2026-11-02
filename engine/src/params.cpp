// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD
//
// The engine's settings by name (tools, benches: -P name=value) and the
// names of Result::reason.

#include "parrotfish/engine.h"

#include <algorithm>
#include <iterator>
#include <string>

namespace parrotfish {

const char *reasonName(Reason reason)
{
    switch (reason) {
    case Reason::None: return "none";
    case Reason::Applied: return "applied";
    case Reason::CaseFix: return "case-fix";
    case Reason::NoCandidate: return "no-candidate";
    case Reason::Digits: return "digits";
    case Reason::LanguageNotSure: return "language-not-sure";
    case Reason::Offensive: return "offensive";
    case Reason::StrengthOff: return "strength-off";
    case Reason::OtherLanguage: return "other-language";
    case Reason::TypedIsWord: return "typed-is-word";
    case Reason::StrongMargin: return "strong-margin";
    case Reason::SlipShortcut: return "slip-shortcut";
    case Reason::BelowNormal: return "below-normal";
    case Reason::TooShort: return "too-short";
    case Reason::BelowGentle: return "below-gentle";
    case Reason::GentleChanges: return "gentle-changes";
    }
    return "?";
}

namespace {

// Every setting settable by name. `decision`: read only by Engine::decide,
// so tools can replay it on one search (sentence-bench -Y; strength only
// unless Strong, whose search differs). The corrector's own settings
// (CorrectorParams) and the language tracker's come first, from their
// tables.
struct Param {
    const char *name;
    bool decision;
    void (*set)(EngineParams &p, double v);
};

const Param Params[] = {
    {"validThreshold",             false, [](EngineParams &p, double v) { p.validThreshold = v; }},
    {"autocorrectConfidence",      true,  [](EngineParams &p, double v) { p.autocorrectConfidence = v; }},
    {"firstWordConfidence",        true,  [](EngineParams &p, double v) { p.firstWordConfidence = v; }},
    {"firstWordShare",             true,  [](EngineParams &p, double v) { p.firstWordShare = v; }},
    {"gentleConfidence",           true,  [](EngineParams &p, double v) { p.gentleConfidence = v; }},
    {"gentleMinLength",            true,  [](EngineParams &p, double v) { p.gentleMinLength = static_cast<size_t>(v); }},
    {"gentleShortLength",          true,  [](EngineParams &p, double v) { p.gentleShortLength = static_cast<size_t>(v); }},
    {"gentleTwoLetterConfidence",  true,  [](EngineParams &p, double v) { p.gentleTwoLetterConfidence = v; }},
    {"gentleTwoLetterAccents",     true,  [](EngineParams &p, double v) { p.gentleTwoLetterAccents = v != 0; }},
    {"changeCost",                 true,  [](EngineParams &p, double v) { p.changeCost = v; }},
    {"changeCostTwo",              true,  [](EngineParams &p, double v) { p.changeCostTwo = v; }},
    {"changeCostShort",            true,  [](EngineParams &p, double v) { p.changeCostShort = v; }},
    {"wrongCost",                  true,  [](EngineParams &p, double v) { p.wrongCost = v; }},
    {"nearWrongCost",              true,  [](EngineParams &p, double v) { p.nearWrongCost = v; }},
    {"normalMinLength",            true,  [](EngineParams &p, double v) { p.normalMinLength = static_cast<size_t>(v); }},
    {"strongMargin",               true,  [](EngineParams &p, double v) { p.strongMargin = v; }},
    {"strongMinLength",            true,  [](EngineParams &p, double v) { p.strongMinLength = static_cast<size_t>(v); }},
    {"otherLanguageMargin",        true,  [](EngineParams &p, double v) { p.otherLanguageMargin = v; }},
    {"unknownBase",                true,  [](EngineParams &p, double v) { p.unknownBase = v; }},
    {"unknownPerLetter",           true,  [](EngineParams &p, double v) { p.unknownPerLetter = v; }},
    {"unknownLetterWeight",        true,  [](EngineParams &p, double v) { p.unknownLetterWeight = v; }},
    {"unknownLetterRef",           true,  [](EngineParams &p, double v) { p.unknownLetterRef = v; }},
    {"unknownShortLength",         true,  [](EngineParams &p, double v) { p.unknownShortLength = static_cast<size_t>(v); }},
    {"unknownLongLetterCost",      true,  [](EngineParams &p, double v) { p.unknownLongLetterCost = v; }},
    {"unknownTwoChangeLength",     true,  [](EngineParams &p, double v) { p.unknownTwoChangeLength = static_cast<size_t>(v); }},
    {"unknownOddsScale",           true,  [](EngineParams &p, double v) { p.unknownOddsScale = v; }},
    {"compoundBonus",              true,  [](EngineParams &p, double v) { p.compoundBonus = v; }},
    {"strength",                   true,  [](EngineParams &p, double v) { p.strength = v <= 0 ? Strength::Off : v < 2 ? Strength::Gentle : v < 3 ? Strength::Normal : Strength::Strong; }},
    {"missingSpaceCost",           false, [](EngineParams &p, double v) { p.missingSpaceCost = v; }},
    {"spaceKeyCost",               false, [](EngineParams &p, double v) { p.spaceKeyCost = v; }},
    {"splitMinLength",             false, [](EngineParams &p, double v) { p.splitMinLength = static_cast<size_t>(v); }},
    {"tapSwitchMargin",            false, [](EngineParams &p, double v) { p.tapSwitchMargin = v; }},
    {"pairWeight",                 false, [](EngineParams &p, double v) { p.pairWeight = v; }},
    {"capitalLowerCost",           false, [](EngineParams &p, double v) { p.capitalLowerCost = v; }},
    {"capitalLowerCostNouns",      false, [](EngineParams &p, double v) { p.capitalLowerCostNouns = v; }},
    {"keyHeightShare",             false, [](EngineParams &p, double v) { p.keyHeightShare = v; }},
    {"searchMargin",               false, [](EngineParams &p, double v) { p.searchMargin = v; }},
    {"searchMinWeight",            false, [](EngineParams &p, double v) { p.searchMinWeight = v; }},
    {"completions",                false, [](EngineParams &p, double v) { p.corrector.completions = v != 0; }},
    {"exactMisspellingBonus",      false, [](EngineParams &p, double v) { p.corrector.exactMisspellingBonus = static_cast<float>(v); }},
};

} // namespace

bool isDecisionParam(const std::string &name)
{
    for (const Param &q : Params)
        if (name == q.name)
            return q.decision;
    return false;
}

bool setParam(EngineParams &p, const std::string &assignment)
{
    if (setParam(p.corrector, assignment) || setParam(p.language, assignment))
        return true;
    const size_t eq = assignment.find('=');
    if (eq == std::string::npos)
        return false;
    const std::string name = assignment.substr(0, eq);
    for (const Param &q : Params)
        if (name == q.name) {
            q.set(p, std::stod(assignment.substr(eq + 1)));
            return true;
        }
    return false;
}

} // namespace parrotfish
