// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "livetext.h"

namespace voice {

std::string LiveText::decode(Recognizer &recognizer, const Plan &plan, const std::vector<float> &samples,
                             float previous)
{
    const int n = static_cast<int>(samples.size());
    switch (plan.kind) {
    case Plan::Pair: {
        const auto parts = Recognizer::split(recognizer.tokens(samples.data(), n), plan.left, plan.split);
        fixed = Recognizer::append(fixed, parts.first);
        open = parts.second;
        return parts.first;
    }
    case Plan::AfterPrevious:
        open = Recognizer::split(recognizer.tokens(samples.data(), n), previous, previous).second;
        return std::string();
    case Plan::Alone:
        break;
    }
    const std::string before = open;
    fixed = Recognizer::append(fixed, before);
    open = recognizer.decode(samples);
    return before;
}

} // namespace voice
