// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include "common.h"

#include <vector>

namespace voice {

// What to decode when a phrase ends (a VAD segment): the phrase together
// with the one before, after ContextSeconds of audio before them; the
// phrase before becomes fixed. No audio or threads here: the PC bench uses
// the same plan.
struct Plan {
    enum Kind {
        Alone,           // the first phrase, or a pair too long to decode at once
        Pair,            // decode from-to; words before `left` are context,
                         // those up to `split` are phrase - 1's, the rest phrase's
        AfterPrevious,   // the first phrase after a pause: decode the end of
                         // the previous recording's speech, then from-to
    };
    Kind kind;
    long phrase;     // numbered from 0 in each recording
    int64_t from;    // samples
    int64_t to;
    float left = 0;  // seconds from `from`
    float split = 0;
};

class Phrases
{
public:
    // `afterPrevious`: the recording continues one whose speech can be
    // decoded before the first phrase.
    explicit Phrases(bool afterPrevious = false)
        : m_afterPrevious(afterPrevious)
    {
    }

    Plan add(const Segment &segment);

    bool empty() const { return m_segments.empty(); }
    // Where the last phrase's audio ends.
    int64_t end() const { return m_segments.empty() ? 0 : m_segments.back().end + EndPad; }

private:
    static constexpr int64_t EndPad = Rate / 4;

    bool m_afterPrevious;
    std::vector<Segment> m_segments;
    // Where each phrase's audio starts and, once the next one has started,
    // ends: Silero's segments start late (up to ~0.7 s), so a phrase
    // reaches into the middle of the pauses around it, at most 1 s.
    std::vector<int64_t> m_starts;
    std::vector<int64_t> m_ends;
};

} // namespace voice
