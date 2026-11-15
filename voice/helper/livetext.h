// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include "phrases.h"
#include "recognizer.h"

#include <string>
#include <vector>

namespace voice {

// The text of one recording, as each phrase's decode changes it. Shared with
// a PC benchmark, so that it measures this code.
struct LiveText {
    std::string fixed;   // the phrases that won't change any more
    std::string open;    // the newest phrase

    // Decodes `plan`'s audio and updates the text; returns the text that the
    // phrase before became (fixed). For AfterPrevious, `samples` starts with
    // `previous` seconds of the previous recording's speech, whose words are
    // dropped.
    std::string decode(Recognizer &recognizer, const Plan &plan, const std::vector<float> &samples,
                       float previous = 0);

    std::string all() const { return Recognizer::append(fixed, open); }
};

} // namespace voice
