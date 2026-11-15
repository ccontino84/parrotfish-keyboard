// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <string>

namespace voice {

// Keeps this process off the network for good (seccomp, no root needed):
// the kernel refuses it sockets of any family but AF_UNIX (PulseAudio's),
// and io_uring, in every thread. False, with the reason, if it couldn't.
bool denyNetwork(std::string &error);

} // namespace voice
