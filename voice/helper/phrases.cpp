// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "phrases.h"

namespace voice {

Plan Phrases::add(const Segment &segment)
{
    const size_t k = m_segments.size();
    if (k) {
        const int64_t mid = (m_segments.back().end + segment.start) / 2;
        m_ends.push_back(std::min(m_segments.back().end + Rate, mid));
        m_starts.push_back(std::max(segment.start - Rate, mid));
    } else {
        m_starts.push_back(std::max<int64_t>(0, segment.start - Rate));
    }
    m_segments.push_back(segment);

    Plan plan;
    plan.phrase = static_cast<long>(k);
    plan.to = segment.end + EndPad;
    if (k) {
        const int64_t from = std::max<int64_t>(0, m_starts[k - 1] - ContextSeconds * Rate);
        if (plan.to - from <= ChunkSeconds * Rate) {
            plan.kind = Plan::Pair;
            plan.from = from;
            plan.left = float(m_starts[k - 1] - from) / Rate;
            plan.split = float(m_ends[k - 1] - from) / Rate;
            return plan;
        }
    }
    plan.kind = !k && m_afterPrevious ? Plan::AfterPrevious : Plan::Alone;
    plan.from = m_starts[k];
    return plan;
}

} // namespace voice
