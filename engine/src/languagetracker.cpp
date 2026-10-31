// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#include "parrotfish/languagetracker.h"

#include <algorithm>
#include <cmath>

namespace parrotfish {

LanguageTracker::LanguageTracker(std::vector<const Dictionary *> dicts, LanguageParams params)
    : m_dicts(std::move(dicts))
    , m_params(params)
{
    reset();
}

void LanguageTracker::reset(const std::vector<double> &prior)
{
    const size_t n = m_dicts.size();
    m_weights.assign(n, n ? 1.0 / n : 0.0);
    if (prior.size() == n) {
        double sum = 0;
        for (double w : prior)
            sum += std::max(w, 0.0);
        if (sum > 0)
            for (size_t i = 0; i < n; ++i)
                m_weights[i] = std::max(prior[i], 0.0) / sum;
    }
}

std::vector<double> LanguageTracker::logLikelihoods(const std::u32string &word) const
{
    std::vector<double> ll(m_dicts.size());
    for (size_t i = 0; i < m_dicts.size(); ++i) {
        const Dictionary &d = *m_dicts[i];
        const uint32_t node = d.find(word);
        // A misspelling (perche) still says which language it belongs to; its
        // node carries the frequency of the word it stands for.
        // A form made by the language's rules (kännykkänsä) is a rare word.
        const int freq = node ? std::max<int>(d.node(node).freq, m_params.unknownFreq)
                              : d.isWordForm(word) ? m_params.formFreq : m_params.unknownFreq;
        ll[i] = d.logProb(static_cast<uint8_t>(freq));
    }
    return ll;
}

void LanguageTracker::observe(const std::u32string &word)
{
    const size_t n = m_weights.size();
    if (n < 2 || word.empty())
        return;
    std::vector<double> ll = logLikelihoods(word);

    // Cap the evidence of a single word relative to the best language.
    const double top = *std::max_element(ll.begin(), ll.end());
    const double weight = word.size() <= 2 ? m_params.shortWordWeight : 1.0;
    for (double &x : ll)
        x = weight * std::max(x - top, -m_params.maxEvidence);

    // Forward step: transition, then emission.
    const double s = m_params.switchProb;
    std::vector<double> next(n);
    double sum = 0;
    for (size_t i = 0; i < n; ++i) {
        const double prior = m_weights[i] * (1.0 - s) + (1.0 - m_weights[i]) * s / (n - 1);
        next[i] = prior * std::exp(ll[i]);
        sum += next[i];
    }
    for (size_t i = 0; i < n; ++i)
        m_weights[i] = next[i] / sum;
}

int LanguageTracker::leading() const
{
    if (m_weights.empty())
        return -1;
    return static_cast<int>(std::max_element(m_weights.begin(), m_weights.end()) - m_weights.begin());
}

double LanguageTracker::confidence() const
{
    return m_weights.empty() ? 0.0 : *std::max_element(m_weights.begin(), m_weights.end());
}

bool setParam(LanguageParams &p, const std::string &assignment)
{
    const size_t eq = assignment.find('=');
    if (eq == std::string::npos)
        return false;
    const std::string name = assignment.substr(0, eq);
    const double v = std::stod(assignment.substr(eq + 1));
    if (name == "switchProb") p.switchProb = v;
    else if (name == "maxEvidence") p.maxEvidence = v;
    else if (name == "shortWordWeight") p.shortWordWeight = v;
    else if (name == "unknownFreq") p.unknownFreq = static_cast<int>(v);
    else if (name == "formFreq") p.formFreq = static_cast<int>(v);
    else return false;
    return true;
}

} // namespace parrotfish
